# Release Notes — 2025-10-24

## Features

- Enabled Memory Integrity Enforcement (ARM MTE) for macOS Apple Silicon builds when supported by your toolchain. This adds Clang's `-fsanitize=memtag` flag for Electron's macOS arm64 targets to improve memory safety.
  - Compatibility: Only applies when building with Apple Clang on macOS arm64. Automatically disabled when AddressSanitizer (ASan) is enabled.
  - Opt-out: If your Xcode/macOS SDK does not support `-fsanitize=memtag`, you can disable it via GN args:

    ```sh
    gn gen out/Testing --args='enable_memtag_on_mac_arm64=false'
    ```

  - Affected targets: Core Electron mac targets including the framework and helper apps.

