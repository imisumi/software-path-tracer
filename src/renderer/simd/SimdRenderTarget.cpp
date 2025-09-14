#include "SimdRenderTarget.h"
#include "scene/Scene.h"
#include "geometry/sphere_data.h"
#include <algorithm>
#include <glm/gtc/constants.hpp>

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "renderer/simd/SimdRenderTarget.cpp"
#include <hwy/foreach_target.h>
#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();
namespace project
{
	namespace HWY_NAMESPACE
	{
		namespace hn = hwy::HWY_NAMESPACE;

		struct RayPacketF
		{
			hn::Vec<hn::ScalableTag<float>> origin_x, origin_y, origin_z;
			hn::Vec<hn::ScalableTag<float>> dir_x, dir_y, dir_z;
		};

		struct HitInfoF
		{
			hn::Vec<hn::ScalableTag<float>> t;
			hn::Vec<hn::ScalableTag<float>> position_x, position_y, position_z;
			hn::Vec<hn::ScalableTag<float>> normal_x, normal_y, normal_z;
			hn::Mask<hn::ScalableTag<float>> hit_mask;
		};

		struct ColorPacketF
		{
			hn::Vec<hn::ScalableTag<float>> r;
			hn::Vec<hn::ScalableTag<float>> g;
			hn::Vec<hn::ScalableTag<float>> b;
			hn::Vec<hn::ScalableTag<float>> a;
		};

		RayPacketF generatePrimaryRays(int x, int y, int width, int height)
		{
			constexpr auto d = hn::ScalableTag<float>{};
			constexpr auto di = hn::ScalableTag<uint32_t>{};
			const size_t N = hn::Lanes(d);

			// Generate lane indices for X coordinates
			const auto lane_indices = hn::Iota(di, x);
			const auto x_coords = hn::ConvertTo(d, lane_indices);
			const auto y_coord = hn::Set(d, float(y));

			// Convert to UV coordinates [-1, 1]
			const auto inv_width = hn::Set(d, 1.0f / float(width));
			const auto inv_height = hn::Set(d, 1.0f / float(height));
			const auto aspect_ratio = hn::Set(d, float(width) / float(height));
			const auto v_two = hn::Set(d, 2.0f);
			const auto v_one = hn::Set(d, 1.0f);

			const auto u = hn::Sub(hn::Mul(hn::Mul(x_coords, inv_width), v_two), v_one);
			const auto v = hn::Sub(hn::Mul(hn::Sub(v_one, hn::Mul(y_coord, inv_height)), v_two), v_one);

			// Ray direction = normalize(vec3(u * aspect, v, 1.0))
			const auto ray_x = hn::Mul(u, aspect_ratio);
			const auto ray_y = v;
			const auto ray_z = v_one;

			// Normalize ray directions
			const auto len_sq = hn::Add(hn::Add(hn::Mul(ray_x, ray_x), hn::Mul(ray_y, ray_y)), hn::Mul(ray_z, ray_z));
			const auto inv_len = hn::Div(v_one, hn::Sqrt(len_sq));

			RayPacketF rays;
			rays.origin_x = hn::Zero(d);
			rays.origin_y = hn::Zero(d);
			rays.origin_z = hn::Zero(d);
			rays.dir_x = hn::Mul(ray_x, inv_len);
			rays.dir_y = hn::Mul(ray_y, inv_len);
			rays.dir_z = hn::Mul(ray_z, inv_len);

			return rays;
		}

