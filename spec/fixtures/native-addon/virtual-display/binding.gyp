{
  "targets": [{
    "target_name": "virtual_display",
    "conditions": [
      ['OS=="mac"', {
        "sources": [
          "src/addon.mm", 
          "src/VirtualDisplayBridge.m"
        ],
        "include_dirs": [
          "<!@(node -p \"require('node-addon-api').include\")",
          "include",
          "build_swift"
        ],
        "dependencies": [
          "<!(node -p \"require('node-addon-api').gyp\")"
        ],
        "libraries": [
          "<(PRODUCT_DIR)/libVirtualDisplay.dylib"
        ],
        "defines": [
          "NODE_ADDON_API_CPP_EXCEPTIONS"
        ],
        "cflags!": [ "-fno-exceptions" ],
        "cflags_cc!": [ "-fno-exceptions" ],
        "xcode_settings": {
          "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
          "CLANG_ENABLE_OBJC_ARC": "YES",
          "CLANG_CXX_LIBRARY": "libc++",
          "SWIFT_OBJC_BRIDGING_HEADER": "include/VirtualDisplayBridge.h",
          "SWIFT_VERSION": "5.0",
          "SWIFT_OBJC_INTERFACE_HEADER_NAME": "virtual_display-Swift.h",
          "MACOSX_DEPLOYMENT_TARGET": "11.0",
          "OTHER_CFLAGS": [
            "-ObjC++",
            "-fobjc-arc"
          ],
          "OTHER_LDFLAGS": [
            "-lswiftCore",
            "-lswiftFoundation", 
            "-lswiftObjectiveC",
            "-lswiftDarwin",
            "-lswiftDispatch",
            "-L/usr/lib/swift",
            "-Wl,-rpath,/usr/lib/swift",
            "-Wl,-rpath,@loader_path"
          ]
        },
        "actions": [
          {
            "action_name": "build_swift",
            "inputs": [
              "src/VirtualDisplay.swift",
              "src/Dummy.swift",
              "include/VirtualDisplayBridge.h"
            ],
            "outputs": [
              "build_swift/libVirtualDisplay.dylib",
              "build_swift/virtual_display-Swift.h"
            ],
            "action": [
              "swiftc",
              "src/VirtualDisplay.swift",
              "src/Dummy.swift",
              "-import-objc-header", "include/VirtualDisplayBridge.h",
              "-emit-objc-header-path", "./build_swift/virtual_display-Swift.h",
              "-emit-library", "-o", "./build_swift/libVirtualDisplay.dylib",
              "-emit-module", "-module-name", "virtual_display",
              "-module-link-name", "VirtualDisplay"
            ]
          },
          {
            "action_name": "copy_swift_lib",
            "inputs": [
              "<(module_root_dir)/build_swift/libVirtualDisplay.dylib"
            ],
            "outputs": [
              "<(PRODUCT_DIR)/libVirtualDisplay.dylib"
            ],
            "action": [
              "sh",
              "-c",
              "cp -f <(module_root_dir)/build_swift/libVirtualDisplay.dylib <(PRODUCT_DIR)/libVirtualDisplay.dylib && install_name_tool -id @rpath/libVirtualDisplay.dylib <(PRODUCT_DIR)/libVirtualDisplay.dylib"
            ]
          }
        ]
      }]
    ]
  }]
}