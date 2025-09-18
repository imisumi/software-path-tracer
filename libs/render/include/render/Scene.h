#pragma once

#include "Types.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <cstring>
#include <string>

namespace render
{

	/// Backend-agnostic scene representation
	/// Manages geometry, materials, lighting, and environment without backend coupling
	class Scene
	{
	public:
	enum class NodeType
	{
		Sphere,
		SkyBox
	};

	struct BaseObject
	{
		uint32_t id;
		glm::mat4 transform;
		
		// Hierarchy
		uint32_t parentId = 0;  // 0 = root node
		std::vector<uint32_t> childIds;
		
		BaseObject(uint32_t objectId) : id(objectId), transform(glm::mat4(1.0f)) {}
		virtual ~BaseObject() = default;
		
		virtual NodeType getType() const = 0;
	};

	struct SphereObject : public BaseObject
	{
		float radius;
		uint32_t materialId;
		
		SphereObject(uint32_t objectId, float sphereRadius, uint32_t material) 
			: BaseObject(objectId), radius(sphereRadius), materialId(material) {}
		
		NodeType getType() const override { return NodeType::Sphere; }
	};

	struct SkyBox : public BaseObject
	{
		std::vector<float> hdrData; // HDR image data
		int width = 0;
		int height = 0;

		SkyBox(const std::vector<float>& data, int w, int h)
			: BaseObject(0), hdrData(data), width(w), height(h) {}

		NodeType getType() const override { return NodeType::SkyBox; }
	};

	// Asset structures
	struct Material
	{
		uint32_t id;
		glm::vec3 albedo = glm::vec3(0.8f);
	};


	private:
		// Scene graph storage
		std::vector<std::unique_ptr<BaseObject>> m_objects;
		uint32_t m_nextObjectId = 1;

		// Asset storage
		std::vector<Material> m_materials;
		uint32_t m_nextMaterialId = 1;

		// Environment
		bool m_has_changes = true;

	public:
		Scene();
		~Scene();

		// Object management
		template<typename T, typename... Args>
		uint32_t createObject(Args&&... args);
		
		void removeObject(uint32_t id);
		BaseObject* getObject(uint32_t id);
		
		template<typename T>
		T* getObjectAs(uint32_t id);
		
		// Hierarchy management
		void setParent(uint32_t childId, uint32_t parentId);
		void removeParent(uint32_t childId);
		std::vector<uint32_t> getChildren(uint32_t parentId) const;
		
		// Convenience creators
		uint32_t addSphere(float radius, uint32_t materialId, const glm::mat4& transform = glm::mat4(1.0f));

		bool setEnvironmentMap(const std::string& filepath);

		// Material management
		uint32_t addMaterial(const glm::vec3& albedo = glm::vec3(0.8f));
		Material* getMaterial(uint32_t id);

		// Change tracking for backends to detect updates
		bool hasChanges() const;
		void markChangesProcessed();

		const std::vector<std::unique_ptr<BaseObject>>& getAllObjects() const { return m_objects; }
	};

	// Template method implementations
	template<typename T, typename... Args>
	uint32_t Scene::createObject(Args&&... args)
	{
		auto object = std::make_unique<T>(m_nextObjectId++, std::forward<Args>(args)...);
		uint32_t id = object->id;
		m_objects.push_back(std::move(object));
		return id;
	}

	template<typename T>
	T* Scene::getObjectAs(uint32_t id)
	{
		if (auto* obj = getObject(id)) {
			return dynamic_cast<T*>(obj);
		}
		return nullptr;
	}

}