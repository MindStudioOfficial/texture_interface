#include "include/texture_interface/frame.h"

Frame::Frame(flutter::TextureRegistrar *texture_registrar) : texture_registrar_(texture_registrar)
{
    texture_ = std::make_unique<flutter::TextureVariant>(
        flutter::PixelBufferTexture(
            [=](size_t width, size_t height) -> const FlutterDesktopPixelBuffer *
            {
                const std::lock_guard<std::mutex> lock(mutex_);

                return &flutter_pixel_buffer_;
            }));

    texture_id_ = texture_registrar_->RegisterTexture(texture_.get());
}

void Frame::Update(uint8_t *buffer, int32_t width, int32_t height)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    flutter_pixel_buffer_.buffer = buffer;
    flutter_pixel_buffer_.width = width;
    flutter_pixel_buffer_.height = height;
    //flutter_pixel_buffer_.release_context = &buffer;
    /*flutter_pixel_buffer_.release_callback = [](void *user_data)
    {
        uint8_t *old_buffer = reinterpret_cast<uint8_t *>(user_data);
        if (old_buffer != nullptr)
            free(old_buffer);
    };*/
    texture_registrar_->MarkTextureFrameAvailable(texture_id_);
}

Frame::~Frame()
{
    texture_registrar_->UnregisterTexture(texture_id_);
    free(flutter_pixel_buffer_.release_context);
}

void Frame::SetReleaseCallback(void(* release_callback)(void* release_context)) {
    this->flutter_pixel_buffer_.release_callback = release_callback;
}

void Frame::SetReleaseContext(void* release_context) {
    this->flutter_pixel_buffer_.release_context = release_context;
}

