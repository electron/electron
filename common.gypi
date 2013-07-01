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
  # Settings to compile node under Windows.
  'target_defaults': {
    'target_conditions': [
      ['_target_name in ["libuv", "http_parser", "cares", "openssl", "node", "zlib"]', {
        'msvs_disabled_warnings': [
          4013,  # 'free' undefined; assuming extern returning int
          4054,  #
          4057,  # 'function' : 'volatile LONG *' differs in indirection to slightly different base types from 'unsigned long *'
          4189,  #
          4131,  # uses old-style declarator
          4133,  # incompatible types
          4152,  # function/data pointer conversion in expression
          4206,  # translation unit is empty
          4204,  # non-constant aggregate initializer
          4214,  # bit field types other than int
          4232,  # address of dllimport 'free' is not static, identity not guaranteed
          4295,  # array is too small to include a terminating null character
          4389,  # '==' : signed/unsigned mismatch
          4505,  # unreferenced local function has been removed
          4701,  # potentially uninitialized local variable 'sizew' used
          4706,  # assignment within conditional expression
          4996,  #
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'WarnAsError': 'false',
          },
        },
      }],
    ],
    'msvs_cygwin_shell': 0, # Strangely setting it to 1 would make building under cygwin fail.
    'msvs_disabled_warnings': [
      4005,  # (node.h) macro redefinition
      4201,  # (uv.h) nameless struct/union
      4800,  # (v8.h) forcing value to bool 'true' or 'false'
      4819,  # The file contains a character that cannot be represented in the current code page
    ],
  },
  'conditions': [
    # Settings to compile with clang under OS X.
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
    # Windows specific settings.
    ['OS=="win"', {
      'target_defaults': {
        'msvs_settings': {
          'VCCLCompilerTool': {
            # Programs that use the Standard C++ library must be compiled with C++
            # exception handling enabled.
            # http://support.microsoft.com/kb/154419
            'ExceptionHandling': 1,
          },
        },
      },
    }],  # OS=="win"
  ],
}
