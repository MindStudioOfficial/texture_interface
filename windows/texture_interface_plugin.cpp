#include "include/texture_interface/texture_interface_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <map>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "include/texture_interface/frame.h"

namespace
{

  class Texture_interfacePlugin : public flutter::Plugin
  {
  public:
    static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

    flutter::MethodChannel<flutter::EncodableValue> *channel() const
    {
      return channel_.get();
    }

    Texture_interfacePlugin(std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel,
                        flutter::TextureRegistrar *texture_registrar);

    virtual ~Texture_interfacePlugin();

  private:
    // Called when a method is called on this plugin's channel from Dart.
    void HandleMethodCall(
        const flutter::MethodCall<flutter::EncodableValue> &method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

    flutter::TextureRegistrar *texture_registrar_;
    std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;
    std::unordered_map<int, std::unique_ptr<Frame>> frames_;
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
      : channel_(std::move(channel)), texture_registrar_(texture_registrar) {}

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
      auto [it, added] = frames_.try_emplace(id, nullptr);

      if (added)
      {
        it->second = std::make_unique<Frame>(texture_registrar_);

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
      return result->Success(flutter::EncodableValue(it->second->texture_id()));
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

      auto frame = frames_.find(id);
      if (frame == frames_.end())
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

      if (frames_.find(id) == frames_.end())
      {
        return result->Error("-2", "Texture was not found.");
      }
      // auto player = g_players->Get(player_id);
      // player->SetVideoFrameCallback(nullptr);
      frames_.erase(id);
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
