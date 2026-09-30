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
      'target_name': 'mouse_input',
      'conditions': [
        ['OS=="win"', {
          'sources': [
            'src/main.cc',
          ],
          'libraries': [
            'user32.lib',
            'advapi32.lib',
          ],
        }],
        # Windows only; the specs using it do not run elsewhere.
        ['OS!="win"', {
          'type': 'none',
        }],
      ],
    }
  ]
}
