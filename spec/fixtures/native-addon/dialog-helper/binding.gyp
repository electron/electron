{
  'targets': [
    {
      'target_name': 'dialog_helper',
      'conditions': [
        ['OS=="mac"', {
          'sources': [
            'src/main.cc',
            'src/dialog_helper_mac.mm',
          ],
          'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
          ],
          'xcode_settings': {
            'OTHER_CFLAGS': ['-fobjc-arc'],
          },
        }, {
          'type': 'none',
        }],
      ],
    }
  ]
}
