#ifndef FRAME_H
#define FRAME_H

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <mutex>

class Frame
{
public:
    Frame(flutter::TextureRegistrar *texture_registrar);

    int64_t texture_id() const { return texture_id_; }

    void Update(uint8_t *buffer, int32_t width, int32_t height);

    ~Frame();

    void SetReleaseCallback(void (* release_callback)(void* release_context));
    void SetReleaseContext(void *release_context);


private:
    FlutterDesktopPixelBuffer flutter_pixel_buffer_{};
    flutter::TextureRegistrar *texture_registrar_ = nullptr;
    std::unique_ptr<flutter::TextureVariant> texture_ = nullptr;
    int64_t texture_id_;
    mutable std::mutex mutex_;  
};

typedef struct {
    flutter::MethodChannel<flutter::EncodableValue>* channel;
    uint8_t* buffer;
} user_release_context;

#endif