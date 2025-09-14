#if 1

#if 0
#include <embree4/rtcore.h>
#include <iostream>
#include <vector>
#include <limits>
#include <array>
#include <string>
#include <cmath>
#include <glm/glm.hpp>
#include <utility>

struct Sphere
{
	float x, y, z; // center
	float radius;
};

int main()
{
	// Initialize Embree device
	RTCDevice device = rtcNewDevice(nullptr);
	if (!device)
	{
		std::cerr << "Failed to create Embree device\n";
		return -1;
	}
	std::cout << "Embree device created successfully" << std::endl;

	// Create scene
	RTCScene scene = rtcNewScene(device);

	// Create sphere geometry
	RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_SPHERE_POINT);

	// Define some spheres
	std::vector<Sphere> spheres = {
		{0.0f, 0.0f, 0.0f, 1.0f},  // Sphere at origin, radius 1
		{2.5f, 0.0f, 1.0f, 0.5f},  // Smaller sphere to the right
		{-1.5f, 1.0f, -0.5f, 0.8f} // Another sphere
	};

	// Set vertex buffer (sphere centers and radii)
	// Format: x, y, z, radius for each sphere
	float *vb = (float *)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0,
												 RTC_FORMAT_FLOAT4, 4 * sizeof(float), spheres.size());

	for (size_t i = 0; i < spheres.size(); i++)
	{
		vb[i * 4 + 0] = spheres[i].x;	   // x
		vb[i * 4 + 1] = spheres[i].y;	   // y
		vb[i * 4 + 2] = spheres[i].z;	   // z
		vb[i * 4 + 3] = spheres[i].radius; // radius
	}

	// Finalize geometry
	rtcCommitGeometry(geom);

	// Attach geometry to scene
	unsigned int geomID = rtcAttachGeometry(scene, geom);
	rtcReleaseGeometry(geom);

	// Commit scene (builds acceleration structure)
	rtcCommitScene(scene);

	std::cout << "Scene created with " << spheres.size() << " spheres\n";
	std::cout << "Testing ray intersections...\n\n";

	// Test multiple rays
	std::vector<std::pair<std::string, std::array<float, 6>>> test_rays = {
		{"Ray through origin sphere", {0.0f, 0.0f, -5.0f, 0.0f, 0.0f, 1.0f}},
		{"Ray towards right sphere", {2.5f, 0.0f, -5.0f, 0.0f, 0.0f, 1.0f}},
		{"Ray that misses all", {10.0f, 10.0f, -5.0f, 0.0f, 0.0f, 1.0f}},
		{"Diagonal ray", {-2.0f, -2.0f, -5.0f, 0.5f, 0.5f, 1.0f}}};

	for (const auto &[name, ray_data] : test_rays)
	{
		RTCRayHit rayhit;

		// Set up ray
		rayhit.ray.org_x = ray_data[0];
		rayhit.ray.org_y = ray_data[1];
		rayhit.ray.org_z = ray_data[2];
		rayhit.ray.dir_x = ray_data[3];
		rayhit.ray.dir_y = ray_data[4];
		rayhit.ray.dir_z = ray_data[5];
		rayhit.ray.tnear = 0.0f;
		rayhit.ray.tfar = std::numeric_limits<float>::infinity();
		rayhit.ray.mask = 0xFFFFFFFF;
		rayhit.ray.flags = 0;
		rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

		// Intersect ray with scene (simplified - no context needed for basic usage)
		rtcIntersect1(scene, &rayhit);

		std::cout << name << ":\n";

		// Check if ray hit something
		if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
		{
			float hit_x = rayhit.ray.org_x + rayhit.ray.tfar * rayhit.ray.dir_x;
			float hit_y = rayhit.ray.org_y + rayhit.ray.tfar * rayhit.ray.dir_y;
			float hit_z = rayhit.ray.org_z + rayhit.ray.tfar * rayhit.ray.dir_z;

			std::cout << "  HIT! Sphere #" << rayhit.hit.primID << " at distance " << rayhit.ray.tfar << "\n";
			std::cout << "  Hit point: (" << hit_x << ", " << hit_y << ", " << hit_z << ")\n";
			std::cout << "  Surface normal: (" << rayhit.hit.Ng_x << ", " << rayhit.hit.Ng_y << ", " << rayhit.hit.Ng_z << ")\n";
			std::cout << "  UV coordinates: (" << rayhit.hit.u << ", " << rayhit.hit.v << ")\n";
		}
		else
		{
			std::cout << "  MISS - Ray didn't hit any spheres\n";
		}
		std::cout << "\n";
	}

	// Cleanup
	rtcReleaseScene(scene);
	rtcReleaseDevice(device);

	return 0;
}

