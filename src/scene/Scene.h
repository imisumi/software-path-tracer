#pragma once

#include <vector>

#include "materials/material.h"
#include "geometry/sphere_data.h"

class Scene
{
public:
	Scene();
	~Scene();

private:
	SphereData sphere_data;
};