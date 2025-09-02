#include "sphere_data.h"

void SphereData::add_sphere(glm::vec3 center, float radius, uint32_t material_index)
{
	centers.push_back(center);
	radii.push_back(radius);
	material_indices.push_back(material_index);
}

void SphereData::remove_sphere(uint32_t index)
{
	if (index < centers.size())
	{
		centers.erase(centers.begin() + index);
		radii.erase(radii.begin() + index);
		material_indices.erase(material_indices.begin() + index);
	}
}

void SphereData::clear()
{
	centers.clear();
	radii.clear();
	material_indices.clear();
}

void SphereData::reserve(size_t count)
{
	centers.reserve(count);
	radii.reserve(count);
	material_indices.reserve(count);
}
