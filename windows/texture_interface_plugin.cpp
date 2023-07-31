#include "include/texture_interface/texture_interface_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include "include/texture_interface/frame.h"

#include <map>
#include <memory>
#include <sstream>
#include <unordered_map>

namespace
{

  class Texture_interfacePlugin : public flutter::Plugin
  {
  public:
    static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

    flutter::MethodChannel<flutter::EncodableValue> *channel() const
    {
      return m_channel.get();
    }

    Texture_interfacePlugin(std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel,
                            flutter::TextureRegistrar *texture_registrar);

    virtual ~Texture_interfacePlugin();

  private:
    // Called when a method is called on this plugin's channel from Dart.
    void HandleMethodCall(
        const flutter::MethodCall<flutter::EncodableValue> &method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

    flutter::TextureRegistrar *m_texture_registrar;
    std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> m_channel;
    std::unordered_map<int, std::unique_ptr<Frame>> m_frames;
    std::unordered_map<int, std::unique_ptr<GPUFrame>> m_gpu_frames;

    ID3D11Device *m_d3d11_device = nullptr;
    ID3D11DeviceContext *m_d3d11_device_context = nullptr;
  };

  // static
  void Texture_interfacePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarWindows *registrar)
  {
    auto plugin = std::make_unique<Texture_interfacePlugin>(
        std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), "texture_interface",
            &flutter::StandardMethodCodec::GetInstance()),
        registrar->texture_registrar());

    plugin->channel()->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result)
        {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    registrar->AddPlugin(std::move(plugin));
  }

  Texture_interfacePlugin::Texture_interfacePlugin(
      std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel,
      flutter::TextureRegistrar *texture_registrar)
      : m_channel(std::move(channel)), m_texture_registrar(texture_registrar) {}

  Texture_interfacePlugin::~Texture_interfacePlugin() {}

  void Texture_interfacePlugin::HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
  {

    if (method_call.method_name().compare("getPlatformVersion") == 0)
    {
      std::ostringstream version_stream;
      version_stream << "Windows ";
      if (IsWindows10OrGreater())
      {
        version_stream << "10+";
      }
      else if (IsWindows8OrGreater())
      {
        version_stream << "8";
      }
      else if (IsWindows7OrGreater())
      {
        version_stream << "7";
      }
      result->Success(flutter::EncodableValue(version_stream.str()));
    }

    else if (method_call.method_name().compare("RegisterTexture") == 0)
    {
      flutter::EncodableMap arguments =
          std::get<flutter::EncodableMap>(*method_call.arguments());
      auto id = std::get<int>(arguments[flutter::EncodableValue("id")]);
      auto [map_entry, added] = m_frames.try_emplace(id, nullptr);

      if (added)
      {
        map_entry->second = std::make_unique<Frame>(m_texture_registrar);

        /*
        it->second->SetReleaseCallback(
            [](void *user_data)
            {
              user_release_context *context = (user_release_context *)user_data;
              int64_t ptra = reinterpret_cast<int64_t>(context->buffer);
              context->channel->InvokeMethod("FreeBuffer", std::make_unique<flutter::EncodableValue>(ptra));
              delete context;
            });*/
      }
      return result->Success(flutter::EncodableValue(map_entry->second->texture_id()));
    }
    else if (method_call.method_name().compare("UpdateFrame") == 0)
    {
      flutter::EncodableMap arguments =
          std::get<flutter::EncodableMap>(*method_call.arguments());

      auto id = std::get<int>(arguments[flutter::EncodableValue("id")]);

      int32_t width = std::get<int32_t>(arguments[flutter::EncodableValue("width")]);
      int32_t height = std::get<int32_t>(arguments[flutter::EncodableValue("height")]);

      int64_t bufferptra = std::get<int64_t>(arguments[flutter::EncodableValue("buffer")]);
      // std::vector<uint8_t> buffer = std::get<std::vector<uint8_t>>(arguments[flutter::EncodableValue("buffer")]);

      uint8_t *bufferptr = reinterpret_cast<uint8_t *>(bufferptra);

      auto frame = m_frames.find(id);
      if (frame == m_frames.end())
      {
        return result->Error("-2", "Texture was not found.");
      }
      /*
      user_release_context *urc = new user_release_context;
      urc->channel = channel_.get();
      urc->buffer = bufferptr;

      frame->second->SetReleaseContext(urc);*/

      frame->second->Update(bufferptr, width, height);

      return result->Success();
    }
    else if (method_call.method_name().compare("UnregisterTexture") == 0)
    {
      flutter::EncodableMap arguments =
          std::get<flutter::EncodableMap>(*method_call.arguments());
      auto id =
          std::get<int>(arguments[flutter::EncodableValue("id")]);

      if (m_frames.find(id) == m_frames.end())
      {
        return result->Error("-2", "Texture was not found.");
      }
      m_frames.erase(id);
      result->Success(flutter::EncodableValue(nullptr));
    }
    else if (method_call.method_name().compare("CreateD3D11Device") == 0)
    {
      HRESULT hr = D3D11CreateDevice(
          nullptr,                           // ADAPTER
          D3D_DRIVER_TYPE_HARDWARE,          // HARDWARE/SOFTWARE
          nullptr,                           // Software
          D3D11_CREATE_DEVICE_VIDEO_SUPPORT, // Flags
          nullptr,                           // pFeatureLevels
          0,                                 // feeatureLevel
          D3D11_SDK_VERSION,                 // SDK Version
          &m_d3d11_device,                   //* out ID3D11Device
          nullptr,                           // pFeatureLevels
          &m_d3d11_device_context            //* out ID3D11DeviceContext
      );
      if FAILED (hr)
      {
        printf("Failed to create D3D11 Device and context.");
      }

      result->Success(flutter::EncodableValue(reinterpret_cast<int64_t>(m_d3d11_device)));
    }
    else if (method_call.method_name().compare("CreateGPUTexture") == 0)
    {
      flutter::EncodableMap arguments =
          std::get<flutter::EncodableMap>(*method_call.arguments());

      auto id = std::get<int>(arguments[flutter::EncodableValue("id")]);
      int32_t width = std::get<int32_t>(arguments[flutter::EncodableValue("width")]);
      int32_t height = std::get<int32_t>(arguments[flutter::EncodableValue("height")]);
      // create
      auto [map_entry, added] = m_gpu_frames.try_emplace(id, nullptr);
      if (!added)
      {
        printf("Could not create new map entry with id %d.\n", id);
        fflush(stdout);
        return result->Success(flutter::EncodableValue(-1));
      }
      map_entry->second = std::make_unique<GPUFrame>(m_texture_registrar, width, height, m_d3d11_device, m_d3d11_device_context);
      HANDLE shared_handle = map_entry->second->shared_handle();
      if (shared_handle == nullptr)
      {
        printf("Could not create new GPUFrame instance. shared_handle is nullptr. \n");
        fflush(stdout);
        return result->Success(flutter::EncodableValue(-1));
      }
      int64_t i_shared_handle = map_entry->second->shared_handle_asInt();
      if (i_shared_handle == 0)
      {
        printf("Could not create new GPUFrame instance. shared_handle is 0. \n");
        fflush(stdout);
        return result->Success(flutter::EncodableValue(-1));
      }
      int64_t texture_id = map_entry->second->texture_id();
      if (texture_id == -1)
      {
        printf("Could not create new GPUFrame instance. Textureid is -1. \n");
        fflush(stdout);
        return result->Success(flutter::EncodableValue(-1));
      }
      flutter::EncodableMap res;
      res[flutter::EncodableValue("shared_handle")] = flutter::EncodableValue(i_shared_handle);
      res[flutter::EncodableValue("texture_id")] = flutter::EncodableValue(texture_id);

      return result->Success(res);
    }
    else if (method_call.method_name().compare("DestroyGPUTexture") == 0)
    {
      flutter::EncodableMap arguments =
          std::get<flutter::EncodableMap>(*method_call.arguments());

      auto id = std::get<int>(arguments[flutter::EncodableValue("id")]);

      if (m_gpu_frames.find(id) == m_gpu_frames.end())
      {
        return result->Error("-2", "Texture was not found.");
      }
      m_gpu_frames.erase(id);
      result->Success(flutter::EncodableValue(nullptr));
    }

    else
    {
      result->NotImplemented();
    }
  }
} // namespace

void Texture_interfacePluginRegisterWithRegistrar(FlutterDesktopPluginRegistrarRef registrar)
{
  Texture_interfacePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
