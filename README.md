# texture_interface

A flutter plugin to directly Interface with the Texture Widget on Windows using a Pointer

| Android | iOS | Linux | Windows | MacOS | Web |
| ------- | --- | ----- | ------- | ----- | --- |
| ❌       | ❌   | ❌     | ✅       | ❌     | ❌   |

## How to use

### Initialize 
```dart
late Texture_interface tr;
tr = Texture_interface();
```

### Initialize a new Texture with an ID
```dart
int id = 0;
bool scuccess = await tr.register(id);
```

### Update the pixelbuffer

```dart
int id = 0;
ffi.Pointer<ffi.Uint8> bytes = ...; // use ffi.calloc or get an external Pointer
int width = 1920;
int height = 1080;
tr.update(id, bytes, width, height);

// the last Pointer is automatically freed when receiving a new buffer
```

### Display the Texture in your Widgettree 
```dart
int id = 0;

// null if id does not exist
Widget? texWidget = tr.widget(id);

```

### Dispose 

```dart
// automatically unregisters all textures
await tr.dispose();
```