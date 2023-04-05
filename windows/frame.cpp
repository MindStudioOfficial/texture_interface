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
    // flutter_pixel_buffer_.release_context = &buffer;
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
    d3d11_texture2D_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
    ComPtr<IDXGIResource> dxgi_resource;
    hr = m_d3d11_texture_2D.As(&dxgi_resource);
    if FAILED (hr)
    {
        printf("Failed to convert to IDXGIResource\n");
    }
    //* GET SHARED HANDLE
    hr = dxgi_resource->GetSharedHandle(&m_shared_handle);
    if FAILED (hr)
    {
        printf("Failed to get dxgi shared handle\n");
    }
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

GPUFrame::~GPUFrame() {
    m_texture_registrar->UnregisterTexture(m_texture_id);
}