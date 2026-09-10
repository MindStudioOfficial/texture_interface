#include "include/texture_interface/frame.h"

#include <iostream>

Frame::Frame(flutter::TextureRegistrar* texture_registrar)
    : m_textureRegistrar(texture_registrar) {
    m_texture = std::make_unique<flutter::TextureVariant>(
        flutter::PixelBufferTexture(
            [this](size_t width, size_t height) -> const FlutterDesktopPixelBuffer* {
                return CopyPixelBuffer(width, height);
            }));

    m_textureId = m_textureRegistrar->RegisterTexture(m_texture.get());
}

const FlutterDesktopPixelBuffer* Frame::CopyPixelBuffer(size_t requested_width, size_t requested_height) {
    const std::scoped_lock<std::mutex> lock(m_mutex);

    if (m_currentSlot == nullptr) {
        std::cout << "No current slot available for CopyPixelBuffer." << std::endl;
        return nullptr;
    }

    if (m_currentSlot->data.get() == nullptr) {
        std::cout << "Current slot has no data allocated." << std::endl;
        return nullptr;
    }

    auto* slot                                   = m_currentSlot;
    m_flutterPixelBufferTexture.buffer           = slot->data.get();
    m_flutterPixelBufferTexture.width            = slot->width;
    m_flutterPixelBufferTexture.height           = slot->height;
    m_flutterPixelBufferTexture.release_context  = new ReleaseContext{this, m_currentSlot};
    m_flutterPixelBufferTexture.release_callback = [](void* release_context) {
        auto* context = static_cast<ReleaseContext*>(release_context);
        context->frame->ReleaseBuffer(context->slot);
        delete context;
    };
    return &m_flutterPixelBufferTexture;
}

void Frame::ReleaseBuffer(PixelBufferSlot* slot) noexcept {
    const std::scoped_lock<std::mutex> lock(m_mutex);
    slot->is_in_use = false;
}

bool Frame::ResizePixelBufferSlot(PixelBufferSlot& slot, uint32_t width, uint32_t height) {
    if (slot.is_in_use) {
        std::cout << "Cannot resize PixelBufferSlot while it is in use." << std::endl;
        return false;
    }
    if (slot.width == width && slot.height == height) {
        return true;
    }
    slot.data = std::make_unique<uint8_t[]>(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    if (!slot.data) {
        std::cout << "Failed to allocate memory for PixelBufferSlot." << std::endl;
        return false;
    }

    slot.width  = width;
    slot.height = height;
    return true;
}

Frame::~Frame() {
}

bool Frame::Update(uint8_t* buffer, uint32_t width, uint32_t height) {
    if (buffer == nullptr || width == 0 || height == 0)
        return false;

    const std::scoped_lock lock(m_mutex);

    for (auto& slot : m_pixelBufferSlots) {
        if (slot.data.get() != buffer) {
            continue;
        }

        if (slot.width != width || slot.height != height || !slot.is_in_use) {
            std::cout << "Buffer size mismatch or slot not in use." << std::endl;
            return false;
        }
        m_currentSlot = &slot;
        m_textureRegistrar->MarkTextureFrameAvailable(m_textureId);
        return true;
    }
    return false;
}

uint8_t* Frame::GetBuffer(uint32_t width, uint32_t height) {
    for (auto& slot : m_pixelBufferSlots) {
        if (!slot.is_in_use) {
            const std::scoped_lock<std::mutex> lock(m_mutex);
            if (!ResizePixelBufferSlot(slot, width, height)) {
                return nullptr;
            }
            slot.is_in_use = true;
            return slot.data.get();
        }
    }
    return nullptr;
}

void Frame::Unregister(std::function<void()> callback) {
    if (m_textureId < 0) {
        if (callback) {
            callback();
        }
        return;
    }

    const auto textureId = m_textureId;
    m_textureId          = -1;

    m_textureRegistrar->UnregisterTexture(textureId, [callback = std::move(callback)]() {
        if (callback) {
            callback();
        }
    });
}
