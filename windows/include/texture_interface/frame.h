#pragma once

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <array>
#include <mutex>

class Frame {
public:
    explicit Frame(flutter::TextureRegistrar* texture_registrar);

    Frame(const Frame&)            = delete;
    Frame& operator=(const Frame&) = delete;

    ~Frame();

    [[nodiscard]]
    int64_t texture_id() const noexcept { return m_textureId; }

    [[nodiscard]]
    bool Update(uint8_t* buffer, uint32_t width, uint32_t height);

    [[nodiscard]]
    uint8_t* GetBuffer(uint32_t width, uint32_t height);

    void Unregister(std::function<void()> callback);

private:
    [[nodiscard]]
    const FlutterDesktopPixelBuffer* CopyPixelBuffer(
        size_t requestedWidth,
        size_t requestedHeight);

    struct PixelBufferSlot {
        std::unique_ptr<uint8_t[]> data      = nullptr;
        uint32_t                   width     = 0;
        uint32_t                   height    = 0;
        bool                       is_in_use = false;

        [[nodiscard]]
        size_t size() const noexcept { return static_cast<size_t>(width) * static_cast<size_t>(height) * 4; }
    };

    void ReleaseBuffer(PixelBufferSlot* slot) noexcept;

    [[nodiscard]]
    bool ResizePixelBufferSlot(PixelBufferSlot& slot, uint32_t width, uint32_t height);

    struct ReleaseContext {
        Frame*           frame;
        PixelBufferSlot* slot;
    };

    FlutterDesktopPixelBuffer                m_flutterPixelBufferTexture{};
    flutter::TextureRegistrar*               m_textureRegistrar = nullptr;
    std::unique_ptr<flutter::TextureVariant> m_texture          = nullptr;
    int64_t                                  m_textureId{};
    std::mutex                               m_mutex;

    std::array<PixelBufferSlot, 2> m_pixelBufferSlots;
    PixelBufferSlot*               m_currentSlot = nullptr;
};
