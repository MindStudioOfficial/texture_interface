#ifndef FRAME_H
#define FRAME_H

#define DIRECT3D_VERSION 0x0900

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include "../EGL/egl.h"
#include "../EGL/eglext.h"
#include "../EGL/eglplatform.h"
#include "../GLES2/gl2.h"
#include "../GLES2/gl2ext.h"

#include <windows.h>

#include <d3d.h>
#include <d3d11.h>
#include <wrl.h>

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
        ID3D11Device *d3d11_device,
        ID3D11DeviceContext *d3d11_device_context);
    int64_t texture_id() const { return m_texture_id; }
    HANDLE shared_handle() const { return m_shared_handle; }
    int64_t shared_handle_asInt()
    {
        return reinterpret_cast<int64_t>(m_shared_handle);
    }

    ~GPUFrame();

private:
    size_t m_width, m_height;
    int64_t m_texture_id = -1;
    mutable std::mutex m_mutex;

    flutter::TextureRegistrar *m_texture_registrar = nullptr;
    std::unique_ptr<flutter::TextureVariant> m_texture = nullptr;
    HANDLE m_shared_handle = nullptr;
    ComPtr<ID3D11Texture2D> m_d3d11_texture_2D;
    std::unique_ptr<FlutterDesktopGpuSurfaceDescriptor> m_gpu_surface_descriptor = nullptr;

    EGLSurface m_surface = EGL_NO_SURFACE;
    EGLDisplay m_display = EGL_NO_DISPLAY;
    EGLContext m_egl_context = nullptr;
    EGLConfig m_egl_config = nullptr;

    static constexpr EGLint kEGLConfigurationAttributes[] = {
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_ALPHA_SIZE,
        8,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,
        EGL_PBUFFER_BIT,
        EGL_NONE,
    };
    static constexpr EGLint kEGLContextAttributes[] = {
        EGL_CONTEXT_CLIENT_VERSION,
        2,
        EGL_NONE,
    };

    static constexpr EGLint kD3D11DisplayAttributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
      EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE,
      EGL_TRUE,
      EGL_NONE,
  };
  static constexpr EGLint kD3D11_9_3DisplayAttributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
      EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE,
      9,
      EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE,
      3,
      EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE,
      EGL_TRUE,
      EGL_NONE,
  };
  static constexpr EGLint kD3D9DisplayAttributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE,
      EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
      EGL_NONE,
  };
  static constexpr EGLint kWrapDisplayAttributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
      EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE,
      EGL_TRUE,
      EGL_NONE,
  };
};
const char* eglGetErrorString(EGLint error);
void checkEglError(int line);

#endif
