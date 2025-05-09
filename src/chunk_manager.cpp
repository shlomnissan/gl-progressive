// Copyright © 2024 - Present, Shlomi Nissan.
// All rights reserved.

#include "chunk_manager.h"

#include <format>

#include <imgui.h>

ChunkManager::ChunkManager(const Parameters& params) :
    image_dims_(params.image_dims),
    window_dims_(params.window_dims),
    lods_(params.lods)
{
    chunks_.resize(lods_);
    GenerateChunks();
}

auto ChunkManager::ComputeLod(const OrthographicCamera& camera) const -> int {
    const auto virtual_width = camera.Width() / glm::length(glm::vec3{camera.transform[0]});
    const auto virtual_units_per_screen_pixel =  virtual_width / window_dims_.width;
    const auto lod_shift = static_cast<float>(lods_);
    const float lod_f = lod_shift + std::log2(1 / virtual_units_per_screen_pixel);
    return std::clamp(static_cast<int>(std::floor(lod_f)), 0, lods_ - 1);
}

auto ChunkManager::Update(const OrthographicCamera& camera) -> void {
    curr_lod_ = ComputeLod(camera);
    visible_bounds_ = ComputeVisibleBounds(camera);
};

auto ChunkManager::GetVisibleChunks() -> std::vector<Chunk>& {
    return chunks_[curr_lod_];
}

auto ChunkManager::GenerateChunks() -> void {
    for (auto i = 0u; i < lods_; ++i) {
        const auto lod_width = static_cast<float>(image_dims_.width) / (1 << i);
        const auto lod_height = static_cast<float>(image_dims_.height) / (1 << i);
        const auto grid_x = static_cast<int>(lod_width / kChunkSize);
        const auto grid_y = static_cast<int>(lod_height / kChunkSize);
        const auto n_chunks = grid_x * grid_y;

        for (auto j = 1; j <= n_chunks; ++j) {
            auto x = (j - 1) % grid_x;
            auto y = (j - 1) / grid_y;
            auto path = std::format("assets/lod_{}/spiralcrop{}_{:02}.jpg", i, i, j);
            chunks_[i].emplace_back(Chunk::Params {
                .lod = i,
                .grid_index = {x, y},
                .position = {x * kChunkSize, y * kChunkSize},
                .size = {kChunkSize, kChunkSize}
            }, path);
        }
    }

    for (auto& chunks : chunks_) {
        for (auto& chunk : chunks) chunk.Load();
    }
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

auto ChunkManager::Debug() const -> void {
    ImGui::SetNextWindowFocus();
    ImGui::Begin("Chunk Manager");
    ImGui::Text("Image dimensions: %dx%d", image_dims_.width, image_dims_.height);
    ImGui::Text("Level of detail: %d", curr_lod_);
    ImGui::Separator();

    for (auto i = 0; i < chunks_[curr_lod_].size(); ++i) {
        IsChunkVisible(chunks_[curr_lod_][i]) ?
            ImGui::Text("[X] CHUNK_%d_%d", curr_lod_, i) :
            ImGui::Text("[ ] CHUNK_%d_%d", curr_lod_, i);
    }
    ImGui::End();
}