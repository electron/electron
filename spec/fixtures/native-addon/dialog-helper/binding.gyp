{
  'targets': [
    {
      'target_name': 'dialog_helper',
      'sources': [
        'src/main.cc',
      ],
      'conditions': [
        ['OS=="mac"', {
          'sources+': [
            'src/dialog_helper_mac.mm',
          ],
          'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
          ],
          'xcode_settings': {
            'OTHER_CFLAGS': ['-fobjc-arc'],
          },
        }],
        ['OS=="win"', {
          'sources+': [
            'src/dialog_helper_win.cc',
          ],
        }],
        ['OS!="mac" and OS!="win"', {
          'type': 'none',
        }],
      ],
    }
  ]
}
