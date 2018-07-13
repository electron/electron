{
  'variables': {
    # Node disables the inspector unless icu is enabled. But node doesn't know
    # that we're building v8 with icu, so force it on.
    'v8_enable_inspector': 1,

    'shlib_suffix': '<(shlib_suffix_gn)',

    'is_component_build%': 'false',
  },
  'conditions': [
    ['OS=="linux"', {
      'make_global_settings': [
        ['CC', '<(llvm_dir)/bin/clang'],
        ['CXX', '<(llvm_dir)/bin/clang++'],
        ['CC.host', '$(CC)'],
        ['CXX.host', '$(CXX)'],
      ],
      'target_defaults': {
        'target_conditions': [
          ['_toolset=="target"', {
            'cflags_cc': [
              '-std=gnu++14',
              '-nostdinc++',
              '-isystem<(libcxx_dir)/trunk/include',
              '-isystem<(libcxxabi_dir)/trunk/include',
            ],
            'ldflags': [
              '-nostdlib++',
            ],
            'libraries': [
              '../../../../../../libc++.so',
            ],
          }]
        ]
      }
    }],
    ['OS=="win"', {
      'make_global_settings': [
        ['CC', '<(llvm_dir)/bin/clang-cl'],
        # Also defining CXX doesn't appear to be necessary.
      ]
    }],
  ],
  'target_defaults': {
    'target_conditions': [
      ['_target_name=="node_lib"', {
        'include_dirs': [
          '../../../v8',
          '../../../v8/include',
          '../../../third_party/icu/source/common',
          '../../../third_party/icu/source/i18n',
        ],
        'defines': [
          'EVP_CTRL_AEAD_SET_IVLEN=EVP_CTRL_GCM_SET_IVLEN',
          'EVP_CTRL_CCM_SET_TAG=EVP_CTRL_GCM_SET_TAG',
          'EVP_CTRL_AEAD_GET_TAG=EVP_CTRL_GCM_GET_TAG',
          'WIN32_LEAN_AND_MEAN',
        ],
        'conditions': [
          ['OS=="win"', {
            'conditions': [
              ['is_component_build=="true"', {
                'libraries': [
                  '-lv8.dll',
                  '-lv8_libbase.dll',
                  '-lv8_libplatform.dll',
                  '-licuuc.dll',
                  '-ldbghelp',
                ],
                'msvs_settings': {
                  # Change location of some hard-coded paths.
                  'VCLinkerTool': {
                    'AdditionalOptions!': [
                      '/WHOLEARCHIVE:<(PRODUCT_DIR)\\lib\\zlib<(STATIC_LIB_SUFFIX)',
                      '/WHOLEARCHIVE:<(PRODUCT_DIR)\\lib\\libuv<(STATIC_LIB_SUFFIX)',
                    ],
                    'AdditionalOptions': [
                      '/WHOLEARCHIVE:<(PRODUCT_DIR)\\obj\\third_party\\electron_node\\deps\\zlib\\zlib<(STATIC_LIB_SUFFIX)',
                      '/WHOLEARCHIVE:<(PRODUCT_DIR)\\obj\\third_party\\electron_node\\deps\\uv\\libuv<(STATIC_LIB_SUFFIX)',
                    ],
                  },
                },
              }]
            ],
          }, {
            'conditions': [
              ['is_component_build=="true"', {
                'libraries': [
                  '-lv8',
                  '-lv8_libbase',
                  '-lv8_libplatform',
                  '-licuuc',
                ]
              }]
            ]
          }]
        ]
      }],
    ],
  },
}
