{
  'target_defaults': {
    'conditions': [
      ['OS=="win"', {
        'msvs_disabled_warnings': [
          4530,
          4506,
        ],
      }],
    ],
  },
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
        }],
        ['OS=="win"', {
          'sources': [
            'src/main.cc',
            'src/dialog_helper_win.cc',
          ],
        }],
        ['OS not in ["mac", "win"]', {
          'type': 'none',
        }],
      ],
    }
  ]
}
