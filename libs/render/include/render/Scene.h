#pragma once

#include "Types.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <memory>
#include <cstring>
#include <string>
#include <unordered_map>
#include <iostream>

namespace render
{
	
	#if 1
	using NodeID = uint32_t;

	enum class NodeType {
		SCENE_ROOT,
		SPHERE_OBJECT,
		MATERIAL,
		GROUP
	};

	struct Transform {
		glm::vec3 position = glm::vec3(0.0f);
		glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 scale = glm::vec3(1.0f);
		
		inline glm::mat4 ToMatrix() const
		{
			glm::mat4 mat = glm::mat4(1.0f);
			mat = glm::translate(mat, position);
			mat *= glm::mat4_cast(rotation);
			mat = glm::scale(mat, scale);
			return mat;
		}

		static inline Transform FromMatrix(const glm::mat4& matrix)
		{
			Transform transform;
			transform.position = glm::vec3(matrix[3]);
			transform.rotation = glm::quat_cast(matrix);
			transform.scale = glm::vec3(glm::length(matrix[0]), glm::length(matrix[1]), glm::length(matrix[2]));
			return transform;
		}

		Transform operator*(const Transform& other) const
		{
			Transform result;
			result.position = position + other.position;
			result.rotation = rotation * other.rotation;
			result.scale = scale * other.scale;
			return result;
		}
	};

	class SceneNode {
	protected:
		static NodeID s_nextID;
		NodeID m_id;
		std::string m_name;
		NodeType m_type;
		
		// Hierarchy
		// SceneNode* m_parent = nullptr;
		// std::vector<std::unique_ptr<SceneNode>> m_children;
		
		// Transform
		Transform m_localTransform;
		mutable Transform m_worldTransform;
		mutable bool m_worldTransformDirty = true;
		
		// Visibility
		// bool m_visible = true;
		// bool m_castsShadows = true;
		// bool m_receivesShadows = true;
		
	public:
		SceneNode(NodeType type, const std::string& name = "Node")
			: m_id(s_nextID++), m_name(name), m_type(type), m_localTransform(), m_worldTransform(), m_worldTransformDirty(true)
		{}
		virtual ~SceneNode() = default;
		
		// Identity
		NodeID GetID() const { return m_id; }
		const std::string& GetName() const { return m_name; }
		void SetName(const std::string& name) { m_name = name; }
		NodeType GetType() const { return m_type; }
		
		// Hierarchy
		// void AddChild(std::unique_ptr<SceneNode> child);
		// void RemoveChild(NodeID childID);
		// SceneNode* FindChild(NodeID id);
		// SceneNode* GetParent() const { return m_parent; }
		// const std::vector<std::unique_ptr<SceneNode>>& GetChildren() const { return m_children; }
		
		// Transform
		// const Transform& GetLocalTransform() const { return m_localTransform; }
		// const Transform& GetWorldTransform() const;
		// void SetLocalTransform(const Transform& transform);
		void SetPosition(const glm::vec3& position)
		{
			m_localTransform.position = position;
			// MarkWorldTransformDirty();
			m_worldTransformDirty = true;
		}
		// void SetRotation(const glm::vec3& rotation); // Euler angles
		// void SetScale(const glm::vec3& scale);
		glm::vec3 GetPosition() const
		{
			return m_localTransform.position;
		}
		// glm::vec3 GetRotation() const;
		// glm::vec3 GetScale() const;

	protected:
		// void MarkWorldTransformDirty();
		// void UpdateWorldTransform() const;
	};

	class SphereObject : public SceneNode {
	private:
		float m_radius = 1.0f;
		
	public:
		SphereObject(const std::string& name = "Sphere") 
			: SceneNode(NodeType::SPHERE_OBJECT, name) {}
		
		float GetRadius() const { return m_radius; }
		void SetRadius(float radius) { m_radius = radius; }
	};

	class Scene
	{
	private:
		// Scene hierarchy
		std::unique_ptr<SceneNode> m_rootNode;
		std::unordered_map<NodeID, SceneNode*> m_nodeRegistry;
		std::vector<std::unique_ptr<SceneNode>> m_nodes; // Store actual node objects
		

		bool m_has_changes = true;
		
	public:
		Scene()
		: m_rootNode(std::make_unique<SceneNode>(NodeType::SCENE_ROOT, "Root"))
		{}
		~Scene()
		{
			// Cleanup all nodes - unique_ptrs will handle deletion automatically
			m_nodeRegistry.clear();
			m_nodes.clear();
		}
		
		// Node Management
		SceneNode* GetRootNode() const { return m_rootNode.get(); }

		const std::unordered_map<NodeID, SceneNode*>& GetAllNodes() const { return m_nodeRegistry; }

		template<typename T, typename... Args>
		T* CreateNode(Args&&... args)
		{
			static_assert(std::is_base_of<SceneNode, T>::value, "T must be derived from SceneNode");
			auto node = std::make_unique<T>(std::forward<Args>(args)...);
			T* nodePtr = node.get();
			RegisterNode(nodePtr);
			m_nodes.push_back(std::move(node)); // Store the actual node object
			// For simplicity, we are not handling hierarchy here
			std::cout << "Created node ID: " << nodePtr->GetID() << ", Name: " << nodePtr->GetName() << std::endl;
			return nodePtr;
		}
		
		bool DeleteNode(NodeID id)
		{
			auto it = m_nodeRegistry.find(id);
			if (it != m_nodeRegistry.end())
			{
				SceneNode* node = it->second;
				UnregisterNode(id);
				// Remove from storage vector
				auto nodeIt = std::find_if(m_nodes.begin(), m_nodes.end(), 
					[node](const std::unique_ptr<SceneNode>& ptr) { return ptr.get() == node; });
				if (nodeIt != m_nodes.end()) {
					m_nodes.erase(nodeIt);
				}
				return true;
			}
			return false;
		}
		SceneNode* FindNode(NodeID id)
		{
			auto it = m_nodeRegistry.find(id);
			return (it != m_nodeRegistry.end()) ? it->second : nullptr;
		}
		SceneNode* FindNode(const std::string& name)
		{
			for (const auto& [id, node] : m_nodeRegistry)
			{
				if (node->GetName() == name)
					return node;
			}
			return nullptr;
		}

		bool hasChanges() const
		{
			return m_has_changes;
		}

		void markChangesProcessed()
		{
			m_has_changes = false;
		}

	
	private:
		void RegisterNode(SceneNode* node)
		{
			m_nodeRegistry[node->GetID()] = node;
		}
		void UnregisterNode(NodeID id)
		{
			m_nodeRegistry.erase(id);
		}
	};

#elif

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
#endif
}
