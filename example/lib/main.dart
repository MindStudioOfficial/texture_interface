import 'dart:async';
import 'dart:ffi' as ffi;
import 'package:ffi/ffi.dart' as ffi;

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:texture_interface/texture_interface.dart';

// global textureInterface instance
late TextureInterface textureInterface;

// hold on to your textureIDs somewhere
Map<String, int> textureIDs = {
  "first": 1,
  "second": 2,
};

Future<bool> initTextures() async {
  // initialize the instance
  textureInterface = TextureInterface();
  bool success = true;

  // register all textures with an id
  for (MapEntry<String, int> textureID in textureIDs.entries) {
    if (!(await textureInterface.register(textureID.value))) success = false;
  }

  return success;
}

void main() {
  runApp(const Main());
}

class Main extends StatefulWidget {
  const Main({super.key});

  @override
  State<Main> createState() => _MainState();
}

class _MainState extends State<Main> {
  bool texturesInitialized = false;
  Timer? timer;
  @override
  void initState() {
    super.initState();

    WidgetsFlutterBinding.ensureInitialized().addPostFrameCallback((timeStamp) async {
      texturesInitialized = await initTextures();
      setState(() {});
      if (!texturesInitialized) return;
      timer = Timer.periodic(const Duration(milliseconds: 100), (timer) {
        int width = 100;
        int height = 50;
        ffi.Pointer<ffi.Uint8> pFirstBuffer = ffi.malloc.call<ffi.Uint8>(width * height * 4);

        for (int i = 0; i < width * height; i++) {
          int byte = i * 4;
          pFirstBuffer.elementAt(byte).value = timer.tick;
          pFirstBuffer.elementAt(byte + 1).value = timer.tick + 50;
          pFirstBuffer.elementAt(byte + 2).value = timer.tick + 100;
          pFirstBuffer.elementAt(byte + 3).value = 255;
        }

        textureInterface.update(textureIDs["first"]!, pFirstBuffer, width, height);
      });
    });
  }

  @override
  void dispose() {
    timer?.cancel();

    textureInterface.dispose();

    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    ValueListenable<TextureInstance>? textureInfo1 =
        texturesInitialized ? textureInterface.textureInfo(textureIDs["first"]!) : null;
    ValueListenable<TextureInstance>? textureInfo2 =
        texturesInitialized ? textureInterface.textureInfo(textureIDs["second"]!) : null;

    Widget firstTexture = texturesInitialized ? textureInterface.widget(textureIDs["first"]!) : const Placeholder();
    Widget secondTexture = texturesInitialized ? textureInterface.widget(textureIDs["second"]!) : const Placeholder();

    return MaterialApp(
      home: Scaffold(
        body: Row(
          mainAxisSize: MainAxisSize.max,
          children: [
            if (textureInfo1 != null)
              Expanded(
                child: ValueListenableBuilder(
                  valueListenable: textureInfo1,
                  builder: (context, info, _) {
                    return Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        info.width > 0 && info.height > 0
                            ? Expanded(
                                child: FittedBox(
                                  fit: BoxFit.contain,
                                  child: firstTexture,
                                ),
                              )
                            : const Expanded(child: Placeholder()),
                        Text("Internal Handle: ${info.handle} Size: ${info.width} x ${info.height}")
                      ],
                    );
                  },
                ),
              ),
            if (textureInfo2 != null)
              Expanded(
                child: ValueListenableBuilder(
                  valueListenable: textureInfo2,
                  builder: (context, info, _) {
                    return Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        info.width > 0 && info.height > 0
                            ? Expanded(
                                child: FittedBox(
                                  fit: BoxFit.contain,
                                  child: secondTexture,
                                ),
                              )
                            : const Expanded(child: Placeholder()),
                        Text("Internal Handle: ${info.handle} Size: ${info.width} x ${info.height}")
                      ],
                    );
                  },
                ),
              ),
          ],
        ),
      ),
    );
  }
}