		HitInfoF intersectScene(const RayPacketF &rays, const SphereData &spheres)
		{
			constexpr auto d = hn::ScalableTag<float>{};

			HitInfoF hits;
			hits.t = hn::Set(d, std::numeric_limits<float>::infinity());
			hits.position_x = hn::Zero(d);
			hits.position_y = hn::Zero(d);
			hits.position_z = hn::Zero(d);
			hits.normal_x = hn::Zero(d);
			hits.normal_y = hn::Zero(d);
			hits.normal_z = hn::Zero(d);
			hits.hit_mask = hn::MaskFromVec(hn::Zero(d));

			// Test intersection with each sphere
			for (size_t sphere_idx = 0; sphere_idx < spheres.size(); ++sphere_idx)
			{
				// Sphere center and radius (broadcast to SIMD lanes)
				const auto sphere_cx = hn::Set(d, spheres.cx[sphere_idx]);
				const auto sphere_cy = hn::Set(d, spheres.cy[sphere_idx]);
				const auto sphere_cz = hn::Set(d, spheres.cz[sphere_idx]);
				const auto sphere_r = hn::Set(d, spheres.radii[sphere_idx]);

				// Vector from ray origin to sphere center
				const auto oc_x = hn::Sub(rays.origin_x, sphere_cx);
				const auto oc_y = hn::Sub(rays.origin_y, sphere_cy);
				const auto oc_z = hn::Sub(rays.origin_z, sphere_cz);

				// Quadratic equation coefficients: at² + bt + c = 0
				// a = dot(ray_dir, ray_dir) = 1.0 (normalized rays)
				const auto a = hn::Set(d, 1.0f);

				// b = 2 * dot(oc, ray_dir)
				const auto b = hn::Mul(hn::Set(d, 2.0f),
									   hn::Add(hn::Add(hn::Mul(oc_x, rays.dir_x), hn::Mul(oc_y, rays.dir_y)), hn::Mul(oc_z, rays.dir_z)));

				// c = dot(oc, oc) - r²
				const auto oc_dot_oc = hn::Add(hn::Add(hn::Mul(oc_x, oc_x), hn::Mul(oc_y, oc_y)), hn::Mul(oc_z, oc_z));
				const auto c = hn::Sub(oc_dot_oc, hn::Mul(sphere_r, sphere_r));

				// Discriminant = b² - 4ac
				const auto discriminant = hn::Sub(hn::Mul(b, b), hn::Mul(hn::Set(d, 4.0f), hn::Mul(a, c)));

				// Check if intersection exists (discriminant >= 0)
				const auto has_intersection = hn::Ge(discriminant, hn::Zero(d));

				// Calculate t values only for rays with valid intersections
				const auto sqrt_discriminant = hn::Sqrt(hn::Max(discriminant, hn::Zero(d)));
				const auto inv_2a = hn::Set(d, 0.5f); // 1/(2*a) where a=1

				const auto t1 = hn::Mul(hn::Sub(hn::Neg(b), sqrt_discriminant), inv_2a);
				const auto t2 = hn::Mul(hn::Sub(hn::Neg(b), hn::Neg(sqrt_discriminant)), inv_2a);

				// Choose closest positive t
				const auto t_min = hn::Min(t1, t2);
				const auto t_max = hn::Max(t1, t2);
				const auto t_positive_min = hn::IfThenElse(hn::Gt(t_min, hn::Zero(d)), t_min, t_max);

				// Valid hit: has intersection AND t > 0 AND t < current closest
				const auto valid_hit = hn::And(has_intersection,
											   hn::And(hn::Gt(t_positive_min, hn::Zero(d)), hn::Lt(t_positive_min, hits.t)));

				// Update hit info for closer intersections
				hits.t = hn::IfThenElse(valid_hit, t_positive_min, hits.t);
				hits.hit_mask = hn::Or(hits.hit_mask, valid_hit);

				// Calculate hit point and normal for valid hits
				const auto hit_x = hn::Add(rays.origin_x, hn::Mul(rays.dir_x, t_positive_min));
				const auto hit_y = hn::Add(rays.origin_y, hn::Mul(rays.dir_y, t_positive_min));
				const auto hit_z = hn::Add(rays.origin_z, hn::Mul(rays.dir_z, t_positive_min));

				// Normal = (hit_point - sphere_center) / radius
				const auto normal_x = hn::Div(hn::Sub(hit_x, sphere_cx), sphere_r);
				const auto normal_y = hn::Div(hn::Sub(hit_y, sphere_cy), sphere_r);
				const auto normal_z = hn::Div(hn::Sub(hit_z, sphere_cz), sphere_r);

				// Update positions and normals for valid hits
				hits.position_x = hn::IfThenElse(valid_hit, hit_x, hits.position_x);
				hits.position_y = hn::IfThenElse(valid_hit, hit_y, hits.position_y);
				hits.position_z = hn::IfThenElse(valid_hit, hit_z, hits.position_z);
				hits.normal_x = hn::IfThenElse(valid_hit, normal_x, hits.normal_x);
				hits.normal_y = hn::IfThenElse(valid_hit, normal_y, hits.normal_y);
				hits.normal_z = hn::IfThenElse(valid_hit, normal_z, hits.normal_z);
			}

			return hits;
		}

		// SIMD random number generation using simple LCG per lane
		struct RngStateF
		{
			hn::Vec<hn::ScalableTag<uint32_t>> state;
		};

