#pragma once

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "materials/material.h"
#include "geometry/sphere_data.h"

typedef struct RTCDeviceTy *RTCDevice;
typedef struct RTCSceneTy *RTCScene;
typedef struct RTCGeometryTy *RTCGeometry;

// HDR Environment Map data
struct EnvironmentMap
{
	std::vector<glm::vec3> pixels;  // HDR pixel data (RGB float)
	uint32_t width = 0;
	uint32_t height = 0;
	std::string filename;
	bool is_loaded = false;
	
	// Get environment color for a direction (theta, phi or direction vector)
	glm::vec3 sample(const glm::vec3& direction) const;
	glm::vec3 sample(float theta, float phi) const;
	
	// Utility functions
	void clear();
	bool load_from_file(const std::string& filepath);
};

class Scene
{
public:
	Scene();
	~Scene();

	// Sphere management
	void add_sphere(glm::vec3 center, float radius);
	void remove_sphere(uint32_t index);
	void update_sphere(uint32_t index, glm::vec3 center, float radius, uint32_t material_index);
	
	// Embree integration
	void init_embree();
	void cleanup_embree();
	void rebuild_embree_scene();

	// Environment map
	bool load_environment_map(const std::string& filepath);
	const EnvironmentMap& get_environment_map() const { return environment_map; }
	glm::vec3 sample_environment(const glm::vec3& direction) const;

	const SphereData &get_sphere_data() const { return sphere_data; }

	bool debug_normals = false;

	RTCDevice device = nullptr;
	RTCScene scene = nullptr;

private:
	SphereData sphere_data;
	EnvironmentMap environment_map;
	RTCGeometry sphere_geometry = nullptr;
	uint32_t embree_geometry_id = 0;
	
	void update_embree_geometry();
};