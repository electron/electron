{
  'includes': [
    'vendor/download/libchromiumcontent/filenames.gypi',
  ],
  'variables': {
    'libchromiumcontent_component%': 1,
    'pkg-config%': 'pkg-config',
    'conditions': [
      # The "libchromiumcontent_component" is defined when calling "gyp".
      ['libchromiumcontent_component', {
        'libchromiumcontent_dir%': '<(libchromiumcontent_shared_libraries_dir)',
        'libchromiumcontent_libraries%': '<(libchromiumcontent_shared_libraries)',
        'libchromiumcontent_v8_libraries%': '<(libchromiumcontent_shared_v8_libraries)',
      }, {
        'libchromiumcontent_dir%': '<(libchromiumcontent_static_libraries_dir)',
        'libchromiumcontent_libraries%': '<(libchromiumcontent_static_libraries)',
        'libchromiumcontent_v8_libraries%': '<(libchromiumcontent_static_v8_libraries)',
      }],
    ],
  },
  'target_defaults': {
    'includes': [
       # Rules for excluding e.g. foo_win.cc from the build on non-Windows.
      'filename_rules.gypi',
    ],
    # Putting this in "configurations" will make overrides not working.
    'xcode_settings': {
      'ALWAYS_SEARCH_USER_PATHS': 'NO',
      'ARCHS': ['x86_64'],
      'COMBINE_HIDPI_IMAGES': 'YES',
      'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',
      'GCC_ENABLE_CPP_RTTI': 'NO',
      'GCC_TREAT_WARNINGS_AS_ERRORS': 'YES',
      'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
      'MACOSX_DEPLOYMENT_TARGET': '10.9',
      'RUN_CLANG_STATIC_ANALYZER': 'YES',
      'USE_HEADER_MAP': 'NO',
    },
    'msvs_configuration_attributes': {
      'OutputDirectory': '<(DEPTH)\\build\\$(ConfigurationName)',
      'IntermediateDirectory': '$(OutDir)\\obj\\$(ProjectName)',
      'CharacterSet': '1',
    },
    'msvs_system_include_dirs': [
      '$(VSInstallDir)/VC/atlmfc/include',
    ],
    'msvs_settings': {
      'VCCLCompilerTool': {
        'AdditionalOptions': ['/MP'],
        'MinimalRebuild': 'false',
        'BufferSecurityCheck': 'true',
        'EnableFunctionLevelLinking': 'true',
        'RuntimeTypeInfo': 'false',
        'WarningLevel': '4',
        'WarnAsError': 'true',
        'DebugInformationFormat': '3',
        # Programs that use the Standard C++ library must be compiled with
        # C++
        # exception handling enabled.
        # http://support.microsoft.com/kb/154419
        'ExceptionHandling': 1,
      },
      'VCLinkerTool': {
        'GenerateDebugInformation': 'true',
        'MapFileName': '$(OutDir)\\$(TargetName).map',
        'ImportLibrary': '$(OutDir)\\lib\\$(TargetName).lib',
        'LargeAddressAware': '2',
        'AdditionalOptions': [
          # ATL 8.0 included in WDK 7.1 makes the linker to generate
          # following
          # warnings:
          #   - warning LNK4254: section 'ATL' (50000040) merged into
          #     '.rdata' (40000040) with different attributes
          #   - warning LNK4078: multiple 'ATL' sections found with
          #     different attributes
          '/ignore:4254',
          '/ignore:4078',
          # views_chromiumcontent.lib generates this warning because it's
          # symobls are defined as dllexport but used as static library:
          #   - warning LNK4217: locally defined symbol imported in function
          #   - warning LNK4049: locally defined symbol imported
          '/ignore:4217',
          '/ignore:4049',
        ],
      },
    },
    'configurations': {
      # The "Debug" and "Release" configurations are not actually used.
      'Debug': {},
      'Release': {},

      'Common_Base': {
        'abstract': 1,
        'defines': [
          # Used by content_browser_client.h:
          'ENABLE_WEBRTC',
          # We are using Release version libchromiumcontent:
          'NDEBUG',
          # Needed by gin:
          'V8_USE_EXTERNAL_STARTUP_DATA',
          # From skia_for_chromium_defines.gypi:
          'SK_SUPPORT_LEGACY_GETTOPDEVICE',
          'SK_SUPPORT_LEGACY_BITMAP_CONFIG',
          'SK_SUPPORT_LEGACY_DEVICE_VIRTUAL_ISOPAQUE',
          'SK_SUPPORT_LEGACY_N32_NAME',
          'SK_SUPPORT_LEGACY_SETCONFIG',
          'SK_IGNORE_ETC1_SUPPORT',
          'SK_IGNORE_GPU_DITHER',
          # NACL is not enabled:
          'DISABLE_NACL',
        ],
        'conditions': [
          ['OS!="mac"', {
            'defines': [
              'TOOLKIT_VIEWS',
              'USE_AURA',
            ],
          }],
          ['OS in ["mac", "win"]', {
            'defines': [
              'USE_OPENSSL',
            ],
          }, {
            'defines': [
              'USE_X11',
              # "use_nss_certs" is set to 1 in libchromiumcontent.
              'USE_NSS_CERTS',
              'USE_NSS',  # deprecated after Chrome 45.
            ],
          }],
          ['OS=="linux"', {
            'defines': [
              '_LARGEFILE_SOURCE',
              '_LARGEFILE64_SOURCE',
              '_FILE_OFFSET_BITS=64',
            ],
            'cflags_cc': [
              '-D__STRICT_ANSI__',
              '-fno-rtti',
            ],
          }],  # OS=="linux"
          ['OS=="mac"', {
            'defines': [
              # The usage of "webrtc/modules/desktop_capture/desktop_capture_options.h"
              # is required to see this macro.
              'WEBRTC_MAC',
             ],
          }],  # OS=="mac"
          ['OS=="win"', {
            'include_dirs': [
              '<(libchromiumcontent_src_dir)/third_party/wtl/include',
            ],
            'defines': [
              '_WIN32_WINNT=0x0602',
              'WINVER=0x0602',
              'WIN32',
              '_WINDOWS',
              'NOMINMAX',
              'PSAPI_VERSION=1',
              '_CRT_RAND_S',
              'CERT_CHAIN_PARA_HAS_EXTRA_FIELDS',
              'WIN32_LEAN_AND_MEAN',
              '_ATL_NO_OPENGL',
              '_SECURE_ATL',
              # The usage of "webrtc/modules/desktop_capture/desktop_capture_options.h"
              # is required to see this macro.
              'WEBRTC_WIN',
            ],
            'conditions': [
              ['target_arch=="x64"', {
                'msvs_configuration_platform': 'x64',
                'msvs_settings': {
                  'VCLinkerTool': {
                    'MinimumRequiredVersion': '5.02',  # Server 2003.
                    'TargetMachine': '17', # x86 - 64
                    # Doesn't exist x64 SDK. Should use oleaut32 in any case.
                    'IgnoreDefaultLibraryNames': [ 'olepro32.lib' ],
                  },
                  'VCLibrarianTool': {
                    'TargetMachine': '17', # x64
                  },
                },
              }],
            ],
          }],  # OS=="win"
        ],
      },  # Common_Base
      'Debug_Base': {
        'abstract': 1,
        'defines': [
          # Use this instead of "NDEBUG" to determine whether we are in
          # Debug build, because "NDEBUG" is already used by Chromium.
          'DEBUG',
          # Require when using libchromiumcontent.
          'COMPONENT_BUILD',
          'GURL_DLL',
          'SKIA_DLL',
          'USING_V8_SHARED',
          'WEBKIT_DLL',
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': '2',  # /MD (nondebug DLL)
            'Optimization': '0',  # 0 = /Od
            # See http://msdn.microsoft.com/en-us/library/8wtf2dfz(VS.71).aspx
            'BasicRuntimeChecks': '3',  # 3 = all checks enabled, 0 = off
          },
        },
      },  # Debug_Base
      'Release_Base': {
        'abstract': 1,
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': '0',  # /MT (nondebug static)
            # 1, optimizeMinSpace, Minimize Size (/O1)
            'Optimization': '1',
            # 2, favorSize - Favor small code (/Os)
            'FavorSizeOrSpeed': '2',
            # See http://msdn.microsoft.com/en-us/library/47238hez(VS.71).aspx
            'InlineFunctionExpansion': '2',  # 2 = max
            # See http://msdn.microsoft.com/en-us/library/2kxx5t2c(v=vs.80).aspx
            'OmitFramePointers': 'false',
            # The above is not sufficient (http://crbug.com/106711): it
            # simply eliminates an explicit "/Oy", but both /O2 and /Ox
            # perform FPO regardless, so we must explicitly disable.
            # We still want the false setting above to avoid having
            # "/Oy /Oy-" and warnings about overriding.
            'AdditionalOptions': ['/Oy-'],
          },
          'VCLinkerTool': {
            # Turn off incremental linking to save binary size.
            'LinkIncremental': '1',  # /INCREMENTAL:NO
          },
        },
        'conditions': [
          ['OS=="linux"', {
            'cflags': [
              '-O2',
              # Generate symbols, will be stripped later.
              '-g',
              # Don't emit the GCC version ident directives, they just end up
              # in the .comment section taking up binary size.
              '-fno-ident',
              # Put data and code in their own sections, so that unused symbols
              # can be removed at link time with --gc-sections.
              '-fdata-sections',
              '-ffunction-sections',
            ],
            'ldflags': [
              # Specifically tell the linker to perform optimizations.
              # See http://lwn.net/Articles/192624/ .
              '-Wl,-O1',
              '-Wl,--as-needed',
              '-Wl,--gc-sections',
            ],
          }],  # OS=="linux"
        ],
      },  # Release_Base
      'conditions': [
        ['libchromiumcontent_component', {
          'D': {
            'inherit_from': ['Common_Base', 'Debug_Base'],
          },  # D (Debug)
        }, {
          'R': {
            'inherit_from': ['Common_Base', 'Release_Base'],
          },  # R (Release)
        }],  # libchromiumcontent_component
        ['OS=="win"', {
          'conditions': [
            # gyp always assumes "_x64" targets on Windows.
            ['libchromiumcontent_component', {
              'D_x64': {
                'inherit_from': ['Common_Base', 'Debug_Base'],
              },  # D_x64
            }, {
              'R_x64': {
                'inherit_from': ['Common_Base', 'Release_Base'],
              },  # R_x64
            }],  # libchromiumcontent_component
          ],
        }],  # OS=="win"
      ],
    },  # configurations
    'target_conditions': [
      # Putting this under "configurations" doesn't work.
      ['libchromiumcontent_component', {
        'xcode_settings': {
          'GCC_OPTIMIZATION_LEVEL': '0',
        },
      }, {  # "Debug_Base"
        'xcode_settings': {
          'DEAD_CODE_STRIPPING': 'YES',  # -Wl,-dead_strip
          'GCC_OPTIMIZATION_LEVEL': '2',
          'OTHER_CFLAGS': [
            '-fno-inline',
            '-fno-omit-frame-pointer',
            '-fno-builtin',
            '-fno-optimize-sibling-calls',
          ],
        },
      }],  # "Release_Base"
      ['OS=="mac" and libchromiumcontent_component==0 and _type in ["executable", "shared_library"]', {
        'xcode_settings': {
          # Generates symbols and strip the binary.
          'DEBUG_INFORMATION_FORMAT': 'dwarf-with-dsym',
          'DEPLOYMENT_POSTPROCESSING': 'YES',
          'STRIP_INSTALLED_PRODUCT': 'YES',
          'STRIPFLAGS': '-x',
        },
      }],  # OS=="mac" and libchromiumcontent_component==0 and _type in ["executable", "shared_library"]
      ['OS=="linux" and target_arch=="ia32" and _toolset=="target"', {
        'ldflags': [
          # Workaround for linker OOM.
          '-Wl,--no-keep-memory',
        ],
      }],
    ],  # target_conditions
    # Ignored compiler warnings of Chromium.
    'conditions': [
      ['OS=="mac"', {
        'xcode_settings': {
          'WARNING_CFLAGS': [
            '-Wall',
            '-Wextra',
            '-Wno-unused-parameter',
            '-Wno-missing-field-initializers',
            '-Wno-deprecated-declarations',
            '-Wno-undefined-var-template', # https://crbug.com/604888
            '-Wno-unneeded-internal-declaration',
            '-Wno-inconsistent-missing-override',
          ],
        },
      }],
      ['OS=="linux"', {
        'cflags': [
          '-Wno-inconsistent-missing-override',
          '-Wno-undefined-var-template', # https://crbug.com/604888
        ],
      }],
      ['OS=="win"', {
        'msvs_disabled_warnings': [
          4100, # unreferenced formal parameter
          4121, # alignment of a member was sensitive to packing
          4127, # conditional expression is constant
          4189, # local variable is initialized but not referenced
          4244, # 'initializing' : conversion from 'double' to 'size_t', possible loss of data
          4245, # 'initializing' : conversion from 'int' to 'const net::QuicVersionTag', signed/unsigned mismatch
          4251, # class 'std::xx' needs to have dll-interface.
          4310, # cast truncates constant value
          4355, # 'this' : used in base member initializer list
          4480, # nonstandard extension used: specifying underlying type for enum
          4481, # nonstandard extension used: override specifier 'override'
          4510, # default constructor could not be generated
          4512, # assignment operator could not be generated
          4610, # user defined constructor required
          4702, # unreachable code
          4819, # The file contains a character that cannot be represented in the current code page
        ],
      }],
    ],  # conditions
  },  # target_defaults
}
