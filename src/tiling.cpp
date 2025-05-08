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

    auto window = Window {win_width, win_height, "Tiling"};
    auto camera = OrthographicCamera {0.0f, camera_width, camera_height, 0.0f, -1.0f, 1.0f};
    auto controls = ZoomPanCamera {&camera};
    auto image_loader = ImageLoader::Create();
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

    auto chunks = std::vector<Chunk> {};
    for (auto i = 0; i < 16; ++i) {
        auto x = i % 4;
        auto y = i / 4;
        chunks.emplace_back(Chunk::Params {
            .lod = 0,
            .grid_index = {x, y},
            .position = {static_cast<float>(x) * 512.0f, static_cast<float>(y) * 512.0f},
            .size = {512.0f, 512.0f}
        }, std::format("assets/lod_0/spiralcrop0_{:02}.jpg", i + 1));
    }

    for (auto& chunk : chunks) chunk.Load();

    const auto calculate_lod = [&]() {
        const auto virtual_width = camera.Width() / glm::length(glm::vec3{camera.transform[0]});
        const auto virtual_units_per_screen_pixel =  virtual_width / win_width;
        const auto lod_shift = static_cast<float>(lods);
        const float lod_f = lod_shift + std::log2(1 / virtual_units_per_screen_pixel);
        return std::clamp(static_cast<int>(std::floor(lod_f)), 0, lods - 1);
    };

    const auto compute_visible_bounds = [&]() {
        const auto top_left_ndc = glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f);
        const auto bottom_right_ndc = glm::vec4( 1.0f, -1.0f, 0.0f, 1.0f);

        const auto inv_vp = glm::inverse(camera.Projection() * camera.View());

        const auto top_left_world = inv_vp * top_left_ndc;
        const auto bottom_right_world = inv_vp * bottom_right_ndc;

        return Bounds {
            .min = {top_left_world.x, top_left_world.y},
            .max = {bottom_right_world.x, bottom_right_world.y}
        };
    };

    const auto draw_debugger = [&]() {
        const auto visible_bounds = compute_visible_bounds();
        const auto lod = calculate_lod();

        ImGui::Begin("Debugger");
        ImGui::Text("Level of detail: %d", lod);
        ImGui::Separator();
        ImGui::Text("Visible bounds:");
        ImGui::Text("min [%.2f, %.2f]", visible_bounds.min.x, visible_bounds.min.y);
        ImGui::Text("max [%.2f, %.2f]", visible_bounds.max.x, visible_bounds.max.y);
        ImGui::Separator();
        ImGui::End();
    };

    window.Start([&]([[maybe_unused]] const double _){
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_debugger();

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