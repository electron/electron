{
  'includes': [
    'toolchain.gypi',
    'brightray/brightray.gypi',
  ],
  'variables': {
    # Tell crashpad to build as external project.
    'crashpad_dependencies': 'external',
    # Required by breakpad.
    'os_bsd': 0,
    'chromeos': 0,
    # Reflects node's config.gypi.
    'component%': 'static_library',
    'debug_http2': 'false',
    'debug_nghttp2': 'false',
    'python': 'python',
    'openssl_fips': '',
    'openssl_no_asm': 1,
    'use_openssl_def': 0,
    'OPENSSL_PRODUCT': 'libopenssl.a',
    'node_release_urlbase': 'https://atom.io/download/electron',
    'node_byteorder': '<!(node <(DEPTH)/tools/get-endianness.js)',
    'node_target_type': 'shared_library',
    'node_install_npm': 'false',
    'node_prefix': '',
    'node_shared': 'true',
    'node_shared_cares': 'false',
    'node_shared_http_parser': 'false',
    'node_shared_libuv': 'false',
    'node_shared_openssl': 'false',
    'node_shared_v8': 'true',
    'node_shared_zlib': 'false',
    'node_tag': '',
    'node_use_dtrace': 'false',
    'node_use_etw': 'false',
    'node_use_mdb': 'false',
    'node_use_openssl': 'true',
    'node_use_perfctr': 'false',
    'node_use_v8_platform': 'false',
    'node_use_bundled_v8': 'false',
    'node_enable_d8': 'false',
    'uv_library': 'static_library',
    'uv_parent_path': 'vendor/node/deps/uv',
    'uv_use_dtrace': 'false',
    'V8_BASE': '',
    'v8_postmortem_support': 'false',
    'v8_enable_i18n_support': 'false',
    'v8_enable_inspector': '1',
  },
  # Settings to compile node under Windows.
  'target_defaults': {
    'target_conditions': [
      ['_target_name in ["libuv", "http_parser", "openssl", "openssl-cli", "cares", "node", "zlib", "nghttp2"]', {
        'msvs_disabled_warnings': [
          4003,  # not enough actual parameters for macro 'V'
          4013,  # 'free' undefined; assuming extern returning int
          4018,  # signed/unsigned mismatch
          4054,  #
          4055,  # 'type cast' : from data pointer 'void *' to function pointer
          4057,  # 'function' : 'volatile LONG *' differs in indirection to slightly different base types from 'unsigned long *'
          4065,  # switch statement contains 'default' but no 'case' labels
          4189,  #
          4131,  # uses old-style declarator
          4133,  # incompatible types
          4146,  # unary minus operator applied to unsigned type, result still unsigned
          4164,  # intrinsic function not declared
          4152,  # function/data pointer conversion in expression
          4206,  # translation unit is empty
          4204,  # non-constant aggregate initializer
          4210,  # nonstandard extension used : function given file scope
          4214,  # bit field types other than int
          4232,  # address of dllimport 'free' is not static, identity not guaranteed
          4291,  # no matching operator delete found
          4295,  # array is too small to include a terminating null character
          4311,  # 'type cast': pointer truncation from 'void *const ' to 'unsigned long'
          4389,  # '==' : signed/unsigned mismatch
          4456,  # declaration of 'm' hides previous local declaration
          4457,  # declaration of 'message' hides function parameter
          4459,  # declaration of 'wq' hides global declaration
          4477,  # format string '%.*s' requires an argument of type 'int'
          4505,  # unreferenced local function has been removed
          4701,  # potentially uninitialized local variable 'sizew' used
          4703,  # potentially uninitialized local pointer variable 'req' used
          4706,  # assignment within conditional expression
          4804,  # unsafe use of type 'bool' in operation
          4996,  # this function or variable may be unsafe.
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'WarnAsError': 'false',
          },
        },
        'xcode_settings': {
          'GCC_TREAT_WARNINGS_AS_ERRORS': 'NO',
          'WARNING_CFLAGS': [
            '-Wno-unknown-warning-option',
            '-Wno-parentheses-equality',
            '-Wno-unused-function',
            '-Wno-sometimes-uninitialized',
            '-Wno-pointer-sign',
            '-Wno-sign-compare',
            '-Wno-string-plus-int',
            '-Wno-unused-variable',
            '-Wno-deprecated-declarations',
            '-Wno-return-type',
            '-Wno-gnu-folding-constant',
            '-Wno-shift-negative-value',
            '-Wno-varargs', # https://git.io/v6Olj
          ],
        },
        'conditions': [
          ['OS=="linux"', {
            'cflags': [
              '-Wno-unused-function',
              '-Wno-unused-variable',
              '-Wno-unused-value',
              '-Wno-deprecated-declarations',
              '-Wno-return-type',
              '-Wno-format',
              '-Wno-varargs', # https://git.io/v6Olj
              # Required when building as shared library.
              '-fPIC',
            ],
          }],
          ['OS=="linux" and clang==1', {
            'cflags': [
              '-Wno-pointer-sign',
              '-Wno-parentheses-equality',
              '-Wno-sometimes-uninitialized',
              '-Wno-string-plus-int',
              '-Wno-shift-negative-value',
              '-Wno-reserved-user-defined-literal',
            ],
          }],
        ],
      }],
      ['_target_name=="node"', {
        'include_dirs': [
          '<(libchromiumcontent_src_dir)',
          '<(libchromiumcontent_src_dir)/third_party/icu/source/common',
          '<(libchromiumcontent_src_dir)/third_party/icu/source/i18n',
          '<(libchromiumcontent_src_dir)/v8',
          '<(libchromiumcontent_src_dir)/v8/include',
        ],
        'defines': [
          # Export V8 symbols from node.dll / libnode.so
          'BUILDING_V8_SHARED',
          'BUILDING_V8_PLATFORM_SHARED',
          'BUILDING_V8_BASE_SHARED',
        ],
        'conditions': [
          ['OS=="mac" and libchromiumcontent_component==0', {
            # -all_load is the "whole-archive" on macOS.
            'xcode_settings': {
              'OTHER_LDFLAGS': [ '-Wl,-all_load' ],
            },
          }],
          ['OS=="win"', {
            # Fix passing fd across modules, see |osfhandle.h| for more.
            'sources': [
              '<(DEPTH)/atom/node/osfhandle.cc',
              '<(DEPTH)/atom/node/osfhandle.h',
            ],
            'include_dirs': [
              '<(DEPTH)/atom/node',
            ],
            'libraries': [
              # Node is using networking API but linking with this itself.
              '-lwinmm.lib',
              # Needed by V8.
              '-ldbghelp.lib',
              '-lshlwapi.lib',
            ],
          }],
          ['OS=="linux" and libchromiumcontent_component==0', {
            # Prevent the linker from stripping symbols.
            'ldflags': [
              '-Wl,--whole-archive',
              '<@(libchromiumcontent_v8_libraries)',
              '-Wl,--no-whole-archive',
            ],
          }, {
            'libraries': [ '<@(libchromiumcontent_v8_libraries)' ],
          }],
        ],
      }],
      ['_target_name=="openssl"', {
        'xcode_settings': {
          'DEAD_CODE_STRIPPING': 'YES',  # -Wl,-dead_strip
          'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES',
          'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES',
        },
        'cflags': [
          '-fvisibility=hidden',
        ],
      }],
      ['_target_name=="libuv"', {
        'conditions': [
          ['OS=="win"', {
            # Expose libuv's symbols.
            'defines': [
              'BUILDING_UV_SHARED=1',
            ],
          }],  # OS=="win"
        ],
      }],
      ['_target_name.startswith("crashpad")', {
        'conditions': [
          ['OS=="mac"', {
            'xcode_settings': {
              'WARNING_CFLAGS': [
                '-Wno-unused-private-field',
                '-Wno-address-of-packed-member',
              ],
            },
          }],  # OS=="mac"
        ],
      }],
      ['_target_name.startswith("breakpad") or _target_name in ["crash_report_sender", "dump_syms"]', {
        'conditions': [
          ['OS=="mac"', {
            'xcode_settings': {
              'WARNING_CFLAGS': [
                '-Wno-deprecated-declarations',
                '-Wno-deprecated-register',
                '-Wno-unused-private-field',
                '-Wno-unused-function',
              ],
            },
          }],  # OS=="mac"
          ['OS=="linux"', {
            'cflags': [
              '-Wno-empty-body',
            ],
          }],  # OS=="linux"
          ['OS=="win"', {
            'msvs_disabled_warnings': [
              # unreferenced local function has been removed.
              4505,
            ],
          }],  # OS=="win"
        ],
      }],
    ],
    'msvs_cygwin_shell': 0, # Strangely setting it to 1 would make building under cygwin fail.
    'msvs_disabled_warnings': [
      4005,  # (node.h) macro redefinition
      4091,  # (node_extern.h) '__declspec(dllimport)' : ignored on left of 'node::Environment' when no variable is declared
      4099,  # (pdf_render_settings.h) type name first seen using 'class' now seen using 'struct'
      4189,  # local variable is initialized but not referenced
      4201,  # (uv.h) nameless struct/union
      4267,  # conversion from 'size_t' to 'int', possible loss of data
      4302,  # (atldlgs.h) 'type cast': truncation from 'LPCTSTR' to 'WORD'
      4458,  # (atldlgs.h) declaration of 'dwCommonButtons' hides class member
      4503,  # decorated name length exceeded, name was truncated
      4530,  # C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
      4577,  # 'noexcept' used with no exception handling mode specified
      4714,  # (atomicstring.h) function marked as __forceinline not inlined
      4800,  # (v8.h) forcing value to bool 'true' or 'false'
      4819,  # The file contains a character that cannot be represented in the current code page
      4838,  # (atlgdi.h) conversion from 'int' to 'UINT' requires a narrowing conversion
      4996,  # (atlapp.h) 'GetVersionExW': was declared deprecated
    ],
  },
  'conditions': [
    # The breakdpad on Windows assumes Debug_x64 and Release_x64 configurations.
    ['OS=="win"', {
      'target_defaults': {
        'configurations': {
          'Debug_x64': {
          },
          'Release_x64': {
          },
        },
      },
    }],  # OS=="win"
    # The breakdpad on Mac assumes Release_Base configuration.
    ['OS=="mac"', {
      'target_defaults': {
        'configurations': {
          'Release_Base': {
          },
        },
      },
    }],  # OS=="mac"
  ],
}