		RngStateF initRngState(int x, int y, int width, uint32_t frame)
		{
			constexpr auto di = hn::ScalableTag<uint32_t>{};
			const auto lane_indices = hn::Iota(di, 0);

			const auto x_coords = hn::Add(hn::Set(di, uint32_t(x)), lane_indices);
			const auto y_coord = hn::Set(di, uint32_t(y));

			// Better mixing - each lane gets a different sequence
			auto seed = hn::Add(x_coords, hn::Mul(y_coord, hn::Set(di, uint32_t(width))));
			seed = hn::Add(seed, hn::Mul(hn::Set(di, frame), hn::Set(di, 982451653U)));

			// Hash the seed better
			seed = hn::Xor(seed, hn::ShiftRight<16>(seed));
			seed = hn::Mul(seed, hn::Set(di, 0x85ebca6bU));
			seed = hn::Xor(seed, hn::ShiftRight<13>(seed));
			seed = hn::Mul(seed, hn::Set(di, 0xc2b2ae35U));
			seed = hn::Xor(seed, hn::ShiftRight<16>(seed));

			RngStateF rng;
			rng.state = seed;
			return rng;
		}

		hn::Vec<hn::ScalableTag<float>> randomFloat(RngStateF &rng)
		{
			constexpr auto di = hn::ScalableTag<uint32_t>{};
			constexpr auto d = hn::ScalableTag<float>{};

			// PCG algorithm (same as CPU version)
			const auto a = hn::Set(di, 747796405U);
			const auto c = hn::Set(di, 2891336453U);
			const auto multiplier = hn::Set(di, 277803737U);

			// state = state * a + c
			rng.state = hn::Add(hn::Mul(rng.state, a), c);

			// PCG output permutation
			const auto shift_amount = hn::Add(hn::ShiftRight<28>(rng.state), hn::Set(di, 4U));
			// Note: Variable shifts are complex in SIMD, so use a simplified version
			const auto xorshifted = hn::Xor(hn::ShiftRight<22>(rng.state), rng.state);
			const auto result_int = hn::Mul(xorshifted, multiplier);
			const auto final_result = hn::Xor(hn::ShiftRight<22>(result_int), result_int);

			// Convert to float [0, 1)
			const auto result = hn::Mul(hn::ConvertTo(d, final_result),
										hn::Set(d, 1.0f / 4294967296.0f)); // 2^32
			return result;
		}

