import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'dart:ffi' as ffi;

class TextureInterface {
  static const MethodChannel _channel = MethodChannel('texture_interface');
  final Map<int, ValueNotifier<TextureInfo>> _ids = {};

  Set<int> get ids => _ids.keys.toSet();

  int getUniqueId() => _ids.isEmpty ? 0 : _ids.keys.reduce((a, b) => a > b ? a : b) + 1;

  Future<bool> register(int id) async {
    if (_ids.containsKey(id)) return false;

    int texId = await _registerTexture(id);
    _ids.addAll({id: ValueNotifier<TextureInfo>(TextureInfo(handle: texId, width: 0, height: 0))});
    return true;
  }

  Future<ffi.Pointer<ffi.Uint8>?> getBuffer(int id, int width, int height) async {
    if (!_ids.containsKey(id)) return null;

    final bufferAddress = await _channel.invokeMethod<int>('GetBuffer', {
      "id": id,
      "width": width,
      "height": height,
    });

    if (bufferAddress == null || bufferAddress == 0) {
      return null;
    }

    return ffi.Pointer<ffi.Uint8>.fromAddress(bufferAddress);
  }

  Future<bool> unregister(int id) async {
    if (!_ids.containsKey(id)) return false;
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

  static Future<String?> get platformVersion => _channel.invokeMethod('getPlatformVersion');

  Size? texSize(int id) {
    final info = _ids[id];
    if (info != null) {
      return Size(info.value.width.toDouble(), info.value.height.toDouble());
    }
    return null;
  }

  Future<int> _registerTexture(int id) async =>
      await _channel.invokeMethod("RegisterTexture", {"id": id});

  Future<bool> update(int id, ffi.Pointer<ffi.Uint8> buffer, int width, int height) async {
    if (!_ids.containsKey(id)) return false;

    _ids[id]!.value = _ids[id]!.value.copyWith(width: width, height: height);

    final success = await _channel.invokeMethod<bool>('UpdateFrame', {
      "id": id,
      "width": width,
      "height": height,
      "buffer": buffer.address,
    });
    return success ?? false;
  }

  Future<void> _unregisterTexture(int id) async =>
      await _channel.invokeMethod("UnregisterTexture", {"id": id});

  Widget widget(int id, {FilterQuality filterQuality = .low}) =>
      ValueListenableBuilder<TextureInfo>(
        valueListenable: _ids[id]!,
        builder: (context, tex, _) => tex.handle != null
            ? SizedBox(
                width: tex.width.toDouble(),
                height: tex.height.toDouble(),
                child: Texture(textureId: tex.handle!, filterQuality: filterQuality),
              )
            : Container(),
      );

  ValueListenable<TextureInfo>? textureInfo(int id) => _ids[id];
}

class TextureInfo {
  int? handle;
  int width, height;

  TextureInfo({required this.handle, required this.width, required this.height});

  Size get size => Size(width.toDouble(), height.toDouble());

  TextureInfo copyWith({
    int? handle,
    int? width,
    int? height,
    ffi.Pointer<ffi.Uint8>? previousBuffer,
  }) => TextureInfo(
    handle: handle ?? this.handle,
    width: width ?? this.width,
    height: height ?? this.height,
  );
}
