import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:ffi/ffi.dart' as ffi;
import 'dart:ffi' as ffi;

class TextureInterface {
  static const MethodChannel _channel = MethodChannel('texture_interface');
  final Map<int, ValueNotifier<TextureInfo>> _ids = {};

  Set<int> get ids => _ids.keys.toSet();

  int getUniqueId() {
    if (_ids.isEmpty) return 0;
    return _ids.keys.reduce((a, b) => a > b ? a : b) + 1;
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

  Future<bool> unregister(int id) async {
    if (!_ids.containsKey(id)) {
      return false;
    }
    await _unregisterTexture(id);
    _ids.remove(id);
    return true;
  }

  Future<void> dispose() async {
    for (int id in _ids.keys) {
      await _unregisterTexture(id);
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

  Widget widget(int id, {FilterQuality filterQuality = FilterQuality.low}) {
    return ValueListenableBuilder<TextureInfo>(
      valueListenable: _ids[id]!,
      builder: (context, tex, _) {
        if (tex.handle != null) {
          return SizedBox(
            width: tex.width.toDouble(),
            height: tex.height.toDouble(),
            child: Texture(
              textureId: tex.handle!,
              filterQuality: filterQuality,
            ),
          );
        }
        return Container();
      },
    );
  }

  ValueListenable<TextureInfo>? textureInfo(int id) => _ids[id];
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
