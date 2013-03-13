{
  'includes': [
    'brightray.gypi',
  ],
  'targets': [
    {
      'target_name': 'brightray',
      'type': 'static_library',
      'include_dirs': [
        '<(libchromiumcontent_include_dir)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(libchromiumcontent_include_dir)',
        ],
      },
      'sources': [
        'browser/browser_client.cc',
        'browser/browser_client.h',
        'browser/browser_context.cc',
        'browser/browser_context.h',
        'browser/browser_main_parts.cc',
        'browser/browser_main_parts.h',
        'browser/browser_main_parts_mac.mm',
        'browser/mac/bry_application.h',
        'browser/mac/bry_application.mm',
        'browser/network_delegate.cc',
        'browser/network_delegate.h',
        'browser/url_request_context_getter.cc',
        'browser/url_request_context_getter.h',
        'common/main_delegate.cc',
        'common/main_delegate.h',
        'common/main_delegate_mac.mm',
      ],
      'conditions': [
        ['OS=="mac"', {
          'link_settings': {
            'libraries': [
              'libchromiumcontent.dylib',
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
            ],
          },
        }],
      ],
    },
  ],
}
