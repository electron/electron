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
            'src/main_win.cc',
          ],
          'libraries': [
            'user32.lib',
            'advapi32.lib',
          ],
        }],
        ['OS=="mac"', {
          'sources': [
            'src/impl.h',
            'src/impl_mac.cc',
            'src/main_posix.cc',
            'src/post_to_window_mac.mm',
          ],
          'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
            '$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework',
          ],
          'xcode_settings': {
            # CGPreflightPostEventAccess() is macOS 10.15+.
            'MACOSX_DEPLOYMENT_TARGET': '11.0',
            'OTHER_CFLAGS': ['-fobjc-arc'],
          },
        }],
        # X11 only. libX11 and libXtst are dlopen()ed, so no X11 development
        # headers or link time dependencies are needed.
        ['OS=="linux"', {
          'sources': [
            'src/impl.h',
            'src/impl_linux.cc',
            'src/main_posix.cc',
          ],
          'libraries': [
            '-ldl',
          ],
        }],
        ['OS not in ["win", "mac", "linux"]', {
          'type': 'none',
        }],
      ],
    }
  ]
}
