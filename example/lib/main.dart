import 'dart:async';
import 'dart:ffi' as ffi;

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:texture_interface/texture_interface.dart';

// global textureInterface instance
late TextureInterface textureInterface;

// hold on to your textureIDs somewhere
Map<String, int> textureIDs = {"first": 1, "second": 2};

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

void main() => runApp(const Main());

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

    WidgetsFlutterBinding.ensureInitialized().addPostFrameCallback((_) async {
      texturesInitialized = await initTextures();
      setState(() {});

      if (!texturesInitialized) return;

      timer = Timer.periodic(const Duration(milliseconds: 33), (timer) async {
        final width = timer.tick.isEven ? 1920 : 1280;
        final height = timer.tick.isEven ? 1080 : 720;
        final value = timer.tick.isEven ? 0xFF000000 : 0xFF151515;

        final sw = Stopwatch()..start();
        final pFirstBuffer = await textureInterface.getBuffer(textureIDs["first"]!, width, height);
        debugPrint("getBuffer took: ${sw.elapsedMicroseconds} us");
        sw.reset();

        if (pFirstBuffer == null) {
          debugPrint("Failed to get buffer for first texture");
          return;
        }

        pFirstBuffer
            .cast<ffi.Uint32>()
            .asTypedList(width * height)
            .fillRange(0, width * height, value);
        debugPrint("fillRange took: ${sw.elapsedMicroseconds} us");
        sw.reset();

        await textureInterface.update(textureIDs["first"]!, pFirstBuffer, width, height);
        debugPrint("update took: ${sw.elapsedMicroseconds} us");
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
    ValueListenable<TextureInfo>? textureInfo1 = texturesInitialized
        ? textureInterface.textureInfo(textureIDs["first"]!)
        : null;
    ValueListenable<TextureInfo>? textureInfo2 = texturesInitialized
        ? textureInterface.textureInfo(textureIDs["second"]!)
        : null;

    Widget firstTexture = texturesInitialized
        ? textureInterface.widget(textureIDs["first"]!)
        : const Placeholder();
    Widget secondTexture = texturesInitialized
        ? textureInterface.widget(textureIDs["second"]!)
        : const Placeholder();

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
                                child: FittedBox(fit: BoxFit.contain, child: firstTexture),
                              )
                            : const Expanded(child: Placeholder()),
                        Text(
                          "Internal Handle: ${info.handle} Size: ${info.width} x ${info.height}",
                        ),
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
                                child: FittedBox(fit: BoxFit.contain, child: secondTexture),
                              )
                            : const Expanded(child: Placeholder()),
                        Text(
                          "Internal Handle: ${info.handle} Size: ${info.width} x ${info.height}",
                        ),
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
