{
  'variables': {
    'NAPI_VERSION%': "<!(node -p \"process.versions.napi\")",
  },
  'targets': [{
    'target_name': 'binding',
    'conditions': [
      ['NAPI_VERSION!=""', { 'defines': ['NAPI_VERSION=<@(NAPI_VERSION)'] } ],
      ['OS=="mac"', {
        'cflags+': ['-fvisibility=hidden'],
        'xcode_settings': {
          'CLANG_CXX_LIBRARY': 'libc++',
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
          'MACOSX_DEPLOYMENT_TARGET': '10.13',
        }
      }],
      ["OS=='win'", {
        "defines": [
          "_HAS_EXCEPTIONS=1"
        ],
        "msvs_settings": {
          "VCCLCompilerTool": {
            "ExceptionHandling": 1,
          },
        },
      }],
    ],
    'defines': [ 'NAPI_CPP_EXCEPTIONS' ],
    'cflags!': [ '-fno-exceptions' ],
    'cflags_cc!': [ '-fno-exceptions' ],
    'include_dirs': ["<!(node -p \"require('node-addon-api').include_dir\")"],
    'cflags': [ '-Werror', '-Wall', '-Wextra', '-Wpedantic', '-Wunused-parameter' ],
    'cflags_cc': [ '-Werror', '-Wall', '-Wextra', '-Wpedantic', '-Wunused-parameter' ],
    'sources': [
      'binding.cc',
    ],
  }],
}
