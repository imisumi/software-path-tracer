#include "MaterialLibrary.h"
#include <algorithm>

namespace render {

MaterialDescriptor::Handle MaterialLibrary::getOrCreate(const MaterialDescriptor& desc) {
	size_t hash = desc.hash();

	// Check if we already have this material
	auto it = m_materials.find(hash);
	if (it != m_materials.end()) {
		return it->second;
	}

	// Create new material instance
	auto material = std::make_shared<MaterialDescriptor>(desc);
	m_materials[hash] = material;
	return material;
}

MaterialDescriptor::Handle MaterialLibrary::get(const std::string& name) const {
	auto it = m_namedMaterials.find(name);
	if (it == m_namedMaterials.end()) {
		return nullptr;
	}

	// Look up the material by hash
	auto matIt = m_materials.find(it->second);
	if (matIt == m_materials.end()) {
		// This shouldn't happen, but handle gracefully
		return nullptr;
	}

	return matIt->second;
}

void MaterialLibrary::registerNamed(const std::string& name, const MaterialDescriptor& desc) {
	// Get or create the material (ensures deduplication)
	auto material = getOrCreate(desc);

	// Register the name
	m_namedMaterials[name] = desc.hash();
}

bool MaterialLibrary::hasNamed(const std::string& name) const {
	return m_namedMaterials.find(name) != m_namedMaterials.end();
}

void MaterialLibrary::removeNamed(const std::string& name) {
	m_namedMaterials.erase(name);
	// Note: We don't remove the material from m_materials
	// It may still be referenced by nodes or other names
}

std::vector<std::string> MaterialLibrary::getNamedMaterials() const {
	std::vector<std::string> names;
	names.reserve(m_namedMaterials.size());

	for (const auto& [name, hash] : m_namedMaterials) {
		names.push_back(name);
	}

	std::sort(names.begin(), names.end());
	return names;
}

void MaterialLibrary::clear() {
	m_materials.clear();
	m_namedMaterials.clear();
}

} // namespace render