		void generateRandomDiffuseDirections(RayPacketF &rays,
											 const hn::Vec<hn::ScalableTag<float>> &normal_x,
											 const hn::Vec<hn::ScalableTag<float>> &normal_y,
											 const hn::Vec<hn::ScalableTag<float>> &normal_z,
											 RngStateF &rng)
		{
			constexpr auto d = hn::ScalableTag<float>{};
			const auto v_one = hn::Set(d, 1.0f);
			const auto v_two = hn::Set(d, 2.0f);
			const auto v_zero = hn::Zero(d);

			// Keep generating until all lanes have valid directions
			auto valid_mask = hn::MaskFromVec(v_zero); // Start with all false
			auto local_x = v_zero;
			auto local_y = v_zero;
			auto local_z = v_zero;

			// Rejection sampling for uniform hemisphere
			for (int attempt = 0; attempt < 8 && !hn::AllTrue(d, valid_mask); ++attempt)
			{
				const auto u1 = randomFloat(rng);
				const auto u2 = randomFloat(rng);
				const auto u3 = randomFloat(rng);

				// Generate random point in unit sphere
				const auto x = hn::Sub(hn::Mul(u1, v_two), v_one); // [-1, 1]
				const auto y = hn::Sub(hn::Mul(u2, v_two), v_one);
				const auto z = hn::Sub(hn::Mul(u3, v_two), v_one);

				const auto len_sq = hn::Add(hn::Add(hn::Mul(x, x), hn::Mul(y, y)), hn::Mul(z, z));
				const auto in_sphere = hn::And(hn::Lt(len_sq, v_one), hn::Gt(len_sq, hn::Set(d, 1e-6f)));

				// Only update lanes that need new values and are valid
				const auto update_mask = hn::And(in_sphere, hn::Not(valid_mask));

				local_x = hn::IfThenElse(update_mask, x, local_x);
				local_y = hn::IfThenElse(update_mask, y, local_y);
				local_z = hn::IfThenElse(update_mask, hn::Abs(z), local_z); // Force to upper hemisphere

				valid_mask = hn::Or(valid_mask, update_mask);
			}

			// Normalize the directions
			const auto len = hn::Sqrt(hn::Add(hn::Add(hn::Mul(local_x, local_x),
													  hn::Mul(local_y, local_y)),
											  hn::Mul(local_z, local_z)));
			const auto inv_len = hn::Div(v_one, hn::Max(len, hn::Set(d, 1e-6f)));

			local_x = hn::Mul(local_x, inv_len);
			local_y = hn::Mul(local_y, inv_len);
			local_z = hn::Mul(local_z, inv_len);

			// Build tangent frame - choose axis with smallest component
			const auto abs_nx = hn::Abs(normal_x);
			const auto abs_ny = hn::Abs(normal_y);
			const auto abs_nz = hn::Abs(normal_z);

			const auto use_x_axis = hn::And(hn::Le(abs_nx, abs_ny), hn::Le(abs_nx, abs_nz));
			const auto use_y_axis = hn::And(hn::Not(use_x_axis), hn::Le(abs_ny, abs_nz));

			const auto temp_x = hn::IfThenElse(use_x_axis, v_one, v_zero);
			const auto temp_y = hn::IfThenElse(use_y_axis, v_one, v_zero);
			const auto temp_z = hn::IfThenElse(hn::Or(use_x_axis, use_y_axis), v_zero, v_one);

			// Tangent = temp - dot(temp, normal) * normal (Gram-Schmidt)
			const auto dot = hn::Add(hn::Add(hn::Mul(temp_x, normal_x),
											 hn::Mul(temp_y, normal_y)),
									 hn::Mul(temp_z, normal_z));

			const auto tangent_x = hn::Sub(temp_x, hn::Mul(dot, normal_x));
			const auto tangent_y = hn::Sub(temp_y, hn::Mul(dot, normal_y));
			const auto tangent_z = hn::Sub(temp_z, hn::Mul(dot, normal_z));

			// Normalize tangent
			const auto tangent_len = hn::Sqrt(hn::Add(hn::Add(hn::Mul(tangent_x, tangent_x),
															  hn::Mul(tangent_y, tangent_y)),
													  hn::Mul(tangent_z, tangent_z)));
			const auto inv_tangent_len = hn::Div(v_one, hn::Max(tangent_len, hn::Set(d, 1e-6f)));

			const auto t_x = hn::Mul(tangent_x, inv_tangent_len);
			const auto t_y = hn::Mul(tangent_y, inv_tangent_len);
			const auto t_z = hn::Mul(tangent_z, inv_tangent_len);

			// Bitangent = cross(normal, tangent)
			const auto bt_x = hn::Sub(hn::Mul(normal_y, t_z), hn::Mul(normal_z, t_y));
			const auto bt_y = hn::Sub(hn::Mul(normal_z, t_x), hn::Mul(normal_x, t_z));
			const auto bt_z = hn::Sub(hn::Mul(normal_x, t_y), hn::Mul(normal_y, t_x));

			// Transform local coordinates to world space
			rays.dir_x = hn::Add(hn::Add(hn::Mul(local_x, t_x), hn::Mul(local_y, bt_x)), hn::Mul(local_z, normal_x));
			rays.dir_y = hn::Add(hn::Add(hn::Mul(local_x, t_y), hn::Mul(local_y, bt_y)), hn::Mul(local_z, normal_y));
			rays.dir_z = hn::Add(hn::Add(hn::Mul(local_x, t_z), hn::Mul(local_y, bt_z)), hn::Mul(local_z, normal_z));
		}

		// Generate random diffuse direction using simplified cosine-weighted hemisphere sampling
		// void generateRandomDiffuseDirections(RayPacketF &rays,
		// 									 const hn::Vec<hn::ScalableTag<float>> &normal_x,
		// 									 const hn::Vec<hn::ScalableTag<float>> &normal_y,
		// 									 const hn::Vec<hn::ScalableTag<float>> &normal_z,
		// 									 RngStateF &rng)
		// {
		// 	constexpr auto d = hn::ScalableTag<float>{};
		// 	const auto v_one = hn::Set(d, 1.0f);
		// 	const auto v_two = hn::Set(d, 2.0f);

		// 	// Generate random point in unit disk using rejection sampling
		// 	// This is simpler than computing sin/cos
		// 	auto local_x = hn::Zero(d);
		// 	auto local_y = hn::Zero(d);

