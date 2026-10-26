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
          "include"
        ],
        "dependencies": [
          "<!(node -p \"require('node-addon-api').gyp\")"
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
          "MACOSX_DEPLOYMENT_TARGET": "11.0",
          "OTHER_CFLAGS": [
            "-fobjc-arc"
          ]
        }
      }]
    ]
  }]
}
