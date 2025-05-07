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

#include "chunk.h"

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

    auto shader = Shaders {{
        {ShaderType::kVertexShader, _SHADER_scene_vert},
        {ShaderType::kFragmentShader, _SHADER_scene_frag}
    }};

    glEnable(GL_DEPTH_TEST);

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

    window.Start([&]([[maybe_unused]] const double _){
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.SetUniform("u_Projection", camera.Projection());

        std::print("Current LOD: {}\n", calculate_lod());

        for (auto& chunk : chunks) {
            if (chunk.State() == ChunkState::Loaded) {
                chunk.Texture().Bind();
                shader.SetUniform("u_ModelView", camera.View() * chunk.ModelMatrix());
                geometry.Draw(shader);
            }
        }
    });

    return 0;
}