{
  'variables': {
    'libchromiumcontent_dir': 'vendor/download/libchromiumcontent',
    'libchromiumcontent_library_dir': '<(libchromiumcontent_dir)/Release',
    'libchromiumcontent_include_dir': '<(libchromiumcontent_dir)/include',
    'libchromiumcontent_resources_dir': '<(libchromiumcontent_library_dir)',
    'mac_deployment_target%': '10.8',
    'mac_sdkroot%': 'macosx',
  },
  'target_defaults': {
    'defines': [
      'NDEBUG',
    ],
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
        'msvs_configuration_attributes': {
          'OutputDirectory': '<(DEPTH)\\build\\$(ConfigurationName)', 
          'IntermediateDirectory': '$(OutDir)\\obj\\$(ProjectName)',
          'CharacterSet': '1',
        },
      },
      'Debug': {
        'inherit_from': [
          'Common_Base',
        ],
        'xcode_settings': {
          'COPY_PHASE_STRIP': 'NO',
          'GCC_OPTIMIZATION_LEVEL': '0',
        },
      },
      'Release': {
        'inherit_from': [
          'Common_Base',
        ],
      },
    },
  },
  'conditions': [
    ['OS=="win"', {
      'target_defaults': {
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
        ],
      },
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
      },
    }],
  ],
}
