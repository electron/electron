{
  'targets': [
    {
      'target_name': 'print_handler',
      'sources': [
        'src/main.cc',
      ],
      'conditions': [
        ['OS=="mac"', {
          'sources': [
            'src/print_handler_mac.mm',
          ],
          'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
          ],
          'xcode_settings': {
            'CLANG_ENABLE_OBJC_ARC': 'YES',
          },
        }],
        ['OS=="win"', {
          'sources': [
            'src/print_handler_win.cc',
          ],
        }],
        ['OS not in ["mac", "win"]', {
          'sources': [
            'src/print_handler_stub.cc',
          ],
        }],
      ],
    }
  ]
}
