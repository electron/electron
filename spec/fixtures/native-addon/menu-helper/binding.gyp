{
  'targets': [{
    'target_name': 'menu_helper',
    'conditions': [
      ['OS=="mac"', {
        'sources': ['src/menu_helper.mm'],
        'libraries': ['$(SDKROOT)/System/Library/Frameworks/AppKit.framework'],
        'xcode_settings': {
          'OTHER_CFLAGS': ['-fobjc-arc'],
        },
      }, {
        'type': 'none',
      }],
    ],
  }],
}