		// 	// Simple approximation: use uniform hemisphere distribution (not perfectly cosine-weighted)
		// 	// u1, u2 in [0,1)
		// 	const auto u1 = randomFloat(rng);
		// 	const auto u2 = randomFloat(rng);

		// 	// Convert to [-1,1] for disk sampling
		// 	const auto disk_x = hn::Sub(hn::Mul(u1, v_two), v_one);
		// 	const auto disk_y = hn::Sub(hn::Mul(u2, v_two), v_one);

		// 	// Only keep points inside unit circle
		// 	const auto len_sq = hn::Add(hn::Mul(disk_x, disk_x), hn::Mul(disk_y, disk_y));
		// 	const auto in_circle = hn::Le(len_sq, v_one);

		// 	// If outside circle, clamp to edge (not perfectly correct but works)
		// 	const auto len = hn::Sqrt(hn::Max(len_sq, hn::Set(d, 1e-6f)));
		// 	const auto inv_len = hn::Div(v_one, len);
		// 	local_x = hn::IfThenElse(in_circle, disk_x, hn::Mul(disk_x, inv_len));
		// 	local_y = hn::IfThenElse(in_circle, disk_y, hn::Mul(disk_y, inv_len));

		// 	// z coordinate from hemisphere constraint: z = sqrt(1 - x² - y²)
		// 	const auto local_len_sq = hn::Add(hn::Mul(local_x, local_x), hn::Mul(local_y, local_y));
		// 	const auto local_z = hn::Sqrt(hn::Max(hn::Sub(v_one, local_len_sq), hn::Zero(d)));

		// 	// Build orthonormal basis from normal (simplified version)
		// 	// Choose up vector that's not parallel to normal
		// 	const auto abs_nz = hn::Abs(normal_z);
		// 	const auto use_z = hn::Lt(abs_nz, hn::Set(d, 0.9f));

		// 	const auto up_x = hn::IfThenElse(use_z, hn::Zero(d), v_one);
		// 	const auto up_y = hn::IfThenElse(use_z, hn::Zero(d), hn::Zero(d));
		// 	const auto up_z = hn::IfThenElse(use_z, v_one, hn::Zero(d));

		// 	// Tangent = normalize(cross(up, normal))
		// 	const auto cross_x = hn::Sub(hn::Mul(up_y, normal_z), hn::Mul(up_z, normal_y));
		// 	const auto cross_y = hn::Sub(hn::Mul(up_z, normal_x), hn::Mul(up_x, normal_z));
		// 	const auto cross_z = hn::Sub(hn::Mul(up_x, normal_y), hn::Mul(up_y, normal_x));

		// 	const auto cross_len = hn::Sqrt(hn::Add(hn::Add(hn::Mul(cross_x, cross_x),
		// 													hn::Mul(cross_y, cross_y)),
		// 											hn::Mul(cross_z, cross_z)));
		// 	const auto inv_cross_len = hn::Div(v_one, hn::Max(cross_len, hn::Set(d, 1e-6f)));

		// 	const auto tangent_x = hn::Mul(cross_x, inv_cross_len);
		// 	const auto tangent_y = hn::Mul(cross_y, inv_cross_len);
		// 	const auto tangent_z = hn::Mul(cross_z, inv_cross_len);

		// 	// Bitangent = cross(normal, tangent)
		// 	const auto bitangent_x = hn::Sub(hn::Mul(normal_y, tangent_z), hn::Mul(normal_z, tangent_y));
		// 	const auto bitangent_y = hn::Sub(hn::Mul(normal_z, tangent_x), hn::Mul(normal_x, tangent_z));
		// 	const auto bitangent_z = hn::Sub(hn::Mul(normal_x, tangent_y), hn::Mul(normal_y, tangent_x));

		// 	// Transform to world space: local_x * tangent + local_y * bitangent + local_z * normal
		// 	rays.dir_x = hn::Add(hn::Add(hn::Mul(local_x, tangent_x),
		// 								 hn::Mul(local_y, bitangent_x)),
		// 						 hn::Mul(local_z, normal_x));
		// 	rays.dir_y = hn::Add(hn::Add(hn::Mul(local_x, tangent_y),
		// 								 hn::Mul(local_y, bitangent_y)),
		// 						 hn::Mul(local_z, normal_y));
		// 	rays.dir_z = hn::Add(hn::Add(hn::Mul(local_x, tangent_z),
		// 								 hn::Mul(local_y, bitangent_z)),
		// 						 hn::Mul(local_z, normal_z));
		// }

