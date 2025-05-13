// Copyright © 2024 - Present, Shlomi Nissan.
// All rights reserved.

#pragma once

#include <memory>
#include <filesystem>

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

#include "core/texture2d.h"
#include "loaders/image_loader.h"

namespace fs = std::filesystem;

enum class ChunkState {
    Unloaded,
    Loading,
    Loaded,
    Error
};

class Chunk {
public:
    struct Params {
        glm::ivec2 grid_index;
        glm::vec2 position {0.0f, 0.0f};
        glm::vec2 size;
        float scale {1.0f};
        unsigned lod;
    };

    bool visible {false};

    Chunk(const Params& params, const fs::path& path);

    [[nodiscard]] auto State() const -> ChunkState {
        return state_;
    }

    [[nodiscard]] auto Position() const {
        return params_.position;
    }

    [[nodiscard]] auto Size() const {
        return params_.size;
    }

    [[nodiscard]] auto Texture() -> Texture2D& {
        return texture_;
    }

    [[nodiscard]] auto ModelMatrix() const -> glm::mat4;

    auto Load() -> void;

private:
    Params params_;

    fs::path source_;

    ChunkState state_ {ChunkState::Unloaded};

    std::shared_ptr<ImageLoader> image_loader_ {nullptr};

    Texture2D texture_ {};
};