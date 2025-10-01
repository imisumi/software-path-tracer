#pragma once

#include <variant>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

#include <glm/glm.hpp>

namespace render {
enum class MaterialType {
	DIFFUSE,
	// Future: METALLIC, GLASS, EMISSIVE, CUSTOM, etc.
};

using MaterialParam = std::variant<
	float,
	int,
	bool,
	glm::vec2,
	glm::vec3,
	glm::vec4,
	std::string>;

struct MaterialDescriptor
{
	using Handle = std::shared_ptr<MaterialDescriptor>;

	MaterialType type;
	std::string shaderPath;  // For OSL custom shaders (empty for built-in materials)
	std::unordered_map<std::string, MaterialParam> parameters;

	// Helper getters with defaults
	template<typename T>
	T get(const std::string& name, const T& defaultValue) const {
		auto it = parameters.find(name);
		if (it == parameters.end()) return defaultValue;

		if (auto* val = std::get_if<T>(&it->second)) {
			return *val;
		}
		return defaultValue;
	}

	// Required parameter (throws if missing)
	template<typename T>
	T getRequired(const std::string& name) const {
		auto it = parameters.find(name);
		if (it == parameters.end()) {
			throw std::runtime_error("Missing required parameter: " + name);
		}

		if (auto* val = std::get_if<T>(&it->second)) {
			return *val;
		}
		throw std::runtime_error("Wrong type for parameter: " + name);
	}

	// Hash for deduplication
	size_t hash() const {
		size_t h = std::hash<int>{}(static_cast<int>(type));
		h ^= std::hash<std::string>{}(shaderPath) << 1;

		// Hash parameters (order-independent)
		for (const auto& [key, value] : parameters) {
			size_t paramHash = std::hash<std::string>{}(key);

			// Hash the variant value
			std::visit([&paramHash](const auto& v) {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, float>) {
					paramHash ^= std::hash<float>{}(v);
				} else if constexpr (std::is_same_v<T, int>) {
					paramHash ^= std::hash<int>{}(v);
				} else if constexpr (std::is_same_v<T, bool>) {
					paramHash ^= std::hash<bool>{}(v);
				} else if constexpr (std::is_same_v<T, glm::vec2>) {
					paramHash ^= std::hash<float>{}(v.x) ^ std::hash<float>{}(v.y);
				} else if constexpr (std::is_same_v<T, glm::vec3>) {
					paramHash ^= std::hash<float>{}(v.x) ^ std::hash<float>{}(v.y) ^ std::hash<float>{}(v.z);
				} else if constexpr (std::is_same_v<T, glm::vec4>) {
					paramHash ^= std::hash<float>{}(v.x) ^ std::hash<float>{}(v.y) ^ std::hash<float>{}(v.z) ^ std::hash<float>{}(v.w);
				} else if constexpr (std::is_same_v<T, std::string>) {
					paramHash ^= std::hash<std::string>{}(v);
				}
			}, value);

			h ^= paramHash;
		}

		return h;
	}

	// Equality comparison for deduplication
	bool operator==(const MaterialDescriptor& other) const {
		return type == other.type &&
		       shaderPath == other.shaderPath &&
		       parameters == other.parameters;
	}

	// Validation
	// bool validate(std::string* errorMsg = nullptr) const;

	// Serialization
	// static MaterialDescriptor fromJson(const std::string& json);
	// std::string toJson() const;
};
}