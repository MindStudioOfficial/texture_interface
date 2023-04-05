import 'dart:async';

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
    if (!(await textureInterface.registerGPU(textureID.value, 100, 50))) success = false;
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
    ValueListenable<GPUTextureInfo>? textureInfo1 =
        texturesInitialized ? textureInterface.gpuTextureInfo(textureIDs["first"]!) : null;
    ValueListenable<GPUTextureInfo>? textureInfo2 =
        texturesInitialized ? textureInterface.gpuTextureInfo(textureIDs["second"]!) : null;

    Widget firstTexture = texturesInitialized ? textureInterface.widgetGPU(textureIDs["first"]!) : const Placeholder();
    Widget secondTexture =
        texturesInitialized ? textureInterface.widgetGPU(textureIDs["second"]!) : const Placeholder();

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
                        Text(
                            "Internal Handle: ${info.textureID} Shared Handle: ${info.sharedHandle} Size: ${info.width} x ${info.height}")
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
                        Text(
                            "Internal Handle: ${info.textureID} Shared Handle: ${info.sharedHandle} Size: ${info.width} x ${info.height}")
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
