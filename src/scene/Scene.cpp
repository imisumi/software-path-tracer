#include "Scene.h"
#include <limits>

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::add_sphere(glm::vec3 center, float radius)
{
	sphere_data.add_sphere(center, radius, 0);
}

void Scene::update_sphere(uint32_t index, glm::vec3 center, float radius, uint32_t material_index)

{
	sphere_data.update_sphere(index, center, radius, material_index);
}
