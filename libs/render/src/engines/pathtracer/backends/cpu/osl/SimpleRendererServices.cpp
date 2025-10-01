#include "SimpleRendererServices.h"

bool OSLRenderer::init()
{
	// Create texture system first
        m_texture_system = OIIO::TextureSystem::create();
        
        // Create renderer services
        m_renderer_services = new SimpleRendererServices(m_texture_system.get());
        
        // Create shading system with renderer services
        m_shading_system = new OSL::ShadingSystem(m_renderer_services, m_texture_system.get());
        
        // Set OSL error reporting to verbose
        // m_shading_system->attribute("debug", 1);
        // m_shading_system->attribute("verbose", 1);
        
        // Tell OSL which outputs we want to use (prevents optimization from removing them)
        OSL::ustring outputs[] = { OSL::ustring("Cout") };
        m_shading_system->attribute("renderer_outputs", OSL::TypeDesc(OSL::TypeDesc::STRING, 1), &outputs);
        
        // Set shader search path
		m_shading_system->attribute("searchpath:shader", "C:/Users/ichir/Desktop/software-path-tracer/shaders");
        
        // Compile shader first
        OSL::OSLCompiler compiler;
        std::vector<std::string> options;
        options.push_back("-o");
        options.push_back("C:/Users/ichir/Desktop/software-path-tracer/shaders/simple_diffuse.oso");
        options.push_back("C:/Users/ichir/Desktop/software-path-tracer/shaders/simple_diffuse.osl");
        
        std::string stdosl_path = "C:/Users/ichir/Desktop/software-path-tracer/shaders/stdosl.h";
        
        if (!compiler.compile("C:/Users/ichir/Desktop/software-path-tracer/shaders/simple_diffuse.osl", options, stdosl_path)) {
            std::cerr << "Failed to compile shader!" << std::endl;
            return false;
        }
        
        std::cout << "Shader compiled successfully!" << std::endl;
        
        // Load compiled shader
        m_shader_group = m_shading_system->ShaderGroupBegin("material_group");
        bool shader_loaded = m_shading_system->Shader(*m_shader_group, "surface", "simple_diffuse", "simple_diffuse");
        if (!shader_loaded) {
            std::cerr << "Failed to load shader 'simple_diffuse'!" << std::endl;
            return false;
        }
        std::cout << "Shader loaded successfully!" << std::endl;
        
        // Set up interactive parameter for diffuse color
        OSL::Color3 initial_color(0.7f, 0.7f, 0.7f);
        m_shading_system->Parameter(*m_shader_group, "diffuse_color", 
                                   OSL::TypeDesc(OSL::TypeDesc::FLOAT, OSL::TypeDesc::VEC3), &initial_color,
                                   OSL::ParamHints::interactive);
        
        m_shading_system->ShaderGroupEnd(*m_shader_group);
        
        // Create persistent thread info and context for optimization and execution
        m_thread_info = m_shading_system->create_thread_info();
        m_context = m_shading_system->get_context(m_thread_info);
        
        // Optimize the shader group
        m_shading_system->optimize_group(m_shader_group.get(), m_context);
        std::cout << "Shader group optimized!" << std::endl;

		//			RGB
		// set_color(0.3f, 0.6f, 0.7f);
		set_color(0.7f, 0.7f, 0.7f);
        
        return true;
}

OSLRenderer::~OSLRenderer()
{
	if (m_context) {
		m_shading_system->release_context(m_context);
	}
	if (m_thread_info) {
		m_shading_system->destroy_thread_info(m_thread_info);
	}
	delete m_shading_system;
	delete m_renderer_services;
	OIIO::TextureSystem::destroy(m_texture_system);
}

glm::vec3 OSLRenderer::shade_hit(const glm::vec3& hit_point, const glm::vec3& normal)
{
	// Set up shader globals
	OSL::ShaderGlobals sg;
	memset(&sg, 0, sizeof(sg));
	
	// Set up transformation matrices (identity for simplicity)
	OSL::Matrix44 Mshad, Mobj;
	Mshad.makeIdentity();
	Mobj.makeIdentity();
	sg.shader2common = OSL::TransformationPtr(&Mshad);
	sg.object2common = OSL::TransformationPtr(&Mobj);
	sg.renderstate = nullptr;
	
	// Basic shader globals
	sg.P = OSL::Vec3(hit_point.x, hit_point.y, hit_point.z);
	sg.N = OSL::Vec3(normal.x, normal.y, normal.z);
	sg.Ng = sg.N;  // Geometric normal = shading normal
	sg.I = OSL::Vec3(0, 0, -1);  // Ray direction
	sg.u = 0.5f; // UV coordinates
	sg.v = 0.5f;
	sg.time = 0.0f;
	sg.dtime = 0.0f;
	sg.raytype = 0;  // default ray type
	sg.surfacearea = 1.0f;
	
	// Derivatives (required by OSL)
	sg.dudx = 0.001f;  sg.dudy = 0.0f;
	sg.dvdx = 0.0f;    sg.dvdy = 0.001f;
	sg.dPdx = OSL::Vec3(0.001f, 0, 0);
	sg.dPdy = OSL::Vec3(0, 0.001f, 0);
	sg.dPdz = OSL::Vec3(0, 0, 0.001f);
	sg.dPdu = OSL::Vec3(1, 0, 0);
	sg.dPdv = OSL::Vec3(0, 1, 0);
	sg.dPdtime = OSL::Vec3(0, 0, 0);
	sg.Ps = sg.P;
	
	// Execute shader using pre-created context
	// std::cout << "About to execute shader..." << std::endl;
	bool success = m_shading_system->execute(*m_context, *m_shader_group, sg);
	// std::cout << "Shader execution result: " << (success ? "SUCCESS" : "FAILED") << std::endl;
	
	glm::vec3 result_color(1, 0, 1); // Magenta for errors
	
	if (success) {
		// Get output color
		const OSL::ShaderSymbol* Cout_sym = m_shading_system->find_symbol(*m_shader_group, OSL::ustring("Cout"));
		if (Cout_sym) {
			const void* color_ptr = m_shading_system->symbol_address(*m_context, Cout_sym);
			if (color_ptr) {
				OSL::Color3 osl_color = *(OSL::Color3*)color_ptr;
				result_color = glm::vec3(osl_color.x, osl_color.y, osl_color.z);
			} else {
				std::cerr << "ERROR: Could not get symbol address for Cout" << std::endl;
			}
		} else {
			std::cerr << "ERROR: Could not find symbol Cout" << std::endl;
		}
	} else {
		std::cerr << "ERROR: Shader execution failed" << std::endl;
	}
	
	return result_color;
}

void OSLRenderer::set_color(float r, float g, float b) {
    OSL::Color3 new_color(r, g, b);
    m_shading_system->ReParameter(*m_shader_group, "simple_diffuse", 
                                 "diffuse_color", OSL::TypeDesc(OSL::TypeDesc::FLOAT, OSL::TypeDesc::VEC3), &new_color);
}