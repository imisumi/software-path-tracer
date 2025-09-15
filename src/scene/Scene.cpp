#include "Scene.h"
#include <limits>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <embree4/rtcore.h>
#include <OpenImageIO/imageio.h>
#include <glm/gtc/constants.hpp>

Scene::Scene()
{
}

Scene::~Scene()
{
	cleanup_embree();
}

void Scene::init_embree()
{
	if (device) return; // Already initialized
	
	device = rtcNewDevice("verbose=1,threads=0");
	if (!device) {
		throw std::runtime_error("Failed to create Embree device");
	}
	
	scene = rtcNewScene(device);
	if (!scene) {
		throw std::runtime_error("Failed to create Embree scene");
	}
	
	// Create geometry for spheres
	sphere_geometry = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_SPHERE_POINT);
	
	// Will be updated when spheres are added
	rebuild_embree_scene();
}

void Scene::cleanup_embree()
{
	if (sphere_geometry) {
		rtcReleaseGeometry(sphere_geometry);
		sphere_geometry = nullptr;
	}
	if (scene) {
		rtcReleaseScene(scene);
		scene = nullptr;
	}
	if (device) {
		rtcReleaseDevice(device);
		device = nullptr;
	}
}

void Scene::add_sphere(glm::vec3 center, float radius)
{
	sphere_data.add_sphere(center, radius, 0);
	if (device) {
		update_embree_geometry();
	}
}

void Scene::remove_sphere(uint32_t index)
{
	sphere_data.remove_sphere(index);
	if (device) {
		update_embree_geometry();
	}
}

void Scene::update_sphere(uint32_t index, glm::vec3 center, float radius, uint32_t material_index)
{
	sphere_data.update_sphere(index, center, radius, material_index);
	if (device) {
		update_embree_geometry();
	}
}

void Scene::rebuild_embree_scene()
{
	if (!device || !scene) return;
	
	// Detach old geometry if it exists
	if (sphere_geometry) {
		rtcDetachGeometry(scene, embree_geometry_id);
		rtcReleaseGeometry(sphere_geometry);
	}
	
	// Create new geometry
	sphere_geometry = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_SPHERE_POINT);
	update_embree_geometry();
	
	// Attach to scene
	embree_geometry_id = rtcAttachGeometry(scene, sphere_geometry);
	
	// Commit scene
	rtcCommitScene(scene);
}

void Scene::update_embree_geometry()
{
	if (!sphere_geometry || sphere_data.size() == 0) return;
	
	// Set vertex buffer (sphere centers and radii)
	float *vb = (float *)rtcSetNewGeometryBuffer(sphere_geometry, RTC_BUFFER_TYPE_VERTEX, 0,
												 RTC_FORMAT_FLOAT4, 4 * sizeof(float), sphere_data.size());
	
	// Copy sphere data to Embree format
	for (size_t i = 0; i < sphere_data.size(); i++) {
		vb[i * 4 + 0] = sphere_data.cx[i];     // x
		vb[i * 4 + 1] = sphere_data.cy[i];     // y
		vb[i * 4 + 2] = sphere_data.cz[i];     // z
		vb[i * 4 + 3] = sphere_data.radii[i];  // radius
	}
	
	// Commit geometry changes
	rtcCommitGeometry(sphere_geometry);
	
	// Commit scene to rebuild acceleration structure
	if (scene) {
		rtcCommitScene(scene);
	}
}

// EnvironmentMap implementation
void EnvironmentMap::clear()
{
	pixels.clear();
	width = height = 0;
	filename.clear();
	is_loaded = false;
}

bool EnvironmentMap::load_from_file(const std::string& filepath)
{
	clear();
	
	auto in = OIIO::ImageInput::open(filepath);
	if (!in) {
		std::cerr << "ERROR: Could not open " << filepath << " : " << OIIO::geterror() << std::endl;
		return false;
	}
	
	const OIIO::ImageSpec& spec = in->spec();
	width = spec.width;
	height = spec.height;
	int channels = spec.nchannels;
	
	std::cout << "Loading HDR environment: " << filepath << std::endl;
	std::cout << "  Resolution: " << width << "x" << height << std::endl;
	std::cout << "  Channels: " << channels << std::endl;
	std::cout << "  Format: " << spec.format.c_str() << std::endl;
	
	// Read as float RGB data
	std::vector<float> raw_pixels(width * height * channels);
	if (!in->read_image(0, 0, 0, channels, OIIO::TypeDesc::FLOAT, raw_pixels.data())) {
		std::cerr << "ERROR: Could not read pixel data from " << filepath << std::endl;
		in->close();
		return false;
	}
	
	// Convert to glm::vec3 format
	pixels.resize(width * height);
	for (uint32_t i = 0; i < width * height; ++i) {
		float r = raw_pixels[i * channels + 0];
		float g = (channels > 1) ? raw_pixels[i * channels + 1] : r;
		float b = (channels > 2) ? raw_pixels[i * channels + 2] : r;
		pixels[i] = glm::vec3(r, g, b);
	}
	
	filename = filepath;
	is_loaded = true;
	in->close();
	
	std::cout << "Successfully loaded HDR environment map!" << std::endl;
	return true;
}

glm::vec3 EnvironmentMap::sample(const glm::vec3& direction) const
{
	if (!is_loaded || pixels.empty()) {
		return glm::vec3(0.5f, 0.7f, 1.0f); // Default sky blue
	}
	
	// Convert direction vector to spherical coordinates
	// Assuming standard HDR environment mapping (equirectangular)
	float theta = atan2f(direction.z, direction.x) - glm::pi<float>() * 0.5f; // azimuth - 90°
	float phi = asinf(direction.y); // elevation
	
	return sample(theta, phi);
}

glm::vec3 EnvironmentMap::sample(float theta, float phi) const
{
	if (!is_loaded || pixels.empty()) {
		return glm::vec3(0.5f, 0.7f, 1.0f); // Default sky blue
	}
	
	// Convert spherical coordinates to texture coordinates
	// theta: [-π, π] -> [0, 1] but flip U to correct horizontal flip
	// phi: [-π/2, π/2] -> [0, 1] but flip V to correct upside-down image
	float u = 1.0f - (theta + glm::pi<float>()) / (2.0f * glm::pi<float>()); // Flip U coordinate
	float v = 1.0f - (phi + glm::pi<float>() * 0.5f) / glm::pi<float>(); // Flip V coordinate
	
	// Clamp to valid range
	u = glm::clamp(u, 0.0f, 1.0f);
	v = glm::clamp(v, 0.0f, 1.0f);
	
	// Convert to pixel coordinates
	uint32_t x = (uint32_t)(u * (width - 1));
	uint32_t y = (uint32_t)(v * (height - 1));
	
	// Sample the environment map
	uint32_t index = y * width + x;
	return pixels[index];
}

// Scene environment methods
bool Scene::load_environment_map(const std::string& filepath)
{
	return environment_map.load_from_file(filepath);
}

glm::vec3 Scene::sample_environment(const glm::vec3& direction) const
{
	return environment_map.sample(direction);
}
