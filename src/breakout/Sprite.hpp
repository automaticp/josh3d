#pragma once
#include "GLObjects.hpp"
#include "GLObjectPool.hpp"

#include <algorithm>
#include <cstring>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>
#include <array>
#include <memory>
#include <utility>



class Sprite {
private:
    learn::Shared<learn::TextureHandle> texture_;
    glm::vec4 color_;

    friend class SpriteRenderer;

public:
    Sprite(learn::Shared<learn::TextureHandle> texture, glm::vec4 color)
        : texture_{ std::move(texture) }, color_{ color } {}

    explicit Sprite(learn::Shared<learn::TextureHandle> texture)
        : Sprite(std::move(texture), glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }) {}

    const learn::TextureHandle& texture() const noexcept { return *texture_; }
    glm::vec4& color() noexcept { return color_; }
    const glm::vec4& color() const noexcept { return color_; }
};




