#include "include/texture_interface/frame.h"

Frame::Frame(flutter::TextureRegistrar *texture_registrar) : m_texture_registrar(texture_registrar)
{
    m_texture = std::make_unique<flutter::TextureVariant>(
        flutter::PixelBufferTexture(
            [=](size_t width, size_t height) -> const FlutterDesktopPixelBuffer *
            {
                const std::lock_guard<std::mutex> lock(m_mutex);

                return &m_flutter_pixel_buffer;
            }));

    m_texture_id = texture_registrar->RegisterTexture(m_texture.get());
}

void Frame::Update(uint8_t *buffer, int32_t width, int32_t height)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_flutter_pixel_buffer.buffer = buffer;
    m_flutter_pixel_buffer.width = width;
    m_flutter_pixel_buffer.height = height;
    m_flutter_pixel_buffer.release_context = nullptr;
    m_flutter_pixel_buffer.release_callback = [](void *release_context) {};
    /*flutter_pixel_buffer_.release_callback = [](void *user_data)
    {
        uint8_t *old_buffer = reinterpret_cast<uint8_t *>(user_data);
        if (old_buffer != nullptr)
            free(old_buffer);
    };*/
    m_texture_registrar->MarkTextureFrameAvailable(m_texture_id);
}

Frame::~Frame()
{
    m_texture_registrar->UnregisterTexture(m_texture_id);
}

void Frame::SetReleaseCallback(void (*release_callback)(void *release_context))
{
    this->m_flutter_pixel_buffer.release_callback = release_callback;
}

void Frame::SetReleaseContext(void *release_context)
{
    this->m_flutter_pixel_buffer.release_context = release_context;
}

