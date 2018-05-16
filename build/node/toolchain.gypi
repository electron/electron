{
  'variables': {
    'llvm_dir': '<(chromium_src_dir)/third_party/llvm-build/Release+Asserts',
  },
  'conditions': [
    ['clang==1', {
      'make_global_settings': [
        ['CC', '<(llvm_dir)/bin/clang'],
        ['CXX', '<(llvm_dir)/bin/clang++'],
        ['CC.host', '$(CC)'],
        ['CXX.host', '$(CXX)'],
      ],
      'target_defaults': {
        'target_conditions': [
          ['OS=="linux" and _toolset=="target"', {
            'cflags_cc': [
              '-std=gnu++14',
              '-nostdinc++',
              '-isystem<(chromium_src_dir)/buildtools/third_party/libc++/trunk/include',
              '-isystem<(chromium_src_dir)/buildtools/third_party/libc++abi/trunk/include',
            ],
            'ldflags': [
              '-nostdlib++',
            ],
          }],
          ['OS=="linux" and _toolset=="host"', {
            'cflags_cc': [
              '-std=gnu++14',
            ],
          }],
        ],
      },
    }],  # clang==1
  ],
}
