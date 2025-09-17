#pragma once

#include "Types.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace render
{

	/// Backend-agnostic scene representation
	/// Manages geometry, materials, lighting, and environment without backend coupling
	class Scene
	{
	public:
		Scene();
		~Scene();

		// Geometry management
		void addGeometry(/* geometry data */);
		void removeGeometry(uint32_t id);
		void updateGeometry(uint32_t id, const glm::mat4 &transform);

		// Environment and lighting
		void setEnvironmentMap(const std::string &filepath);
		void addLight(/* light data */);

		// Material management
		// uint32_t addMaterial(/* material data */);
		// void updateMaterial(uint32_t id, /* material data */);

		// Change tracking for backends to detect updates
		bool hasChanges() const;
		void markChangesProcessed();
	};

}