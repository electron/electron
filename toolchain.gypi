{
  'variables': {
    # Clang stuff.
    'make_clang_dir%': 'vendor/llvm-build/Release+Asserts',
    # Set this to true when building with Clang.
    'clang%': 1,

    # Set this to the absolute path to sccache when building with sccache
    'cc_wrapper%': '',

    # Path to mips64el toolchain.
    'make_mips64_dir%': 'vendor/gcc-4.8.3-d197-n64-loongson/usr',

    'variables': {
      # The minimum macOS SDK version to use.
      'mac_sdk_min%': '10.12',

      # Set ARM architecture version.
      'arm_version%': 7,

      # Set NEON compilation flags.
      'arm_neon%': 1,

      # Abosulte path to source root.
      'source_root%': '<!(node <(DEPTH)/tools/atom_source_root.js)',
    },

    # Copy conditionally-set variables out one scope.
    'mac_sdk_min%': '<(mac_sdk_min)',
    'arm_version%': '<(arm_version)',
    'arm_neon%': '<(arm_neon)',
    'source_root%': '<(source_root)',

    # Variables to control Link-Time Optimization (LTO).
    'use_lto%': 0,
    'use_lto_o2%': 0,

    'conditions': [
      # Do not use Clang on Windows or when building for mips64el.
      ['OS=="win" or target_arch=="mips64el"', {
        'clang%': 0,
      }],  # OS=="win"

      # Search for the available version of SDK.
      ['OS=="mac"', {
        'mac_sdk%': '<!(python <(DEPTH)/tools/mac/find_sdk.py <(mac_sdk_min))',
      }],

      ['OS=="linux"', {
        'variables': {
          # The system libdir used for this ABI.
          'system_libdir%': 'lib',

          # Setting the path to sysroot.
          'conditions': [
            ['target_arch=="arm"', {
              # sysroot needs to be an absolute path otherwise it generates
              # incorrect results when passed to pkg-config
              'sysroot%': '<(source_root)/vendor/debian_stretch_arm-sysroot',
            }],
            ['target_arch=="arm64"', {
              'sysroot%': '<(source_root)/vendor/debian_stretch_arm64-sysroot',
            }],
            ['target_arch=="ia32"', {
              'sysroot%': '<(source_root)/vendor/debian_stretch_i386-sysroot',
            }],
            ['target_arch=="x64"', {
              'sysroot%': '<(source_root)/vendor/debian_stretch_amd64-sysroot',
            }],
            ['target_arch=="mips64el"', {
              'sysroot%': '<(source_root)/vendor/debian_jessie_mips64-sysroot',
            }],
          ],
        },
        # Copy conditionally-set variables out one scope.
        'sysroot%': '<(sysroot)',
        'system_libdir%': '<(system_libdir)',

        # Redirect pkg-config to search from sysroot.
        'pkg-config%': '<(source_root)/tools/linux/pkg-config-wrapper "<(sysroot)" "<(target_arch)" "<(system_libdir)"',
      }],

      # Set default compiler flags depending on ARM version.
      ['arm_version==6', {
        'arm_arch%': 'armv6',
        'arm_tune%': '',
        'arm_fpu%': 'vfp',
        'arm_float_abi%': 'softfp',
        'arm_thumb%': 0,
      }],  # arm_version==6
      ['arm_version==7', {
        'arm_arch%': 'armv7-a',
        'arm_tune%': 'generic-armv7-a',
        'conditions': [
          ['arm_neon==1', {
            'arm_fpu%': 'neon',
          }, {
            'arm_fpu%': 'vfpv3-d16',
          }],
        ],
        'arm_float_abi%': 'hard',
        'arm_thumb%': 1,
      }],  # arm_version==7
    ],
  },
  'conditions': [
    # Setup cc_wrapper
    ['cc_wrapper!=""', {
      'make_global_settings': [
        ['CC_wrapper', '<(cc_wrapper)'],
        ['CXX_wrapper', '<(cc_wrapper)'],
        ['CC.host_wrapper', '<(cc_wrapper)'],
        ['CXX.host_wrapper', '<(cc_wrapper)']
      ],
    }],
    # Setup building with clang.
    ['clang==1', {
      'make_global_settings': [
        ['CC', '<(make_clang_dir)/bin/clang'],
        ['CXX', '<(make_clang_dir)/bin/clang++'],
        ['CC.host', '$(CC)'],
        ['CXX.host', '$(CXX)'],
      ],
      'target_defaults': {
        'xcode_settings': {
          'CC': '<(make_clang_dir)/bin/clang',
          'LDPLUSPLUS': '<(make_clang_dir)/bin/clang++',
          'OTHER_CFLAGS': [
            '-fcolor-diagnostics',
          ],

          'GCC_C_LANGUAGE_STANDARD': 'c99',  # -std=c99
          'CLANG_CXX_LIBRARY': 'libc++',  # -stdlib=libc++
          'CLANG_CXX_LANGUAGE_STANDARD': 'c++14',  # -std=c++14
        },
        'target_conditions': [
          ['_target_name in ["electron", "brightray"]', {
            'conditions': [
              ['OS=="mac"', {
                'xcode_settings': {
                  'OTHER_CFLAGS': [
                    "-Xclang",
                    "-load",
                    "-Xclang",
                    "<(source_root)/<(make_clang_dir)/lib/libFindBadConstructs.dylib",
                    "-Xclang",
                    "-add-plugin",
                    "-Xclang",
                    "find-bad-constructs",
                  ],
                },
              }, {  # OS=="mac"
                'cflags_cc': [
                  "-Xclang",
                  "-load",
                  "-Xclang",
                  "<(source_root)/<(make_clang_dir)/lib/libFindBadConstructs.so",
                  "-Xclang",
                  "-add-plugin",
                  "-Xclang",
                  "find-bad-constructs",
                ],
              }],
            ],
          }],
          ['OS=="mac" and _type in ["executable", "shared_library"]', {
            'xcode_settings': {
              # On some machines setting CLANG_CXX_LIBRARY doesn't work for
              # linker.
              'OTHER_LDFLAGS': [ '-stdlib=libc++' ],
            },
          }],
          ['OS=="linux" and _toolset=="target"', {
            'cflags_cc': [
              '-std=c++14',
              '-nostdinc++',
              '-isystem<(libchromiumcontent_src_dir)/buildtools/third_party/libc++/trunk/include',
              '-isystem<(libchromiumcontent_src_dir)/buildtools/third_party/libc++abi/trunk/include',
            ],
            'ldflags': [
              '-nostdlib++',
            ],
          }],
          ['OS=="linux" and _toolset=="host"', {
            'cflags_cc': [
              '-std=c++14',
            ],
          }],
        ],
      },
    }],  # clang==1

    ['target_arch=="mips64el"', {
      'make_global_settings': [
        ['CC', '<(make_mips64_dir)/bin/mips64el-redhat-linux-gcc'],
        ['CXX', '<(make_mips64_dir)/bin/mips64el-redhat-linux-g++'],
        ['CC.host', '$(CC)'],
        ['CXX.host', '$(CXX)'],
      ],
      'target_defaults': {
        'cflags_cc': [
          '-std=c++14',
        ],
      },
    }],

    # Specify the SDKROOT.
    ['OS=="mac"', {
      'target_defaults': {
        'xcode_settings': {
          'SDKROOT': 'macosx<(mac_sdk)',  # -isysroot
        },
      },
    }],

    # Setup sysroot environment.
    ['OS=="linux" and target_arch in ["arm", "ia32", "x64", "arm64", "mips64el"]', {
      'target_defaults': {
        'target_conditions': [
          ['_toolset=="target"', {
            # Do not use 'cflags' to make sure sysroot is appended at last.
            'cflags_cc': [
              '--sysroot=<(sysroot)',
            ],
            'cflags_c': [
              '--sysroot=<(sysroot)',
            ],
            'ldflags': [
              '--sysroot=<(sysroot)',
              '<!(<(source_root)/tools/linux/sysroot_ld_path.sh <(sysroot))',
            ],
          }]
        ],
      },
    }],  # sysroot

    # Setup cross-compilation on Linux.
    ['OS=="linux"', {
      'target_defaults': {
        'target_conditions': [
          ['target_arch=="ia32" and _toolset=="target"', {
            'asflags': [
              '-32',
            ],
            'cflags': [
              '-msse2',
              '-mfpmath=sse',
              '-mmmx',  # Allows mmintrin.h for MMX intrinsics.
              '-m32',
            ],
            'ldflags': [
              '-m32',
            ],
          }],  # target_arch=="ia32" and _toolset=="target"
          ['target_arch=="x64" and _toolset=="target"', {
            'cflags': [
              '-m64',
              '-march=x86-64',
            ],
            'ldflags': [
              '-m64',
            ],
          }],  # target_arch=="x64" and _toolset=="target"
          ['target_arch=="arm" and _toolset=="target"', {
            'conditions': [
              ['clang==0', {
                'cflags_cc': [
                  '-Wno-abi',
                ],
              }],
              ['clang==1 and arm_arch!=""', {
                'cflags': [
                  '-target arm-linux-gnueabihf',
                ],
                'ldflags': [
                  '-target arm-linux-gnueabihf',
                ],
              }],
              ['arm_arch!=""', {
                'cflags': [
                  '-march=<(arm_arch)',
                ],
                'conditions': [
                  ['use_lto==1 or use_lto_o2==1', {
                    'ldflags': [
                      '-march=<(arm_arch)',
                    ],
                  }],
                ],
              }],
              ['arm_tune!=""', {
                'cflags': [
                  '-mtune=<(arm_tune)',
                ],
                'conditions': [
                  ['use_lto==1 or use_lto_o2==1', {
                    'ldflags': [
                      '-mtune=<(arm_tune)',
                    ],
                  }],
                ],
              }],
              ['arm_fpu!=""', {
                'cflags': [
                  '-mfpu=<(arm_fpu)',
                ],
                'conditions': [
                  ['use_lto==1 or use_lto_o2==1', {
                    'ldflags': [
                      '-mfpu=<(arm_fpu)',
                    ],
                  }],
                ],
              }],
              ['arm_float_abi!=""', {
                'cflags': [
                  '-mfloat-abi=<(arm_float_abi)',
                ],
                'conditions': [
                  ['use_lto==1 or use_lto_o2==1', {
                    'ldflags': [
                      '-mfloat-abi=<(arm_float_abi)',
                    ],
                  }],
                ],
              }],
              ['arm_thumb==1', {
                'cflags': [
                  '-mthumb',
                ],
                'conditions': [
                  ['use_lto==1 or use_lto_o2==1', {
                    'ldflags': [
                      '-mthumb',
                    ],
                  }],
                ],
              }],
            ],
          }],  # target_arch=="arm64" and _toolset=="target"
          ['target_arch=="arm64" and _toolset=="target"', {
            'conditions': [
              ['clang==0', {
                'cflags_cc': [
                  '-Wno-abi',
                ],
              }],
              ['clang==1 and arm_arch!=""', {
                'cflags': [
                  '-target  aarch64-linux-gnu',
                ],
                'ldflags': [
                  '-target  aarch64-linux-gnu',
                ],
              }],
            ],
          }],  # target_arch=="arm" and _toolset=="target"
        ],
      },
    }],  # OS=="linux"
  ],
}
