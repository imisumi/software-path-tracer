#include "render/Scene.h"
#include <algorithm>

#include <OpenImageIO/imageio.h>

namespace render
{

Scene::Scene() = default;
Scene::~Scene() = default;

void Scene::removeObject(uint32_t id)
{
	auto it = std::find_if(m_objects.begin(), m_objects.end(),
		[id](const std::unique_ptr<BaseObject>& obj) { return obj->id == id; });
	
	if (it != m_objects.end()) {
		// Remove from parent's child list
		if ((*it)->parentId != 0) {
			removeParent(id);
		}
		
		// Remove children (they become orphaned)
		for (uint32_t childId : (*it)->childIds) {
			if (auto* child = getObject(childId)) {
				child->parentId = 0;
			}
		}
		
		m_objects.erase(it);
	}
}

Scene::BaseObject* Scene::getObject(uint32_t id)
{
	auto it = std::find_if(m_objects.begin(), m_objects.end(),
		[id](const std::unique_ptr<BaseObject>& obj) { return obj->id == id; });
	
	return (it != m_objects.end()) ? it->get() : nullptr;
}


void Scene::setParent(uint32_t childId, uint32_t parentId)
{
	auto* child = getObject(childId);
	auto* parent = getObject(parentId);
	
	if (!child || !parent) return;
	
	// Remove from old parent if exists
	if (child->parentId != 0) {
		removeParent(childId);
	}
	
	// Set new parent
	child->parentId = parentId;
	parent->childIds.push_back(childId);
}

void Scene::removeParent(uint32_t childId)
{
	auto* child = getObject(childId);
	if (!child || child->parentId == 0) return;
	
	auto* parent = getObject(child->parentId);
	if (parent) {
		auto& children = parent->childIds;
		children.erase(std::remove(children.begin(), children.end(), childId), children.end());
	}
	
	child->parentId = 0;
}

std::vector<uint32_t> Scene::getChildren(uint32_t parentId) const
{
	auto it = std::find_if(m_objects.begin(), m_objects.end(),
		[parentId](const std::unique_ptr<BaseObject>& obj) { return obj->id == parentId; });
	
	return (it != m_objects.end()) ? (*it)->childIds : std::vector<uint32_t>();
}

uint32_t Scene::addSphere(float radius, uint32_t materialId, const glm::mat4& transform)
{
	auto sphere = std::make_unique<SphereObject>(m_nextObjectId++, radius, materialId);
	sphere->transform = transform;
	uint32_t id = sphere->id;
	m_objects.push_back(std::move(sphere));
	return id;
}

uint32_t Scene::addMaterial(const glm::vec3& albedo)
{
	Material material;
	material.id = m_nextMaterialId++;
	material.albedo = albedo;
	m_materials.push_back(material);
	return material.id;
}

Scene::Material* Scene::getMaterial(uint32_t id)
{
	auto it = std::find_if(m_materials.begin(), m_materials.end(),
		[id](const Material& mat) { return mat.id == id; });
	
	return (it != m_materials.end()) ? &(*it) : nullptr;
}

bool Scene::hasChanges() const
{
	return m_has_changes;
}

void Scene::markChangesProcessed()
{
	// Implementation for change tracking
	m_has_changes = false;
}

bool Scene::setEnvironmentMap(const std::string& filepath)
{
	auto in = OIIO::ImageInput::open(filepath);
	if (!in) {
		std::cerr << "ERROR: Could not open " << filepath << " : " << OIIO::geterror() << std::endl;
		return false;
	}
	
	const OIIO::ImageSpec& spec = in->spec();
	int width = spec.width;
	int height = spec.height;
	int channels = spec.nchannels;
	
	std::cout << "Loading HDR environment: " << filepath << std::endl;
	std::cout << "  Resolution: " << width << "x" << height << std::endl;
	std::cout << "  Channels: " << channels << std::endl;
	std::cout << "  Format: " << spec.format.c_str() << std::endl;
	
	// Read as float RGB data
	std::vector<float> raw_pixels(width * height * channels);
	if (!in->read_image(0, 0, 0, channels, OIIO::TypeDesc::FLOAT, raw_pixels.data())) {
		std::cerr << "ERROR: Could not read pixel data from " << filepath << std::endl;
		in->close();
		return false;
	}
	
	// Convert to glm::vec3 format
	// std::vector<float> pixels;
	// pixels.resize(width * height);
	// for (uint32_t i = 0; i < width * height; ++i) {
	// 	float r = raw_pixels[i * channels + 0];
	// 	float g = (channels > 1) ? raw_pixels[i * channels + 1] : r;
	// 	float b = (channels > 2) ? raw_pixels[i * channels + 2] : r;
	// 	pixels[i] = glm::vec3(r, g, b);
	// }
	
	// filename = filepath;
	// is_loaded = true;
	in->close();
	
	std::cout << "Successfully loaded HDR environment map!" << std::endl;

	std::unique_ptr<SkyBox> skybox = std::make_unique<SkyBox>(raw_pixels, width, height);
	m_objects.push_back(std::move(skybox));
	return true;
}

} 