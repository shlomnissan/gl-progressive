// Copyright © 2024 - Present, Shlomi Nissan.
// All rights reserved.

#include "chunk.h"

#include <random>
#include <thread>
#include <chrono>

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

Chunk::Chunk(const Params& params, const fs::path& path) : params_(params), source_(path) {
    image_loader_ = ImageLoader::Create();
}

auto Chunk::Load() -> void {
    image_loader_->LoadAsync(source_, [this](const auto& image) {
        if (image.has_value()) {
            texture_.SetImage(image.value());

            // Add random delay for chunks at Lod 0 to simulate loading time
            if (params_.lod == 0) {
                static thread_local std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution dist(100, 1000);
                auto delay_ms = dist(rng);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }

            state_ = ChunkState::Loaded;
            std::print("Loaded chunk {}\n", source_.string());
        } else {
            state_ = ChunkState::Error;
        }
    });
}

auto Chunk::ModelMatrix() const -> glm::mat4 {
    return glm::translate(glm::mat4(1.0f), glm::vec3 {
        params_.position.x + params_.size.x / 2.0f, // offset right
        params_.position.y + params_.size.y / 2.0f, // offset up
        0.0f
    });
}