		void generateDiffuseBounces(RayPacketF& bounceRays, const HitInfoF &hits, RngStateF &rng)
		{
			constexpr auto d = hn::ScalableTag<float>{};

			// Origins are hit points (slightly offset along normal to avoid self-intersection)
			const auto epsilon = hn::Set(d, 0.001f);
			bounceRays.origin_x = hn::Add(hits.position_x, hn::Mul(hits.normal_x, epsilon));
			bounceRays.origin_y = hn::Add(hits.position_y, hn::Mul(hits.normal_y, epsilon));
			bounceRays.origin_z = hn::Add(hits.position_z, hn::Mul(hits.normal_z, epsilon));

			// Generate random diffuse directions
			generateRandomDiffuseDirections(bounceRays, hits.normal_x, hits.normal_y, hits.normal_z, rng);
		}

		ColorPacketF tracePixelPacket(int x, int y, int width, int height, const SphereData &spheres, uint32_t frame)
		{
			constexpr auto d = hn::ScalableTag<float>{};
			const int maxBounces = 8;

			// Start with primary rays
			RayPacketF rays = generatePrimaryRays(x, y, width, height);
			ColorPacketF accumulatedColor;
			accumulatedColor.r = hn::Zero(d);
			accumulatedColor.g = hn::Zero(d);
			accumulatedColor.b = hn::Zero(d);
			accumulatedColor.a = hn::Set(d, 1.0f);

			ColorPacketF throughput;
			throughput.r = hn::Set(d, 1.0f);
			throughput.g = hn::Set(d, 1.0f);
			throughput.b = hn::Set(d, 1.0f);
			throughput.a = hn::Set(d, 1.0f);

			// Initialize RNG state
			RngStateF rng = initRngState(x, y, width, frame);

			for (int bounce = 0; bounce < maxBounces; bounce++)
			{
				HitInfoF hits = intersectScene(rays, spheres);

				// Sky gradient for misses
				const auto v_half = hn::Set(d, 0.5f);
				const auto v_one = hn::Set(d, 1.0f);
				const auto t = hn::Mul(v_half, hn::Add(rays.dir_y, v_one));

				const auto sky_blue_r = hn::Set(d, 0.5f);
				const auto sky_blue_g = hn::Set(d, 0.7f);
				const auto sky_blue_b = v_one;

				const auto one_minus_t = hn::Sub(v_one, t);
				const auto sky_r = hn::Add(hn::Mul(v_one, one_minus_t), hn::Mul(sky_blue_r, t));
				const auto sky_g = hn::Add(hn::Mul(v_one, one_minus_t), hn::Mul(sky_blue_g, t));
				const auto sky_b = hn::Add(hn::Mul(v_one, one_minus_t), hn::Mul(sky_blue_b, t));

				// Add sky contribution for rays that missed AND exit early for misses
				const auto miss_contribution_r = hn::Mul(throughput.r, sky_r);
				const auto miss_contribution_g = hn::Mul(throughput.g, sky_g);
				const auto miss_contribution_b = hn::Mul(throughput.b, sky_b);

				accumulatedColor.r = hn::Add(accumulatedColor.r,
											 hn::IfThenElse(hits.hit_mask, hn::Zero(d), miss_contribution_r));
				accumulatedColor.g = hn::Add(accumulatedColor.g,
											 hn::IfThenElse(hits.hit_mask, hn::Zero(d), miss_contribution_g));
				accumulatedColor.b = hn::Add(accumulatedColor.b,
											 hn::IfThenElse(hits.hit_mask, hn::Zero(d), miss_contribution_b));

				// Check if any rays are still active (hit something)
				if (hn::AllFalse(d, hits.hit_mask))
				{
					break; // All rays missed, no more bounces possible
				}

				// Material albedo - same as CPU version
				const auto albedo = hn::Set(d, 0.7f);
				throughput.r = hn::IfThenElse(hits.hit_mask, hn::Mul(throughput.r, albedo), hn::Zero(d));
				throughput.g = hn::IfThenElse(hits.hit_mask, hn::Mul(throughput.g, albedo), hn::Zero(d));
				throughput.b = hn::IfThenElse(hits.hit_mask, hn::Mul(throughput.b, albedo), hn::Zero(d));

				// Russian roulette termination after bounce 3
				if (bounce > 3)
				{
					const auto max_throughput = hn::Max(hn::Max(throughput.r, throughput.g), throughput.b);
					const auto continue_prob = hn::Min(max_throughput, hn::Set(d, 0.95f));
					const auto rr_rand = randomFloat(rng);
					const auto continue_mask = hn::And(hits.hit_mask, hn::Lt(rr_rand, continue_prob));

					// Early termination if no rays continue
					if (hn::AllFalse(d, continue_mask))
					{
						break;
					}

					// Boost throughput for surviving rays
					const auto inv_continue_prob = hn::Div(v_one, continue_prob);
					throughput.r = hn::IfThenElse(continue_mask, hn::Mul(throughput.r, inv_continue_prob), hn::Zero(d));
					throughput.g = hn::IfThenElse(continue_mask, hn::Mul(throughput.g, inv_continue_prob), hn::Zero(d));
					throughput.b = hn::IfThenElse(continue_mask, hn::Mul(throughput.b, inv_continue_prob), hn::Zero(d));

					// Update hit mask
					hits.hit_mask = continue_mask;
				}

				// Generate bounce rays for surviving hits
				generateDiffuseBounces(rays, hits, rng);
			}

			return accumulatedColor;
		}

