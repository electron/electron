{
  'variables': {
    # Clang stuff.
    'make_clang_dir%': 'vendor/llvm-build/Release+Asserts',
    # Set this to true when building with Clang.
    'clang%': 1,
    'conditions': [
      ['OS=="win"', {
        # Do not use Clang on Windows.
        'clang%': 0,
      }],
    ],
  },
  'conditions': [
    ['clang==1', {
      'make_global_settings': [
        ['CC', '<(make_clang_dir)/bin/clang'],
        ['CXX', '<(make_clang_dir)/bin/clang++'],
        ['CC.host', '$(CC)'],
        ['CXX.host', '$(CXX)'],
      ],
      'target_defaults': {
        'cflags_cc': [
          '-std=c++11',
        ],
        'xcode_settings': {
          'CC': '<(make_clang_dir)/bin/clang',
          'LDPLUSPLUS': '<(make_clang_dir)/bin/clang++',
          'OTHER_CFLAGS': [
            '-fcolor-diagnostics',
          ],

          'GCC_C_LANGUAGE_STANDARD': 'c99',  # -std=c99
          'CLANG_CXX_LIBRARY': 'libc++',  # -stdlib=libc++
          'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',  # -std=c++11
        },
        'target_conditions': [
          ['_type in ["executable", "shared_library"]', {
            'xcode_settings': {
              # On some machines setting CLANG_CXX_LIBRARY doesn't work for
              # linker.
              'OTHER_LDFLAGS': [ '-stdlib=libc++' ],
            },
          }],
        ],
      },
    }],  # clang==1
  ],
}
