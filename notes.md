⏺ Perfect! Based on my analysis of your codebase and the excellent design document, here's your minimal-risk migration strategy
  in very small steps:

  📊 Current Architecture Analysis

  Your codebase has a classic monolithic structure:
  - App.cpp/h: Mixed UI (SDL/ImGui) and scene management
  - Scene.cpp/h: Tightly coupled to Embree (RTCDevice, RTCScene)
  - EmbreeRenderTarget: Contains both ray tracing logic AND display management
  - Renderer: Static class that coordinates everything
  - Everything builds as single executable

  🎯 Ultra-Small-Step Migration Plan

  Phase 1: Foundation (Week 1) - Zero Breaking Changes

⏺ Step 1A: Create Directory Structure (5 minutes)

  mkdir -p libs/pathtracer/include/pathtracer
  mkdir -p libs/pathtracer/src
  mkdir -p apps/editor/src
  mkdir -p apps/editor/include

  Step 1B: Extract Pure Data Structures First

  Start with the safest extraction - pure data with no dependencies:

  // libs/pathtracer/include/pathtracer/Types.h
  #pragma once
  #include <glm/glm.hpp>
  #include <vector>

  namespace pathtracer {
      struct RenderSettings {
          uint32_t width = 512;
          uint32_t height = 512;
          uint32_t samples = 64;
          uint32_t max_bounces = 8;
          float exposure = 1.0f;
          bool auto_exposure = false;
      };

      struct RenderResult {
          std::vector<glm::vec4> pixels;
          uint32_t width, height;
          uint32_t frame_number;
      };
  }

  Step 1C: Create Minimal Backend Interface

  // libs/pathtracer/include/pathtracer/RenderEngine.h  
  #pragma once
  #include "Types.h"

  namespace pathtracer {
      class RenderEngine {
      public:
          virtual ~RenderEngine() = default;
          virtual RenderResult render(const RenderSettings& settings) = 0;
          virtual std::string getBackendName() const = 0;
      };
  }

  Step 1D: Test Each Step

  After each micro-step:
  # Compile test - should still work exactly the same
  cmake --build build
  ./build/bin/MyApp  # Should work identically

  Phase 2: Gradual Extraction (Week 2)

  Step 2A: Move Scene Data Only

  // libs/pathtracer/include/pathtracer/SceneData.h
  #pragma once
  #include <glm/glm.hpp>
  #include <vector>

  namespace pathtracer {
      struct SphereData {
          std::vector<float> cx, cy, cz, radii;
          std::vector<uint32_t> material_indices;
          // ... exactly what you have now, but in namespace
      };

      struct EnvironmentMapData {
          std::vector<glm::vec3> pixels;
          uint32_t width, height;
          // ... exactly what you have now
      };
  }

  Step 2B: Create Scene Interface

  // libs/pathtracer/include/pathtracer/Scene.h
  #pragma once
  #include "SceneData.h"

  namespace pathtracer {
      class Scene {
      public:
          virtual ~Scene() = default;
          virtual void addSphere(glm::vec3 center, float radius) = 0;
          virtual void removeSphere(uint32_t index) = 0;
          virtual const SphereData& getSphereData() const = 0;
          virtual bool loadEnvironmentMap(const std::string& path) = 0;
      };
  }

  Step 2C: Keep Your Current Scene

  Your current Scene class becomes EmbreeScene and implements the interface:
  // src/scene/EmbreeScene.h (rename from Scene.h)
  #include <pathtracer/Scene.h>

  class EmbreeScene : public pathtracer::Scene {
      // All your current code stays exactly the same!
      // Just implement the interface methods
  };

  Phase 3: Backend Separation (Week 3)

  Step 3A: Extract Embree Backend

  // libs/pathtracer/src/backends/EmbreeBackend.cpp
  #include <pathtracer/RenderEngine.h>
  #include "../../src/renderer/embree/EmbreeRenderTarget.h" // temporary

  class EmbreeBackend : public pathtracer::RenderEngine {
  public:
      RenderResult render(const RenderSettings& settings) override {
          // Move logic from EmbreeRenderTarget::render here
          // Keep display code separate
      }
  };

  Testing Strategy for Each Step

  Automated Testing

  // tests/test_migration.cpp
  TEST(Migration, SceneDataExtraction) {
      // Test that old and new scene data are identical
      auto old_scene = std::make_unique<Scene>();
      auto new_scene = std::make_unique<pathtracer::EmbreeScene>();

      old_scene->add_sphere({0,0,0}, 1.0f);
      new_scene->addSphere({0,0,0}, 1.0f);

      EXPECT_EQ(old_scene->get_sphere_data().size(),
                new_scene->getSphereData().size());
  }

  TEST(Migration, RenderingResults) {
      // Test that old and new rendering produce identical output
      auto old_target = std::make_unique<EmbreeRenderTarget>(64, 64);
      auto new_backend = std::make_unique<EmbreeBackend>();

      // Render same scene, compare pixel-perfect results
      // This catches any regression immediately
  }

  Visual Testing

  // Visual regression test
  void save_reference_image(const std::string& test_name) {
      // Render with current system, save as reference
  }

  void compare_with_reference(const std::string& test_name) {
      // Render with new system, compare pixels
      // Fail if any pixel differs by more than threshold
  }

  Application Structure Recommendations

  Recommended Final Structure

  software-path-tracer/
  ├── libs/pathtracer/           # Core library
  │   ├── include/pathtracer/    # Public API headers
  │   │   ├── RenderEngine.h     # Backend interface
  │   │   ├── Scene.h           # Scene interface
  │   │   ├── Types.h           # Common data structures
  │   │   └── PostProcess.h     # Post-processing interface
  │   └── src/
  │       ├── backends/         # Backend implementations
  │       │   ├── embree/       # Embree-specific code
  │       │   ├── optix/        # Future NVIDIA backend
  │       │   └── metal/        # Future Apple backend
  │       └── postprocess/      # Post-processing effects
  ├── apps/
  │   ├── editor/              # Your current GUI app
  │   │   ├── src/
  │   │   │   ├── main.cpp
  │   │   │   ├── EditorApp.cpp  # Renamed from App.cpp
  │   │   │   └── ui/           # UI-specific code
  │   │   └── include/
  │   └── cli/                 # Future CLI tool
  └── tests/                   # Unit & integration tests

  Key Design Principles

  1. Incremental Safety: Each step compiles and runs identically
  2. Backwards Compatibility: Old code works until fully migrated
  3. Testable: Every step has automated verification
  4. Reversible: Can roll back any step if issues arise

  Migration Success Criteria

  ✅ After each step:
  - Code compiles without warnings
  - Application runs identically to before
  - All existing functionality preserved
  - New tests pass

  ✅ Final state:
  - Clean library/application separation
  - Multiple backend support ready
  - CLI tool can be added easily
  - Post-processing pipeline extensible

⏺ Next Immediate Steps

  1. Start with Step 1A - just create the directories
  2. Create the basic Types.h with your existing data structures
  3. Add simple CMake library target (even if empty initially)
  4. Test compilation - should work exactly the same

  This approach gives you:
  - Zero risk of breaking existing functionality
  - Immediate testing after each tiny step
  - Easy rollback if any step causes issues
  - Clear path toward the full architecture in your design doc

  The key is starting with data structures and interfaces only, then gradually moving implementation. Each step should take 15-30
   minutes max and be immediately testable.

  Would you like me to help you implement Step 1A first - just creating the directory structure and basic CMake setup?
