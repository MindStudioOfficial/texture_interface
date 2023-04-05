import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:ffi/ffi.dart' as ffi;
import 'dart:ffi' as ffi;

class TextureInterface {
  static const MethodChannel _channel = MethodChannel('texture_interface');
  final Map<int, ValueNotifier<TextureInfo>> _ids = {};
  final Map<int, ValueNotifier<GPUTextureInfo>> _gpuIDS = {};

  Set<int> get ids => _ids.keys.toSet();
  Set<int> get gpuIDS => _gpuIDS.keys.toSet();

  bool _d3d11Initialized = false;

  int getUniqueId() {
    if (_ids.isEmpty) return 0;
    return _ids.keys.reduce((a, b) => a > b ? a : b) + 1;
  }

  int getUniqueGPUid() {
    if (_gpuIDS.isEmpty) return 0;
    return _gpuIDS.keys.reduce((a, b) => a > b ? a : b) + 1;
  }

  Future<bool> register(int id) async {
    if (_ids.containsKey(id)) {
      return false;
    }

    int texId = await _registerTexture(id);
    _ids.addAll(
      {
        id: ValueNotifier<TextureInfo>(
          TextureInfo(
            handle: texId,
            width: 0,
            height: 0,
            previousBuffer: ffi.nullptr,
          ),
        ),
      },
    );
    return true;
  }

  Future<bool> registerGPU(int id, int width, int height) async {
    if (!_d3d11Initialized) {
      await _channel.invokeMethod("CreateD3D11Device");
      _d3d11Initialized = true;
    }

    if (_gpuIDS.containsKey(id)) return false;
    GPUTextureInfo textureInfo = await _createGPUTexture(id, width, height);
    _gpuIDS.addAll({id: ValueNotifier<GPUTextureInfo>(textureInfo)});
    return true;
  }

  Future<bool> unregister(int id) async {
    if (!_ids.containsKey(id)) {
      return false;
    }
    await _unregisterTexture(id);
    _ids.remove(id);
    return true;
  }

  Future<bool> unregisterGPU(int id) async {
    if (!_gpuIDS.containsKey(id)) return false;
    await _destroyGPUTexture(id);
    _gpuIDS.remove(id);
    return true;
  }

  Future<void> dispose() async {
    for (int id in _ids.keys) {
      await _unregisterTexture(id);
    }
    _ids.clear();

    for (int id in _gpuIDS.keys) {
      _destroyGPUTexture(id);
    }
    _ids.clear();
  }

  static Future<String?> get platformVersion async {
    final String? version = await _channel.invokeMethod('getPlatformVersion');
    return version;
  }

  Size? texSize(int id) {
    if (_ids[id] != null) return Size(_ids[id]!.value.width.toDouble(), _ids[id]!.value.height.toDouble());
    return null;
  }

  Future<int> _registerTexture(int id) async {
    int texId = await _channel.invokeMethod(
      "RegisterTexture",
      {
        "id": id,
      },
    );
    return texId;
  }

  Future<GPUTextureInfo> _createGPUTexture(int id, int width, int height) async {
    Map<dynamic, dynamic>? res = await _channel
        .invokeMethod<Map<dynamic, dynamic>>("CreateGPUTexture", {"id": id, "width": width, "height": height});

    if (res == null || res["texture_id"] == null || res["shared_handle"] == null) {
      throw Exception("Couldn't create GPUTexture");
    }
    int textureID = res["texture_id"] as int;
    if (textureID == -1) throw Exception("Couldn't create GPUTexture (ID is -1)");
    int sharedHandle = res["shared_handle"] as int;
    return GPUTextureInfo(height: height, sharedHandle: sharedHandle, textureID: textureID, width: width);
  }

  Future<void> _destroyGPUTexture(int id) async {
    await _channel.invokeMethod<Map<String, int>>("DestroyGPUTexture", {"id": id});
  }

  Future<void> update(int id, ffi.Pointer<ffi.Uint8> buffer, int width, int height) async {
    if (!_ids.containsKey(id)) {
      ffi.calloc.free(buffer);
      return;
    }
    _ids[id]!.value = _ids[id]!.value.copyWith(width: width, height: height);

    await _channel.invokeMethod('UpdateFrame', {
      "id": id,
      "width": width,
      "height": height,
      "buffer": buffer.address,
    });
    ffi.Pointer<ffi.Uint8> prev = _ids[id]!.value._previousBuffer;
    Future.delayed(const Duration(milliseconds: 20), () {
      if (prev != ffi.nullptr) ffi.calloc.free(prev);
    });
    _ids[id]!.value = _ids[id]!.value.copyWith(previousBuffer: buffer);
  }

  Future<void> _unregisterTexture(int id) async {
    await _channel.invokeMethod(
      "UnregisterTexture",
      {
        "id": id,
      },
    );
  }

  Widget widget(int id) {
    return ValueListenableBuilder<TextureInfo>(
      valueListenable: _ids[id]!,
      builder: (context, tex, _) {
        if (tex.handle != null) {
          return SizedBox(
            width: tex.width.toDouble(),
            height: tex.height.toDouble(),
            child: Texture(textureId: tex.handle!),
          );
        }
        return Container();
      },
    );
  }

  Widget widgetGPU(int id) {
    return ValueListenableBuilder<GPUTextureInfo>(
      valueListenable: _gpuIDS[id]!,
      builder: (context, tex, _) {
        return SizedBox(
          width: tex.width.toDouble(),
          height: tex.height.toDouble(),
          child: Texture(textureId: tex.textureID),
        );
      },
    );
  }

  ValueListenable<TextureInfo>? textureInfo(int id) => _ids[id];
  ValueListenable<GPUTextureInfo>? gpuTextureInfo(int id) => _gpuIDS[id];
}

class TextureInfo {
  int? handle;
  int width, height;
  ffi.Pointer<ffi.Uint8> _previousBuffer = ffi.nullptr;

  TextureInfo({
    required this.handle,
    required this.width,
    required this.height,
    required ffi.Pointer<ffi.Uint8> previousBuffer,
  }) {
    _previousBuffer = previousBuffer;
  }

  Size get size => Size(width.toDouble(), height.toDouble());

  TextureInfo copyWith({int? handle, int? width, int? height, ffi.Pointer<ffi.Uint8>? previousBuffer}) {
    return TextureInfo(
      handle: handle ?? this.handle,
      width: width ?? this.width,
      height: height ?? this.height,
      previousBuffer: previousBuffer ?? _previousBuffer,
    );
  }
}

class GPUTextureInfo {
  int sharedHandle;
  int textureID;
  int width, height;

  GPUTextureInfo({required this.height, required this.sharedHandle, required this.textureID, required this.width});

  Size get size => Size(width.toDouble(), height.toDouble());
}
