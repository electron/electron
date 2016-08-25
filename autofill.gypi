{
  'variables': {
    'autofill_sources': [
      'atom/browser/api/atom_api_autofill.cc',
      'atom/browser/api/atom_api_autofill.h',
      'atom/browser/autofill/atom_autofill_client.cc',
      'atom/browser/autofill/atom_autofill_client.h',
      'atom/browser/autofill/autofill_observer.h',
      'atom/browser/autofill/personal_data_manager_factory.cc',
      'atom/browser/autofill/personal_data_manager_factory.h',
    ],
  },
  'conditions': [
    ['OS=="mac" or OS=="linux"', {
      'variables': {
        'autofill_libraries': [
          '<(libchromiumcontent_dir)/libaddressinput_util.a',
          '<(libchromiumcontent_dir)/libautofill_content_browser.a',
          '<(libchromiumcontent_dir)/libautofill_content_common.a',
          '<(libchromiumcontent_dir)/libautofill_content_mojo_bindings.a',
          '<(libchromiumcontent_dir)/libautofill_content_renderer.a',
          '<(libchromiumcontent_dir)/libautofill_core_browser.a',
          '<(libchromiumcontent_dir)/libautofill_core_common.a',
          '<(libchromiumcontent_dir)/libautofill_server_proto.a',
          '<(libchromiumcontent_dir)/libdata_use_measurement_core.a',
          '<(libchromiumcontent_dir)/libgoogle_apis.a',
          '<(libchromiumcontent_dir)/libphonenumber.a',
          '<(libchromiumcontent_dir)/libphonenumber_without_metadata.a',
          '<(libchromiumcontent_dir)/libsignin_core_browser.a',
          '<(libchromiumcontent_dir)/libsignin_core_common.a',
          '<(libchromiumcontent_dir)/libos_crypt.a',
        ]
      }
    }],
    ['OS=="win"', {
      'variables': {
        'autofill_libraries': [
          '<(libchromiumcontent_dir)/libaddressinput_util.lib',
          '<(libchromiumcontent_dir)/autofill_content_browser.lib',
          '<(libchromiumcontent_dir)/autofill_content_common.lib',
          '<(libchromiumcontent_dir)/autofill_content_mojo_bindings.lib',
          '<(libchromiumcontent_dir)/autofill_content_renderer.lib',
          '<(libchromiumcontent_dir)/autofill_core_browser.lib',
          '<(libchromiumcontent_dir)/autofill_core_common.lib',
          '<(libchromiumcontent_dir)/autofill_server_proto.lib',
          '<(libchromiumcontent_dir)/data_use_measurement_core.lib',
          '<(libchromiumcontent_dir)/google_apis.lib',
          '<(libchromiumcontent_dir)/libphonenumber.lib',
          '<(libchromiumcontent_dir)/libphonenumber_without_metadata.lib',
          '<(libchromiumcontent_dir)/signin_core_browser.lib',
          '<(libchromiumcontent_dir)/signin_core_common.lib',
          '<(libchromiumcontent_dir)/os_crypt.lib',
        ],
      },
    }],
  ],
  'target_defaults': {
    'defines': [
      'GOOGLE_PROTOBUF_NO_RTTI'
    ],
  },
}