		ColorPacketF shade(const HitInfoF &hits, const RayPacketF &rays)
		{
			constexpr auto d = hn::ScalableTag<float>{};

			// Sky gradient using ray direction Y component (for misses)
			const auto v_half = hn::Set(d, 0.5f);
			const auto v_one = hn::Set(d, 1.0f);
			const auto t = hn::Mul(v_half, hn::Add(rays.dir_y, v_one));

			// Sky colors: white -> blue gradient
			const auto sky_blue_r = hn::Set(d, 0.5f);
			const auto sky_blue_g = hn::Set(d, 0.7f);
			const auto sky_blue_b = v_one;

			const auto one_minus_t = hn::Sub(v_one, t);
			const auto sky_r = hn::Add(hn::Mul(v_one, one_minus_t), hn::Mul(sky_blue_r, t));
			const auto sky_g = hn::Add(hn::Mul(v_one, one_minus_t), hn::Mul(sky_blue_g, t));
			const auto sky_b = hn::Add(hn::Mul(v_one, one_minus_t), hn::Mul(sky_blue_b, t));

			// Hit colors: normal visualization (map [-1,1] to [0,1])
			const auto hit_r = hn::Mul(hn::Add(hits.normal_x, v_one), v_half);
			const auto hit_g = hn::Mul(hn::Add(hits.normal_y, v_one), v_half);
			const auto hit_b = hn::Mul(hn::Add(hits.normal_z, v_one), v_half);

			// Select between hit and miss colors based on hit mask
			ColorPacketF colors;
			colors.r = hn::IfThenElse(hits.hit_mask, hit_r, sky_r);
			colors.g = hn::IfThenElse(hits.hit_mask, hit_g, sky_g);
			colors.b = hn::IfThenElse(hits.hit_mask, hit_b, sky_b);
			colors.a = v_one;

			return colors;
		}

		void writeColorsToBuffer(const ColorPacketF &colors, std::vector<glm::vec4> &float_data,
								 int x, int y, int width, size_t lanes)
		{
			constexpr auto d = hn::ScalableTag<float>{};

			// Handle edge case where packet extends beyond image width
			const size_t pixels_to_write = std::min(lanes, size_t(width - x));

			alignas(32) float r[hn::MaxLanes(d)], g[hn::MaxLanes(d)], b[hn::MaxLanes(d)];
			hn::Store(colors.r, d, r);
			hn::Store(colors.g, d, g);
			hn::Store(colors.b, d, b);

			for (size_t i = 0; i < pixels_to_write; ++i)
			{
				const int pixel_idx = y * width + x + static_cast<int>(i);
				// Direct write to internal format, bypassing setPixel overhead
				float_data[pixel_idx] += glm::vec4(r[i], g[i], b[i], 1.0f);
			}
		}

