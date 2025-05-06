// Copyright © 2024 - Present, Shlomi Nissan.
// All rights reserved.

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>

#include "core/orthographic_camera.h"
#include "core/shaders.h"
#include "core/texture2d.h"
#include "core/window.h"
#include "geometries/plane_geometry.h"
#include "loaders/image_loader.h"
#include "resources/zoom_pan_camera.h"
#include "shaders/headers/scene_frag.h"
#include "shaders/headers/scene_vert.h"

auto main() -> int {
    const auto win_width = 1024;
    const auto win_height = 768;
    auto window = Window {win_width, win_height, "Tiling"};
    auto camera = OrthographicCamera {0.0f, win_width, win_height, 0.0f, -1.0f, 1.0f};
    auto controls = ZoomPanCamera {&camera};
    auto image_loader = ImageLoader::Create();
    auto texture = Texture2D {};
    auto geometry = PlaneGeometry {{
        .width = 512.0f,
        .height = 512.0f,
        .width_segments = 2,
        .height_segments = 2
    }};

    auto shader = Shaders {{
        {ShaderType::kVertexShader, _SHADER_scene_vert},
        {ShaderType::kFragmentShader, _SHADER_scene_frag}
    }};

    glEnable(GL_DEPTH_TEST);

    image_loader->LoadAsync("assets/spiralcrop_lod2.jpg", [&](const auto& image) {
        texture.SetImage(image.value());
    });

    window.Start([&]([[maybe_unused]] const double _){
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui::Begin("Hello, ImGui!");
        ImGui::Text("Hello, world!");
        ImGui::End();

        auto model = glm::mat4{1.0f};
        model = glm::translate(model, glm::vec3 {
            win_width >> 1,
            win_height >> 1,
            0.0f
        });

        shader.SetUniform("u_Projection", camera.Projection());
        shader.SetUniform("u_ModelView", camera.View() * model);

        if (texture.IsLoaded()) {
            texture.Bind();
            geometry.Draw(shader);
        }
    });

    return 0;
}