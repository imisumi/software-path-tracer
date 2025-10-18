#pragma once

#include "Types.h"
#include "../src/MaterialLibrary.h"
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

		// Transform
		Transform m_localTransform;
		mutable Transform m_worldTransform;
		mutable bool m_worldTransformDirty = true;

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

		// Transform
		void SetPosition(const glm::vec3& position)
		{
			m_localTransform.position = position;
			m_worldTransformDirty = true;
		}

		glm::vec3 GetPosition() const
		{
			return m_localTransform.position;
		}
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

		// Material management
		MaterialLibrary m_materialLibrary;
		std::unordered_map<NodeID, MaterialDescriptor::Handle> m_nodeMaterials;

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
			m_nodeMaterials.clear();
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

		/// Manually mark scene as having changes (e.g., when materials are edited externally)
		void markDirty()
		{
			m_has_changes = true;
		}

		// Material Management
		/// Set material for a node - automatically deduplicates identical materials
		void setMaterial(NodeID nodeId, const MaterialDescriptor& desc)
		{
			m_nodeMaterials[nodeId] = m_materialLibrary.getOrCreate(desc);
			m_has_changes = true;
		}

		/// Get material for a node (returns nullptr if no material set)
		MaterialDescriptor::Handle getMaterial(NodeID nodeId) const
		{
			auto it = m_nodeMaterials.find(nodeId);
			return (it != m_nodeMaterials.end()) ? it->second : nullptr;
		}

		/// Remove material from a node
		void removeMaterial(NodeID nodeId)
		{
			m_nodeMaterials.erase(nodeId);
			m_has_changes = true;
		}

		/// Register a named material for easy reuse
		void registerNamedMaterial(const std::string& name, const MaterialDescriptor& desc)
		{
			m_materialLibrary.registerNamed(name, desc);
		}

		/// Get a named material
		MaterialDescriptor::Handle getNamedMaterial(const std::string& name) const
		{
			return m_materialLibrary.get(name);
		}

		/// Check if a named material exists
		bool hasNamedMaterial(const std::string& name) const
		{
			return m_materialLibrary.hasNamed(name);
		}

		/// Get material library for advanced usage
		MaterialLibrary& getMaterialLibrary() { return m_materialLibrary; }
		const MaterialLibrary& getMaterialLibrary() const { return m_materialLibrary; }


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

}