GPUFrame::GPUFrame(
    flutter::TextureRegistrar *texture_registrar,
    size_t initialWidth,
    size_t initialHeight,
    ID3D11Device *d3d11_device,
    ID3D11DeviceContext *d3d11_device_context) : m_texture_registrar(texture_registrar),
                                                 m_height(initialHeight),
                                                 m_width(initialWidth)
{
    //* CREATE D3D11 Texture
    D3D11_TEXTURE2D_DESC d3d11_texture2D_desc = {0};
    d3d11_texture2D_desc.Width = (UINT)m_width;
    d3d11_texture2D_desc.Height = (UINT)m_height;
    d3d11_texture2D_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    d3d11_texture2D_desc.MipLevels = 1;
    d3d11_texture2D_desc.ArraySize = 1;
    d3d11_texture2D_desc.SampleDesc.Count = 1;
    d3d11_texture2D_desc.SampleDesc.Quality = 0;
    d3d11_texture2D_desc.Usage = D3D11_USAGE_DEFAULT;
    d3d11_texture2D_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    d3d11_texture2D_desc.CPUAccessFlags = 0;
    d3d11_texture2D_desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    HRESULT hr = d3d11_device->CreateTexture2D(&d3d11_texture2D_desc, nullptr, &m_d3d11_texture_2D);
    if FAILED (hr)
    {
        printf("Failed to create d3d11_texture_2D\n");
    }
    //* CONVERT TO DXGI Resource
    auto dxgi_resource = ComPtr<IDXGIResource>{};
    hr = m_d3d11_texture_2D.As(&dxgi_resource);
    if (FAILED (hr) ||dxgi_resource == nullptr) 
    {
        printf("Failed to convert to IDXGIResource\n");
    }
    //* GET SHARED HANDLE
    hr = dxgi_resource->GetSharedHandle(&m_shared_handle);
    if FAILED (hr)
    {
        printf("Failed to get dxgi shared handle\n");
    }
    

    //* CREATE EGL DISPLAY
    if (m_display == EGL_NO_DISPLAY)
    {
        auto eglGetPlatformDisplayEXT =
            reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
                eglGetProcAddress("eglGetPlatformDisplayEXT"));
        checkEglError(__LINE__);
        if (eglGetPlatformDisplayEXT)
        {
            // D3D11
            m_display = eglGetPlatformDisplayEXT(
                EGL_PLATFORM_ANGLE_ANGLE,
                EGL_DEFAULT_DISPLAY,
                kD3D11DisplayAttributes);
            if (eglInitialize(m_display, 0, 0) == EGL_FALSE)
            {
                m_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE,
                                                     EGL_DEFAULT_DISPLAY,
                                                     kD3D11_9_3DisplayAttributes);
                if (eglInitialize(m_display, 0, 0) == EGL_FALSE)
                {
                    // D3D 9.
                    m_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE,
                                                         EGL_DEFAULT_DISPLAY,
                                                         kD3D9DisplayAttributes);
                    if (eglInitialize(m_display, 0, 0) == EGL_FALSE)
                    {
                        // Whatever.
                        m_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE,
                                                             EGL_DEFAULT_DISPLAY,
                                                             kWrapDisplayAttributes);
                        if (eglInitialize(m_display, 0, 0) == EGL_FALSE)
                        {
                            printf("Failed to initialize EGL Display\n");
                            return;
                        }
                    }
                }
            }
        }
        else
        {
            printf("Failed to initialize EGL Display eglGetProcAddress\n");
            return;
        }
    }

    //* CREATE AND BIND EGL SURFACE

    if (m_egl_context == EGL_NO_CONTEXT)
    {
        EGLint count = 0;
        EGLBoolean res = eglChooseConfig(
            m_display,
            kEGLConfigurationAttributes,
            &m_egl_config,
            1,
            &count);

        checkEglError(__LINE__);
        if (res == EGL_FALSE || count != 1 || m_egl_config == nullptr)
        {
            printf("Failed to choose EGL config.\n");
            return;
        }
        m_egl_context = eglCreateContext(
            m_display,
            m_egl_config,
            EGL_NO_CONTEXT,
            kEGLContextAttributes);
        checkEglError(__LINE__);
        if (m_egl_context == EGL_NO_CONTEXT)
        {
            printf("Failed to create EGL context.\n");
            return;
        }
    }
    EGLint buffer_attributes[] = {
        EGL_WIDTH,
        (EGLint)m_width,
        EGL_HEIGHT,
        (EGLint)m_height,
        EGL_TEXTURE_TARGET,
        EGL_TEXTURE_2D,
        EGL_TEXTURE_FORMAT,
        EGL_TEXTURE_RGBA,
        EGL_NONE,
    };

    m_surface = eglCreatePbufferFromClientBuffer(
        m_display,
        (EGLenum)EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE,
        (EGLClientBuffer)m_shared_handle,
        m_egl_config,
        buffer_attributes);
    checkEglError(__LINE__);
    if (m_surface == EGL_NO_SURFACE)
    {
        printf("Failed to create EGL surface.\n");
        return;
    }

    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    eglBindTexImage(m_display, m_surface, EGL_BACK_BUFFER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //* CREATE FLUTTER GPU TEXTURE
    m_gpu_surface_descriptor = std::make_unique<FlutterDesktopGpuSurfaceDescriptor>();
    m_gpu_surface_descriptor->struct_size = sizeof(FlutterDesktopGpuSurfaceDescriptor);
    m_gpu_surface_descriptor->handle = m_shared_handle;
    m_gpu_surface_descriptor->width = m_gpu_surface_descriptor->visible_width = m_width;
    m_gpu_surface_descriptor->height = m_gpu_surface_descriptor->visible_height = m_height;
    m_gpu_surface_descriptor->release_context = nullptr;
    m_gpu_surface_descriptor->release_callback = [](void *release_context) {};
    m_gpu_surface_descriptor->format = kFlutterDesktopPixelFormatRGBA8888;
    m_texture = std::make_unique<flutter::TextureVariant>(
        flutter::GpuSurfaceTexture(
            kFlutterDesktopGpuSurfaceTypeDxgiSharedHandle,
            [&](size_t width, size_t height) -> const FlutterDesktopGpuSurfaceDescriptor *
            {
                return m_gpu_surface_descriptor.get();
            }));

    m_texture_id = m_texture_registrar->RegisterTexture(m_texture.get());
}

GPUFrame::~GPUFrame()
{
    m_texture_registrar->UnregisterTexture(m_texture_id);
}

const char* eglGetErrorString(EGLint error) {
    switch (error) {
        case EGL_SUCCESS:
            return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:
            return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:
            return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:
            return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:
            return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT:
            return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG:
            return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE:
            return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:
            return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE:
            return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH:
            return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER:
            return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP:
            return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:
            return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST:
            return "EGL_CONTEXT_LOST";
        default:
            return "Unknown EGL Error";
    }
}

void checkEglError(int line) {
    EGLint error = eglGetError();
    if (error != EGL_SUCCESS) {
        printf("EGL Error line %d: %s\n",line, eglGetErrorString(error));
    }
}