#else

#include "App.h"

int main()
{
	App app;
	app.run();
	return 0;
}
#endif

#else

#include <iostream>
#include <vector>

// Generates code for every target that this compiler can support.
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "main.cpp" // this file
#include <hwy/foreach_target.h>		  // must come before highway.h
#include <hwy/highway.h>

// Forward declaration for the exported function
namespace project
{
	void CallMulAddLoop(const float *HWY_RESTRICT mul_array,
						const float *HWY_RESTRICT add_array,
						const size_t size, float *HWY_RESTRICT x_array);
}

namespace project
{
	namespace HWY_NAMESPACE
	{ // required: unique per target

		// Can skip hn:: prefixes if already inside hwy::HWY_NAMESPACE.
		namespace hn = hwy::HWY_NAMESPACE;

		using T = float;

		// Alternative to per-function HWY_ATTR: see HWY_BEFORE_NAMESPACE
		HWY_ATTR void MulAddLoop(const T *HWY_RESTRICT mul_array,
								 const T *HWY_RESTRICT add_array,
								 const size_t size, T *HWY_RESTRICT x_array)
		{
			const hn::ScalableTag<T> d;
			const size_t lanes = hn::Lanes(d);

			std::cout << "Using SIMD with " << lanes << " lanes per vector\n";
			std::cout << "Target: " << hwy::TargetName(HWY_TARGET) << "\n";

			for (size_t i = 0; i < size; i += lanes)
			{
				const auto mul = hn::Load(d, mul_array + i);
				const auto add = hn::Load(d, add_array + i);
				auto x = hn::Load(d, x_array + i);
				x = hn::MulAdd(mul, x, add); // x = mul * x + add
				hn::Store(x, d, x_array + i);
			}
		}

	} // namespace HWY_NAMESPACE
} // namespace project

// The table of pointers to the various implementations in HWY_NAMESPACE must
// be compiled only once (foreach_target #includes this file multiple times).
// HWY_ONCE is true for only one of these 'compilation passes'.
#if HWY_ONCE

namespace project
{

	// This macro declares a static array used for dynamic dispatch.
	HWY_EXPORT(MulAddLoop);

	void CallMulAddLoop(const float *HWY_RESTRICT mul_array,
						const float *HWY_RESTRICT add_array,
						const size_t size, float *HWY_RESTRICT x_array)
	{
		// This must reside outside of HWY_NAMESPACE because it references (calls the
		// appropriate one from) the per-target implementations there.
		// For static dispatch, use HWY_STATIC_DISPATCH.
		return HWY_DYNAMIC_DISPATCH(MulAddLoop)(mul_array, add_array, size, x_array);
	}

} // namespace project

#endif // HWY_ONCE

#if HWY_ONCE
int main()
{
	// Create test data
	const size_t size = 8;
	std::vector<float> mul_array = {2.0f, 3.0f, 4.0f, 5.0f, 1.5f, 2.5f, 3.5f, 4.5f};
	std::vector<float> add_array = {1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.5f};
	std::vector<float> x_array = {10.0f, 20.0f, 30.0f, 40.0f, 100.0f, 200.0f, 300.0f, 400.0f};

	std::cout << "Initial values:\n";
	std::cout << "mul: ";
	for (const auto &val : mul_array)
		std::cout << val << " ";
	std::cout << "\nadd: ";
	for (const auto &val : add_array)
		std::cout << val << " ";
	std::cout << "\nx:   ";
	for (const auto &val : x_array)
		std::cout << val << " ";
	std::cout << "\n\n";

	// Perform mul-add operation: x = mul * x + add
	project::CallMulAddLoop(mul_array.data(), add_array.data(), size, x_array.data());

	std::cout << "After MulAdd (x = mul * x + add):\n";
	std::cout << "x:   ";
	for (const auto &val : x_array)
		std::cout << val << " ";
	std::cout << "\n\n";

	// Verify manually for first element: 2.0 * 10.0 + 1.0 = 21.0
	std::cout << "Manual verification for first element:\n";
	std::cout << "2.0 * 10.0 + 1.0 = " << (2.0f * 10.0f + 1.0f) << "\n";
	std::cout << "Result: " << x_array[0] << "\n";

	return 0;
}
#endif // HWY_ONCE
#endif // 0