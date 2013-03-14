{
  'variables': {
    'libchromiumcontent_dir': 'vendor/download/libchromiumcontent',
    'libchromiumcontent_library_dir': '<(libchromiumcontent_dir)/Release',
    'libchromiumcontent_include_dir': '<(libchromiumcontent_dir)/include',
    'libchromiumcontent_resources_dir': '<(libchromiumcontent_library_dir)',
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
      'GCC_ENABLE_CPP_RTTI': 'NO',
      'GCC_TREAT_WARNINGS_AS_ERRORS': 'YES',
      'MACOSX_DEPLOYMENT_TARGET': '10.7',
      'RUN_CLANG_STATIC_ANALYZER': 'YES',
      'SDKROOT': 'macosx10.7',
      'USE_HEADER_MAP': 'NO',
      'WARNING_CFLAGS': [
        '-Wall',
        '-Wextra',
        '-Wno-unused-parameter',
        '-Wno-missing-field-initializers',
      ],
    },
    'configurations': {
      'Debug': {
        'xcode_settings': {
          'COPY_PHASE_STRIP': 'NO',
          'GCC_OPTIMIZATION_LEVEL': '0',
        },
      },
      'Release': {
      },
    },
  },
}
