# Native VideoFrame fixtures

Run the focused spec with a build containing the proposed API:

```sh
npm test -- --files spec/api-native-video-frame.spec.ts
```

Invalid-input checks run without native Canvas storage. The native cases require
Linux and a working hardware GLES context; they are explicitly enabled:

```sh
ELECTRON_NATIVE_FRAME_HARDWARE_TESTS=1 npm test -- --files spec/api-native-video-frame.spec.ts
```

Run the standalone hardware fixture on the desired display backend:

```sh
ELECTRON_NATIVE_FRAME_HARDWARE_TESTS=1 ../out/Default/electron --ozone-platform=wayland spec/fixtures/api/native-video-frame/main.cjs
ELECTRON_NATIVE_FRAME_HARDWARE_TESTS=1 ELECTRON_NATIVE_FRAME_LAYOUT=tiled ../out/Default/electron --ozone-platform=wayland spec/fixtures/api/native-video-frame/main.cjs
```

A supported native Canvas allocation is required. An unsupported GPU or buffer
layout is a test failure, not a silent software fallback. The Linux-only hardware
fixture uses `/proc` and file descriptors; Windows/macOS native-consumer coverage
is still needed.

The native checks cover exact pixels after early `VideoFrame.close()` and Canvas
redraw, document ownership, single-use IDs, bounded pending exports, cancellation,
navigation, descriptor release, 20 reuse cycles and timeout cleanup. Every mode
requires zero compositor paint events. CPU readback is only used to verify pixels.

The initial API allows one acquisition in progress per renderer context and four
unresolved IDs per document. Native encoding and additional pipeline/fence modes
are outside this change.