		void render(std::vector<glm::vec4> &float_data, int width, int height, const SphereData &spheres, uint32_t frame)
		{
			constexpr auto d = hn::ScalableTag<float>{};
			const size_t N = hn::Lanes(d);

			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x += static_cast<int>(N))
				{
					// Trace pixel packet with bounces
					const ColorPacketF colors = tracePixelPacket(x, y, width, height, spheres, frame);

					// Write directly to internal format
					writeColorsToBuffer(colors, float_data, x, y, width, N);
				}
			}
		}

	} // namespace HWY_NAMESPACE
} // namespace project
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace project
{
	HWY_EXPORT(render);

	// Dispatch function that calls the best available SIMD implementation
	void simd_render(std::vector<glm::vec4> &float_data, int width, int height, const SphereData &spheres, uint32_t frame)
	{
		return HWY_DYNAMIC_DISPATCH(render)(float_data, width, height, spheres, frame);
	}
}

SimdRenderTarget::SimdRenderTarget(uint32_t width, uint32_t height)
{
	m_texture = std::make_unique<Texture2D>(width, height, Texture2D::Format::RGBA8);
	m_floatData.resize(width * height, glm::vec4(0.0f));
	m_displayData.resize(width * height, 0);
	m_frameCount = 0;
}

void SimdRenderTarget::setPixel(uint32_t x, uint32_t y, const glm::vec3 &color)
{
	if (x >= getWidth() || y >= getHeight())
		return;

	uint32_t index = y * getWidth() + x;
	// Accumulate color instead of overwriting
	m_floatData[index] += glm::vec4(color, 1.0f);
}

void SimdRenderTarget::setPixel(uint32_t x, uint32_t y, const glm::vec4 &color)
{
	if (x >= getWidth() || y >= getHeight())
		return;

	uint32_t index = y * getWidth() + x;
	// Accumulate color instead of overwriting
	m_floatData[index] += color;
}

void SimdRenderTarget::updateRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	(void)x;
	(void)y;
	(void)width;
	(void)height;
	// For CPU rendering, we update the entire texture at once
	// This could be optimized to update only the specified region
	commitPixels();
}

uint32_t SimdRenderTarget::getWidth() const
{
	return m_texture ? m_texture->get_width() : 0;
}

uint32_t SimdRenderTarget::getHeight() const
{
	return m_texture ? m_texture->get_height() : 0;
}

void SimdRenderTarget::clear(const glm::vec3 &color)
{
	glm::vec4 clearColor = glm::vec4(color, 1.0f);
	std::fill(m_floatData.begin(), m_floatData.end(), clearColor);
	m_frameCount = 0;
}

void SimdRenderTarget::resize(uint32_t width, uint32_t height)
{
	if (m_texture && width == m_texture->get_width() && height == m_texture->get_height())
	{
		return;
	}

	if (m_texture)
	{
		m_texture->resize(width, height);
	}
	else
	{
		m_texture = std::make_unique<Texture2D>(width, height, Texture2D::Format::RGBA8);
	}

	m_floatData.resize(width * height, glm::vec4(0.0f));
	m_displayData.resize(width * height, 0);
	m_frameCount = 0;
}

void SimdRenderTarget::commitPixels()
{
	if (m_texture && m_frameCount > 0)
	{
		// Convert float buffer to uint8 RGBA for display
		// Divide by frame count for averaging
		for (size_t i = 0; i < m_floatData.size(); ++i)
		{
			glm::vec3 averagedColor = glm::vec3(m_floatData[i]) / static_cast<float>(m_frameCount);
			m_displayData[i] = colorToRGBA(averagedColor);
		}
		m_texture->set_data(m_displayData);
	}
}

void SimdRenderTarget::render(const Scene &scene, uint32_t frame)
{

	const uint32_t width = getWidth();
	const uint32_t height = getHeight();

	// Clear on first frame (frame == 1) to reset accumulation
	if (frame == 1)
	{
		clear(glm::vec3(0.0f, 0.0f, 0.0f));
	}

	// Increment frame count for accumulation
	m_frameCount++;

	// Use the new Highway-based SIMD render function with direct access to float data
	project::simd_render(m_floatData, width, height, scene.get_sphere_data(), frame);

	// Commit all pixels to the texture
	commitPixels();
}

uint32_t SimdRenderTarget::colorToRGBA(const glm::vec3 &color) const
{
	const uint8_t r = (uint8_t)(std::clamp(color.r, 0.0f, 1.0f) * 255);
	const uint8_t g = (uint8_t)(std::clamp(color.g, 0.0f, 1.0f) * 255);
	const uint8_t b = (uint8_t)(std::clamp(color.b, 0.0f, 1.0f) * 255);
	const uint8_t a = 255;

	return (r << 24) | (g << 16) | (b << 8) | (a << 0);
}

#endif