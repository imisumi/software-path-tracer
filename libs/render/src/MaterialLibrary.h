#pragma once

#include "Material.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <optional>

namespace render {

/// Manages material instances with automatic deduplication
/// Materials with identical properties share the same instance
class MaterialLibrary {
public:
	MaterialLibrary() = default;
	~MaterialLibrary() = default;

	// Non-copyable but movable
	MaterialLibrary(const MaterialLibrary&) = delete;
	MaterialLibrary& operator=(const MaterialLibrary&) = delete;
	MaterialLibrary(MaterialLibrary&&) = default;
	MaterialLibrary& operator=(MaterialLibrary&&) = default;

	/// Get or create a material - automatically deduplicates
	/// If an identical material exists, returns the existing instance
	MaterialDescriptor::Handle getOrCreate(const MaterialDescriptor& desc);

	/// Get a named material by name
	/// Returns nullptr if not found
	MaterialDescriptor::Handle get(const std::string& name) const;

	/// Register a material with a name for easy reuse
	/// If a material with the same properties exists, it will be reused
	void registerNamed(const std::string& name, const MaterialDescriptor& desc);

	/// Check if a named material exists
	bool hasNamed(const std::string& name) const;

	/// Remove a named material reference (does not delete the material if still in use)
	void removeNamed(const std::string& name);

	/// Get all named material names
	std::vector<std::string> getNamedMaterials() const;

	/// Get statistics
	size_t getMaterialCount() const { return m_materials.size(); }
	size_t getNamedCount() const { return m_namedMaterials.size(); }

	/// Clear all materials (be careful - invalidates all handles)
	void clear();

	// Future: Serialization support
	// void loadFromFile(const std::string& path);
	// void saveToFile(const std::string& path) const;

private:
	// Hash-based storage for automatic deduplication
	std::unordered_map<size_t, MaterialDescriptor::Handle> m_materials;

	// Named material lookup (name -> hash)
	std::unordered_map<std::string, size_t> m_namedMaterials;
};

} // namespace render
