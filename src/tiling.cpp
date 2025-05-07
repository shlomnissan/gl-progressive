// Copyright © 2024 - Present, Shlomi Nissan.
// All rights reserved.

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
    const auto win_width = 1024;
    const auto win_height = 768;
    const auto aspect = static_cast<float>(win_width) / win_height;
    const auto view_width = 2048.0f; // 2048 x 2048 virtual units
    const auto view_height = view_width / aspect;

    auto window = Window {win_width, win_height, "Tiling"};
    auto camera = OrthographicCamera {0.0f, view_width, view_height, 0.0f, -1.0f, 1.0f};
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

    for (auto& chunk : chunks) {
        chunk.Load();
    }

    window.Start([&]([[maybe_unused]] const double _){
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.SetUniform("u_Projection", camera.Projection());

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