// Copyright © 2024 - Present, Shlomi Nissan.
// All rights reserved.

#pragma once

#include "chunk.h"

#include "core/orthographic_camera.h"

#include <vector>

#include <glm/vec2.hpp>

class ChunkManager {
public:
    constexpr static auto kChunkSize = 512.0f;

    struct Dimensions {
        unsigned int width {0};
        unsigned int height {0};
    };

    struct Bounds {
        glm::vec2 min {0.0f};
        glm::vec2 max {0.0f};
    };

    struct Parameters {
        Dimensions image_dims;
        Dimensions window_dims;
        int lods {0};
    };

    explicit ChunkManager(const Parameters& params);

    auto Debug() const -> void;

    auto Update(const OrthographicCamera& camera) -> void;

    auto GetVisibleChunks() -> std::vector<Chunk>&;

private:
    std::vector<std::vector<Chunk>> chunks_;

    Bounds visible_bounds_ {};
    Dimensions image_dims_ {};
    Dimensions window_dims_ {};

    int lods_ {0};
    int curr_lod_ {0};

    auto GenerateChunks() -> void;

    auto ComputeLod(const OrthographicCamera& camera) const -> int;

    auto IsChunkVisible(const Chunk& chunk) const -> bool;

    auto ComputeVisibleBounds(const OrthographicCamera& camera) const -> Bounds;
};