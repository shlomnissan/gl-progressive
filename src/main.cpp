// Copyright © 2024 - Present, Shlomi Nissan.
// All rights reserved.

#include <print>
#include <vector>

#include "core/orthographic_camera.h"
#include "core/shaders.h"
#include "core/window.h"
#include "geometries/plane_geometry.h"
#include "resources/zoom_pan_camera.h"

#include "shaders/headers/scene_frag.h"
#include "shaders/headers/scene_vert.h"
#include "shaders/headers/line_vert.h"
#include "shaders/headers/line_frag.h"

#include "chunk.h"
#include "chunk_manager.h"

#include <imgui.h>

struct Bounds {
    glm::vec2 min {0.0f};
    glm::vec2 max {0.0f};
};

auto main() -> int {
    constexpr auto win_width = 1024;
    constexpr auto win_height = 1024;
    constexpr auto aspect = static_cast<float>(win_width) / win_height;
    constexpr auto camera_width = 2048.0f; // 2048 x 2048 virtual units
    constexpr auto camera_height = camera_width / aspect;
    constexpr auto lods = 3;

    auto chunk_manager = ChunkManager {{
        .image_dims = {2048, 2048},
        .window_dims = {win_width, win_height},
        .lods = lods
    }};

    auto window = Window {win_width, win_height, "Tiling"};
    auto camera = OrthographicCamera {0.0f, camera_width, camera_height, 0.0f, -1.0f, 1.0f};
    auto controls = ZoomPanCamera {&camera};
    auto geometry = PlaneGeometry {{
        .width = 512.0f,
        .height = 512.0f,
        .width_segments = 1,
        .height_segments = 1
    }};

    auto shader_tile = Shaders {{
        {ShaderType::kVertexShader, _SHADER_scene_vert},
        {ShaderType::kFragmentShader, _SHADER_scene_frag}
    }};

    auto shader_line = Shaders {{
        {ShaderType::kVertexShader, _SHADER_line_vert},
        {ShaderType::kFragmentShader, _SHADER_line_frag}
    }};

    window.Start([&]([[maybe_unused]] const double _){
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        chunk_manager.Update(camera);
        chunk_manager.Debug();

        auto& chunks = chunk_manager.GetVisibleChunks();

        // draw tiles

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_BLEND);

        shader_tile.Use();
        shader_tile.SetUniform("u_Projection", camera.Projection());

        for (auto& chunk : chunks) {
            if (chunk.State() == ChunkState::Loaded) {
                chunk.Texture().Bind();
                shader_tile.SetUniform("u_ModelView", camera.View() * chunk.ModelMatrix());
                geometry.Draw(shader_tile);
            }
        }

        // draw wireframes

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        shader_line.Use();
        shader_line.SetUniform("u_Projection", camera.Projection());
        for (const auto& chunk : chunks) {
            shader_line.SetUniform("u_ModelView", camera.View() * chunk.ModelMatrix());
            geometry.Draw(shader_line);
        }
    });

    return 0;
}