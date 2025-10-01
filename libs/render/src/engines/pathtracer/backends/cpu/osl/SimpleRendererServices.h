#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <string_view>
#include <OSL/oslcomp.h>
#include <OSL/oslexec.h>
#include <OSL/rendererservices.h>
#include <glm/glm.hpp>
#include <OpenImageIO/texture.h>

class SimpleRendererServices : public OSL::RendererServices
{
public:
	SimpleRendererServices(OIIO::TextureSystem* texsys = nullptr) 
		: OSL::RendererServices(texsys) {}

private:
};

class OSLRenderer {
private:
    OSL::ShadingSystem* m_shading_system;
    OSL::ShaderGroupRef m_shader_group;
    SimpleRendererServices* m_renderer_services;
    std::shared_ptr<OIIO::TextureSystem> m_texture_system;
    OSL::PerThreadInfo* m_thread_info;
    OSL::ShadingContext* m_context;
    
public:
    bool init();
    
    ~OSLRenderer();
    
    glm::vec3 shade_hit(const glm::vec3& hit_point, const glm::vec3& normal);

	void set_color(float r, float g, float b);
};