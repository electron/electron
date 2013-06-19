{
  'variables': {
    'clang': 0,
    'source_root': '<!(python tools/mac/source_root.py)',
    'conditions': [
      ['OS=="mac"', {
        'clang': 1,
        'mac_sdk%': '<!(python tools/mac/find_sdk.py 10.8)',
      }],
    ],
  },
  'conditions': [
    ['clang==1', {
      'make_global_settings': [
        ['CC', '/usr/bin/clang'],
        ['CXX', '/usr/bin/clang++'],
        ['LINK', '$(CXX)'],
        ['CC.host', '$(CC)'],
        ['CXX.host', '$(CXX)'],
        ['LINK.host', '$(LINK)'],
      ],
      'target_defaults': {
        'cflags_cc': [
          '-std=c++11',
        ],
        'xcode_settings': {
          'CC': '/usr/bin/clang',
          'LDPLUSPLUS': '/usr/bin/clang++',
          'OTHER_CPLUSPLUSFLAGS': [
            '$(inherited)', '-std=gnu++11'
          ],
          'OTHER_CFLAGS': [
            '-fcolor-diagnostics',
          ],

          'SDKROOT': 'macosx<(mac_sdk)',     # -isysroot
          'GCC_C_LANGUAGE_STANDARD': 'c99',  # -std=c99
        },
      },
    }],  # clang==1
  ],
}
