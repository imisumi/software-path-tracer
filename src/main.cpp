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

#include "App.h"

class SimpleRendererServices : public OSL::RendererServices {
public:
    SimpleRendererServices(OIIO::TextureSystem* texsys = nullptr) 
        : OSL::RendererServices(texsys) {}

    // Minimal required overrides - just return false/failure for now
    // virtual bool get_matrix(OSL::ShaderGlobals* sg, OSL::Matrix44& result,
    //                        OSL::TransformationPtr xform, float time) override {
    //     return false;
    // }
    
    // virtual bool get_matrix(OSL::ShaderGlobals* sg, OSL::Matrix44& result,
    //                        OSL::ustringhash from, float time) override {
    //     return false;
    // }
    
    // virtual bool get_matrix(OSL::ShaderGlobals* sg, OSL::Matrix44& result,
    //                        OSL::TransformationPtr xform) override {
    //     return false;
    // }
    
    // virtual bool get_matrix(OSL::ShaderGlobals* sg, OSL::Matrix44& result,
    //                        OSL::ustringhash from) override {
    //     return false;
    // }

    // virtual bool get_attribute(OSL::ShaderGlobals* sg, bool derivatives,
    //                           OSL::ustringhash object, OSL::TypeDesc type,
    //                           OSL::ustringhash name, void* val) override {
    //     return false;
    // }

    // virtual bool get_userdata(bool derivatives, OSL::ustringhash name, 
    //                          OSL::TypeDesc type, OSL::ShaderGlobals* sg, 
    //                          void* val) override {
    //     return false;
    // }
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
    bool init() {
        // Create texture system first
        m_texture_system = OIIO::TextureSystem::create();
        
        // Create renderer services
        m_renderer_services = new SimpleRendererServices(m_texture_system.get());
        
        // Create shading system with renderer services
        m_shading_system = new OSL::ShadingSystem(m_renderer_services, m_texture_system.get());
        
        // Set OSL error reporting to verbose
        m_shading_system->attribute("debug", 1);
        m_shading_system->attribute("verbose", 1);
        
        // Tell OSL which outputs we want to use (prevents optimization from removing them)
        OSL::ustring outputs[] = { OSL::ustring("Cout") };
        m_shading_system->attribute("renderer_outputs", OSL::TypeDesc(OSL::TypeDesc::STRING, 1), &outputs);
        
        // Set shader search path
		m_shading_system->attribute("searchpath:shader", "C:/Users/ichir/Desktop/software-path-tracer/shaders");
        
        // Compile shader first
        OSL::OSLCompiler compiler;
        std::vector<std::string> options;
        options.push_back("-o");
        options.push_back("C:/Users/ichir/Desktop/software-path-tracer/shaders/flat_red.oso");
        options.push_back("C:/Users/ichir/Desktop/software-path-tracer/shaders/flat_red.osl");
        
        std::string stdosl_path = "C:/Users/ichir/Desktop/software-path-tracer/shaders/stdosl.h";
        
        if (!compiler.compile("C:/Users/ichir/Desktop/software-path-tracer/shaders/flat_red.osl", options, stdosl_path)) {
            std::cerr << "Failed to compile shader!" << std::endl;
            return false;
        }
        
        std::cout << "Shader compiled successfully!" << std::endl;
        
        // Load compiled shader
        m_shader_group = m_shading_system->ShaderGroupBegin("diffuse_group");
        bool shader_loaded = m_shading_system->Shader(*m_shader_group, "surface", "flat_red", "flat_red");
        if (!shader_loaded) {
            std::cerr << "Failed to load shader 'flat_red'!" << std::endl;
            return false;
        }
        std::cout << "Shader loaded successfully!" << std::endl;
        m_shading_system->ShaderGroupEnd(*m_shader_group);
        
        // Create persistent thread info and context for optimization and execution
        m_thread_info = m_shading_system->create_thread_info();
        m_context = m_shading_system->get_context(m_thread_info);
        
        // Optimize the shader group
        m_shading_system->optimize_group(m_shader_group.get(), m_context);
        std::cout << "Shader group optimized!" << std::endl;
        
        return true;
    }
    
    ~OSLRenderer() {
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
    
    glm::vec3 shade_hit(const glm::vec3& hit_point, const glm::vec3& normal) {
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
        std::cout << "About to execute shader..." << std::endl;
        bool success = m_shading_system->execute(*m_context, *m_shader_group, sg);
        std::cout << "Shader execution result: " << (success ? "SUCCESS" : "FAILED") << std::endl;
        
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
};

int main() {
	App app;
	app.run();
	return 0;


    std::cout << "Simple OSL Shader Test" << std::endl;
	OSLRenderer osl_renderer;

	if (osl_renderer.init()) {
		std::cout << "OSL Renderer initialized successfully." << std::endl;
	} else {
		std::cout << "Failed to initialize OSL Renderer." << std::endl;
		return -1;
	}

	std::cout << "Created ShadingSystem" << std::endl;
	
	// Test shader evaluation
	std::cout << "\nTesting shader evaluation:" << std::endl;
	glm::vec3 test_point(0.0f, 0.0f, 0.0f);
	glm::vec3 test_normal(0.0f, 0.0f, 1.0f);
	
	glm::vec3 result = osl_renderer.shade_hit(test_point, test_normal);
	
	std::cout << "Input point: (" << test_point.x << ", " << test_point.y << ", " << test_point.z << ")" << std::endl;
	std::cout << "Input normal: (" << test_normal.x << ", " << test_normal.y << ", " << test_normal.z << ")" << std::endl;
	std::cout << "Shader output: (" << result.x << ", " << result.y << ", " << result.z << ")" << std::endl;
	
	// Expected: (1, 0, 0) for red color
	if (result.x == 1.0f && result.y == 0.0f && result.z == 0.0f) {
		std::cout << "SUCCESS: Shader returned expected red color!" << std::endl;
	} else {
		std::cout << "FAILURE: Expected (1, 0, 0), got (" << result.x << ", " << result.y << ", " << result.z << ")" << std::endl;
	}
	
	return 0;
}

// int main()
// {
// 	App app;
// 	app.run();
// 	return 0;
// }
