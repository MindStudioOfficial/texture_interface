#ifndef FRAME_H
#define FRAME_H

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <windows.h>
#include <wrl.h>

#include <d3d.h>
#include <d3d11.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

#include <mutex>

using namespace Microsoft::WRL;

class Frame
{
public:
    Frame(flutter::TextureRegistrar *texture_registrar);

    int64_t texture_id() const { return m_texture_id; }

    void Update(uint8_t *buffer, int32_t width, int32_t height);

    ~Frame();

    void SetReleaseCallback(void (*release_callback)(void *release_context));
    void SetReleaseContext(void *release_context);

private:
    FlutterDesktopPixelBuffer m_flutter_pixel_buffer{};
    flutter::TextureRegistrar *m_texture_registrar = nullptr;
    std::unique_ptr<flutter::TextureVariant> m_texture = nullptr;
    int64_t m_texture_id;
    mutable std::mutex m_mutex;
};

class GPUFrame
{
public:
    GPUFrame(
        flutter::TextureRegistrar *texture_registrar,
        size_t initialWidth,
        size_t initialHeight,
        ID3D11Device* d3d11_device,
        ID3D11DeviceContext* d3d11_device_context);
    int64_t texture_id() const { return m_texture_id; }
    HANDLE shared_handle() const {return m_shared_handle; }
    int64_t shared_handle_asInt() {
        return reinterpret_cast<int64_t>(m_shared_handle);
    }

    ~GPUFrame();

private:
    size_t m_width, m_height;
    int64_t m_texture_id = -1;
    mutable std::mutex m_mutex;

    flutter::TextureRegistrar *m_texture_registrar = nullptr;
    std::unique_ptr<flutter::TextureVariant> m_texture = nullptr;
    HANDLE m_shared_handle;
    ComPtr<ID3D11Texture2D> m_d3d11_texture_2D;
    std::unique_ptr<FlutterDesktopGpuSurfaceDescriptor> m_gpu_surface_descriptor = nullptr;
};

#endif