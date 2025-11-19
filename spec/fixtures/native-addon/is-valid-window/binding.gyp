{
  'target_defaults': {
    'conditions': [
      ['OS=="win"', {
        'msvs_disabled_warnings': [
          4530,  # C++ exception handler used, but unwind semantics are not enabled
          4506,  # no definition for inline function
        ],
      }],
    ],
  },
  'targets': [
    {
      'target_name': 'is_valid_window',
      'sources': [
        'src/impl.h',
        'src/main.cc',
      ],
      'conditions': [
        ['OS=="win"', {
          'sources': [
            'src/impl_win.cc',
          ],
        }],
        ['OS=="mac"', {
          'sources': [
            'src/impl_darwin.mm',
          ],
          'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
          ],
        }],
        ['OS not in ["mac", "win"]', {
          'sources': [
            'src/impl_posix.cc',
          ],
        }],
      ],
    }
  ]
}
