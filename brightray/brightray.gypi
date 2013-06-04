{
  'variables': {
    'libchromiumcontent_dir': 'vendor/download/libchromiumcontent',
    'libchromiumcontent_library_dir': '<(libchromiumcontent_dir)/Release',
    'libchromiumcontent_include_dir': '<(libchromiumcontent_dir)/src',
    'libchromiumcontent_resources_dir': '<(libchromiumcontent_library_dir)',
    'libchromiumcontent_src_dir': '<(libchromiumcontent_dir)/src',
    'mac_deployment_target%': '10.8',
    'mac_sdkroot%': 'macosx',

    'win_release_RuntimeLibrary%': '2', # /MD (nondebug DLL)
    'win_debug_RuntimeLibrary%': '3', # /MTd (debug DLL)

    # See http://msdn.microsoft.com/en-us/library/aa652360(VS.71).aspx
    'win_release_Optimization%': '2', # 2 = /Os
    'win_debug_Optimization%': '0',   # 0 = /Od

    # See http://msdn.microsoft.com/en-us/library/2kxx5t2c(v=vs.80).aspx
    # Tri-state: blank is default, 1 on, 0 off
    'win_release_OmitFramePointers%': '0',
    # Tri-state: blank is default, 1 on, 0 off
    'win_debug_OmitFramePointers%': '',

    # See http://msdn.microsoft.com/en-us/library/8wtf2dfz(VS.71).aspx
    'win_debug_RuntimeChecks%': '3',    # 3 = all checks enabled, 0 = off

    # See http://msdn.microsoft.com/en-us/library/47238hez(VS.71).aspx
    'win_debug_InlineFunctionExpansion%': '',    # empty = default, 0 = off,
    'win_release_InlineFunctionExpansion%': '2', # 1 = only __inline, 2 = max
  },
  'target_defaults': {
    'xcode_settings': {
      'ALWAYS_SEARCH_USER_PATHS': 'NO',
      'CLANG_CXX_LANGUAGE_STANDARD': 'gnu++11',
      'CLANG_CXX_LIBRARY': 'libstdc++',
      'COMBINE_HIDPI_IMAGES': 'YES',
      'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',
      'GCC_ENABLE_CPP_RTTI': 'NO',
      'GCC_TREAT_WARNINGS_AS_ERRORS': 'YES',
      'MACOSX_DEPLOYMENT_TARGET': '<(mac_deployment_target)',
      'RUN_CLANG_STATIC_ANALYZER': 'YES',
      'SDKROOT': '<(mac_sdkroot)',
      'USE_HEADER_MAP': 'NO',
      'WARNING_CFLAGS': [
        '-Wall',
        '-Wextra',
        '-Wno-unused-parameter',
        '-Wno-missing-field-initializers',
      ],
    },
    'configurations': {
      'Common_Base': {
        'abstract': 1,
        'defines': [
          'COMPONENT_BUILD',
          'GURL_DLL',
          'SKIA_DLL',
          'NDEBUG',
          'USING_V8_SHARED',
          'WEBKIT_DLL',
        ],
        'msvs_configuration_attributes': {
          'OutputDirectory': '<(DEPTH)\\build\\$(ConfigurationName)', 
          'IntermediateDirectory': '$(OutDir)\\obj\\$(ProjectName)',
          'CharacterSet': '1',
        },
        'msvs_settings': {
          'VCLinkerTool': {
            'AdditionalDependencies': [
              'advapi32.lib',
              'user32.lib',
            ],
          },
        },
      },
      'Debug': {
        'inherit_from': [
          'Common_Base',
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'Optimization': '<(win_debug_Optimization)',
            'BasicRuntimeChecks': '<(win_debug_RuntimeChecks)',
            # We use Release to match the version of chromiumcontent.dll we
            # link against.
            'RuntimeLibrary': '<(win_release_RuntimeLibrary)',
            'conditions': [
              # According to MSVS, InlineFunctionExpansion=0 means
              # "default inlining", not "/Ob0".
              # Thus, we have to handle InlineFunctionExpansion==0 separately.
              ['win_debug_InlineFunctionExpansion==0', {
                'AdditionalOptions': ['/Ob0'],
              }],
              ['win_debug_InlineFunctionExpansion!=""', {
                'InlineFunctionExpansion':
                  '<(win_debug_InlineFunctionExpansion)',
              }],
              # if win_debug_OmitFramePointers is blank, leave as default
              ['win_debug_OmitFramePointers==1', {
                'OmitFramePointers': 'true',
              }],
              ['win_debug_OmitFramePointers==0', {
                'OmitFramePointers': 'false',
                # The above is not sufficient (http://crbug.com/106711): it
                # simply eliminates an explicit "/Oy", but both /O2 and /Ox
                # perform FPO regardless, so we must explicitly disable.
                # We still want the false setting above to avoid having
                # "/Oy /Oy-" and warnings about overriding.
                'AdditionalOptions': ['/Oy-'],
              }],
            ],
          },
        },
        'xcode_settings': {
          'COPY_PHASE_STRIP': 'NO',
          'GCC_OPTIMIZATION_LEVEL': '0',
        },
      },
      'Release': {
        'inherit_from': [
          'Common_Base',
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'Optimization': '<(win_release_Optimization)',
            'RuntimeLibrary': '<(win_release_RuntimeLibrary)',
            'conditions': [
              # According to MSVS, InlineFunctionExpansion=0 means
              # "default inlining", not "/Ob0".
              # Thus, we have to handle InlineFunctionExpansion==0 separately.
              ['win_release_InlineFunctionExpansion==0', {
                'AdditionalOptions': ['/Ob0'],
              }],
              ['win_release_InlineFunctionExpansion!=""', {
                'InlineFunctionExpansion':
                  '<(win_release_InlineFunctionExpansion)',
              }],
              # if win_release_OmitFramePointers is blank, leave as default
              ['win_release_OmitFramePointers==1', {
                'OmitFramePointers': 'true',
              }],
              ['win_release_OmitFramePointers==0', {
                'OmitFramePointers': 'false',
                # The above is not sufficient (http://crbug.com/106711): it
                # simply eliminates an explicit "/Oy", but both /O2 and /Ox
                # perform FPO regardless, so we must explicitly disable.
                # We still want the false setting above to avoid having
                # "/Oy /Oy-" and warnings about overriding.
                'AdditionalOptions': ['/Oy-'],
              }],
            ],
          },
        },
      },
    },
    'conditions': [
      ['OS!="mac"', {
        'sources/': [
          ['exclude', '/mac/'],
          ['exclude', '_mac\.(mm|h)$'],
        ],
      }],
      ['OS!="win"', {
        'sources/': [
          ['exclude', '/win/'],
          ['exclude', '_win\.(cc|h)$'],
        ],
      }],
    ],
  },
  'conditions': [
    ['OS=="win"', {
      'target_defaults': {
        'include_dirs': [
          '<(libchromiumcontent_include_dir)/third_party/wtl/include',
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
        ],
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
          },
          'VCLinkerTool': {
            'GenerateDebugInformation': 'true',
            'MapFileName': '$(OutDir)\\$(TargetName).map',
            'ImportLibrary': '$(OutDir)\\lib\\$(TargetName).lib',
          },
        },
        'msvs_disabled_warnings': [
          4100, # unreferenced formal parameter
          4127, # conditional expression is constant
          4244, # 'initializing' : conversion from 'double' to 'size_t', possible loss of data
          4245, # 'initializing' : conversion from 'int' to 'const net::QuicVersionTag', signed/unsigned mismatch
          4251, # class 'std::xx' needs to have dll-interface.
          4310, # cast truncates constant value
          4355, # 'this' : used in base member initializer list
          4480, # nonstandard extension used: specifying underlying type for enum
          4481, # nonstandard extension used: override specifier 'override'
          4512, # assignment operator could not be generated
          4702, # unreachable code
        ],
      },
    }],
  ],
}
