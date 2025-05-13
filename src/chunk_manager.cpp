// Copyright © 2024 - Present, Shlomi Nissan.
// All rights reserved.

#include "chunk_manager.h"

#include <format>

#include <imgui.h>

ChunkManager::ChunkManager(const Parameters& params) :
    image_dims_(params.image_dims),
    window_dims_(params.window_dims),
    lods_(params.lods),
    max_lod_(params.lods - 1)
{
    chunks_.resize(lods_);
    GenerateChunks();
}

auto ChunkManager::ComputeLod(const OrthographicCamera& camera) const -> int {
    const auto virtual_width = camera.Width() / glm::length(glm::vec3{camera.transform[0]});
    const auto virtual_units_per_screen_pixel =  virtual_width / window_dims_.width;
    const auto lod_shift = static_cast<float>(lods_) - 1;
    const float lod_f = lod_shift + std::log2(1 / virtual_units_per_screen_pixel);
    return std::clamp(static_cast<int>(std::floor(lod_f)), 0, max_lod_);
}

auto ChunkManager::Update(const OrthographicCamera& camera) -> void {
    auto this_lod = ComputeLod(camera);
    if (this_lod != curr_lod) {
        prev_lod = curr_lod;
        curr_lod = this_lod;
    }

    visible_bounds_ = ComputeVisibleBounds(camera);

    for (auto lod = 0; lod < lods_; ++lod) {
        for (auto& chunk : chunks_[lod]) {
            chunk.visible = IsChunkVisible(chunk);
            if (curr_lod == lod && chunk.visible && chunk.State() == ChunkState::Unloaded) {
                chunk.Load();
            }
        }
    }
};

auto ChunkManager::GenerateChunks() -> void {
    for (auto i = 0u; i < lods_; ++i) {
        const auto lod_width = static_cast<float>(image_dims_.width) / (1 << i);
        const auto lod_height = static_cast<float>(image_dims_.height) / (1 << i);
        const auto scale = static_cast<float>(pow(2, i));
        const auto grid_x = static_cast<int>(lod_width / kChunkSize);
        const auto grid_y = static_cast<int>(lod_height / kChunkSize);
        const auto n_chunks = grid_x * grid_y;

        for (auto j = 1; j <= n_chunks; ++j) {
            auto x = (j - 1) % grid_x;
            auto y = (j - 1) / grid_y;
            auto path = std::format("assets/lod_{}/spiralcrop{}_{:02}.jpg", i, i, j);
            chunks_[i].emplace_back(Chunk::Params {
                .grid_index = {x, y},
                .position = {x * kChunkSize * scale, y * kChunkSize * scale},
                .size = {kChunkSize * scale, kChunkSize * scale},
                .scale = scale,
                .lod = i
            }, path);
        }
    }

    for (auto& chunk : chunks_[max_lod_]) {
        chunk.Load();
    }
}

auto ChunkManager::GetVisibleChunks() -> std::vector<Chunk*> {
    std::vector<Chunk*> visible_chunks;

    // always include low-res tiles
    for (auto& chunk : chunks_[max_lod_]) {
        if (chunk.visible && chunk.State() == ChunkState::Loaded) {
            visible_chunks.push_back(&chunk);
        }
    }

    if (curr_lod == max_lod_) return visible_chunks;

    // collect all visible chunks from the current LOD
    bool all_loaded = true;
    for (auto& chunk : chunks_[curr_lod]) {
        if (chunk.visible) {
            visible_chunks.push_back(&chunk);
            if (chunk.State() != ChunkState::Loaded) {
                all_loaded = false;
            }
        }
    }

    // if not all chunks are loaded, add the previous LOD
    if (!all_loaded && curr_lod != max_lod_) {
        for (auto& chunk : chunks_[prev_lod]) {
            if (chunk.visible && chunk.State() == ChunkState::Loaded) {
                visible_chunks.push_back(&chunk);
            }
        }
    }

    if (all_loaded && prev_lod != curr_lod && prev_lod != max_lod_) {
        // TODO: release previous LOD memory
    }

    return visible_chunks;
}

auto ChunkManager::ComputeVisibleBounds(const OrthographicCamera& camera) const -> Bounds {
    const auto top_left_ndc = glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f);
    const auto bottom_right_ndc = glm::vec4( 1.0f, -1.0f, 0.0f, 1.0f);

    const auto inv_vp = glm::inverse(camera.Projection() * camera.View());

    const auto top_left_world = inv_vp * top_left_ndc;
    const auto bottom_right_world = inv_vp * bottom_right_ndc;

    return Bounds {
        .min = {top_left_world.x, top_left_world.y},
        .max = {bottom_right_world.x, bottom_right_world.y}
    };
}

auto ChunkManager::IsChunkVisible(const Chunk& chunk) const -> bool {
    const Bounds chunk_bounds = {
        .min = chunk.Position(),
        .max = chunk.Position() + chunk.Size()
    };

    const auto chunk_min = glm::min(chunk_bounds.min, chunk_bounds.max);
    const auto chunk_max = glm::max(chunk_bounds.min, chunk_bounds.max);
    const auto bounds_min = glm::min(visible_bounds_.min, visible_bounds_.max);
    const auto bounds_max = glm::max(visible_bounds_.min, visible_bounds_.max);

    return (chunk_min.x <= bounds_max.x && chunk_max.x >= bounds_min.x) &&
           (chunk_min.y <= bounds_max.y && chunk_max.y >= bounds_min.y);
}

auto ChunkManager::Debug() -> void {
    ImGui::SetNextWindowFocus();
    ImGui::Begin("Chunk Manager");
    ImGui::Text("Image dimensions: %dx%d", image_dims_.width, image_dims_.height);
    ImGui::Text("Current LOD: %d", curr_lod);
    ImGui::Separator();
    ImGui::Checkbox("Show Wireframes", &show_wireframes);
    ImGui::Separator();
    ImGui::Text(" V  L  ");
    for (auto lod = max_lod_; lod >= 0; --lod) {
        for (auto i = 0; i < chunks_[lod].size(); ++i) {
            const auto visible = chunks_[lod][i].visible ? "X" : " ";
            const auto loaded = chunks_[lod][i].State() == ChunkState::Loaded ? "X" : " ";
            ImGui::Text("[%s][%s] LOD_%d_CHUNK_%d", visible, loaded, lod, i);
        }
    }
    ImGui::End();
}