#include "sphere_data.h"

void SphereData::add_sphere(glm::vec3 center, float radius, uint32_t material_index)
{
	cx.push_back(center.x);
	cy.push_back(center.y);
	cz.push_back(center.z);
	radii.push_back(radius);
	material_indices.push_back(material_index);
}

void SphereData::remove_sphere(uint32_t index)
{
	if (index < cx.size())
	{
		cx.erase(cx.begin() + index);
		cy.erase(cy.begin() + index);
		cz.erase(cz.begin() + index);
		radii.erase(radii.begin() + index);
		material_indices.erase(material_indices.begin() + index);
	}
}

void SphereData::clear()
{
	cx.clear();
	cy.clear();
	cz.clear();
	radii.clear();
	material_indices.clear();
}

void SphereData::reserve(size_t count)
{
	cx.reserve(count);
	cy.reserve(count);
	cz.reserve(count);
	radii.reserve(count);
	material_indices.reserve(count);
}

void SphereData::update_sphere(uint32_t index, glm::vec3 center, float radius, uint32_t material_index)
{
	if (index < cx.size())
	{
		cx[index] = center.x;
		cy[index] = center.y;
		cz[index] = center.z;
		radii[index] = radius;
		material_indices[index] = material_index;
	}
}
