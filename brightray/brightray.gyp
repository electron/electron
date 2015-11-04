{
  'variables': {
    # The libraries brightray will be compiled to.
    'linux_system_libraries': 'gtk+-2.0 libnotify dbus-1 x11 xrandr xext gconf-2.0 gmodule-2.0 nss'
  },
  'includes': [
    'filenames.gypi',
  ],
  'targets': [
    {
      'target_name': 'brightray',
      'type': 'static_library',
      'include_dirs': [
        '.',
        '<(libchromiumcontent_src_dir)',
        '<(libchromiumcontent_src_dir)/skia/config',
        '<(libchromiumcontent_src_dir)/third_party/skia/include/core',
        '<(libchromiumcontent_src_dir)/third_party/mojo/src',
        '<(libchromiumcontent_src_dir)/third_party/WebKit',
        '<(libchromiumcontent_dir)/gen',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '.',
          '..',
          '<(libchromiumcontent_src_dir)',
          '<(libchromiumcontent_src_dir)/skia/config',
          '<(libchromiumcontent_src_dir)/third_party/skia/include/core',
          '<(libchromiumcontent_src_dir)/third_party/icu/source/common',
          '<(libchromiumcontent_src_dir)/third_party/mojo/src',
          '<(libchromiumcontent_src_dir)/third_party/WebKit',
          '<(libchromiumcontent_dir)/gen',
        ],
      },
      'sources': [ '<@(brightray_sources)' ],
      'conditions': [
        # Link with libraries of libchromiumcontent.
        ['OS=="linux" and libchromiumcontent_component==0', {
          # On Linux we have to use "--whole-archive" to force executable
          # to include all symbols, otherwise we will have plenty of
          # unresolved symbols errors.
          'direct_dependent_settings': {
            'ldflags': [
              '-Wl,--whole-archive',
              '<@(libchromiumcontent_libraries)',
              '-Wl,--no-whole-archive',
            ],
          }
        }, {  # (Release build on Linux)
          'link_settings': {
            'libraries': [ '<@(libchromiumcontent_libraries)' ]
          },
        }],  # (Normal builds)
        # Linux specific link settings.
        ['OS=="linux"', {
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other <(linux_system_libraries))',
            ],
            'libraries': [
              '-lpthread',
              '<!@(pkg-config --libs-only-l <(linux_system_libraries))',
            ],
          },
          'cflags': [
            '<!@(pkg-config --cflags <(linux_system_libraries))',
            # Needed by using libgtk2ui:
            '-Wno-deprecated-register',
            '-Wno-sentinel',
          ],
          'cflags_cc': [
            '-Wno-reserved-user-defined-literal',
          ],
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags <(linux_system_libraries))',
              '-Wno-deprecated-register',
              '-Wno-sentinel',
            ],
          },
          'conditions': [
            ['libchromiumcontent_component', {
              'link_settings': {
                'libraries': [
                  # Following libraries are always linked statically.
                  '<(libchromiumcontent_dir)/libgtk2ui.a',
                  '<(libchromiumcontent_dir)/libdevtools_discovery.a',
                  '<(libchromiumcontent_dir)/libdevtools_http_handler.a',
                  '<(libchromiumcontent_dir)/libhttp_server.a',
                ],
              },
            }, {
              'link_settings': {
                'libraries': [
                  # Following libraries are required by libchromiumcontent:
                  '-lasound',
                  '-lcap',
                  '-lcups',
                  '-lrt',
                  '-ldl',
                  '-lresolv',
                  '-lfontconfig',
                  '-lfreetype',
                  '-lX11 -lXi -lXcursor -lXext -lXfixes -lXrender -lXcomposite -lXdamage -lXtst -lXrandr',
                  '-lexpat',
                ],
              },
            }],
          ],
        }],  # OS=="linux"
        ['OS=="mac"', {
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
            ],
          },
          'conditions':  [
            ['libchromiumcontent_component', {
              'link_settings': {
                'libraries': [
                  # Following libraries are always linked statically.
                  '<(libchromiumcontent_dir)/libdevtools_discovery.a',
                  '<(libchromiumcontent_dir)/libdevtools_http_handler.a',
                  '<(libchromiumcontent_dir)/libhttp_server.a',
                ],
              },
            }, {
              'link_settings': {
                'libraries': [
                  # Link with system frameworks.
                  # ui_base.gypi:
                  '$(SDKROOT)/System/Library/Frameworks/Accelerate.framework',
                  # net.gypi:
                  '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
                  '$(SDKROOT)/System/Library/Frameworks/Security.framework',
                  '$(SDKROOT)/System/Library/Frameworks/SystemConfiguration.framework',
                  '-lresolv',
                  # media.gyp:
                  '$(SDKROOT)/System/Library/Frameworks/AudioToolbox.framework',
                  '$(SDKROOT)/System/Library/Frameworks/AudioUnit.framework',
                  '$(SDKROOT)/System/Library/Frameworks/CoreAudio.framework',
                  '$(SDKROOT)/System/Library/Frameworks/CoreMIDI.framework',
                  '$(SDKROOT)/System/Library/Frameworks/CoreVideo.framework',
                  '$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
                  # surface.gyp:
                  '$(SDKROOT)/System/Library/Frameworks/IOSurface.framework',
                  # content_common.gypi:
                  '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
                  # base.gyp:
                  '$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework',
                  '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
                  '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
                  '$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
                  # content_browser.gypi:
                  '-lbsm',
                  # bluetooth.gyp:
                  '$(SDKROOT)/System/Library/Frameworks/IOBluetooth.framework',
                ],
              },
            }],
          ]
        }],  # OS=="mac"
        ['OS=="win"', {
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalDependencies': [
                'runtimeobject.lib',
                'windowsapp.lib'
              ],
            },
          },
          'conditions': [
            ['libchromiumcontent_component', {
              # sandbox, base_static, devtools_discovery, devtools_http_handler,
              # http_server are always linked statically.
              'link_settings': {
                'libraries': [
                  '<(libchromiumcontent_dir)/base_static.lib',
                  '<(libchromiumcontent_dir)/sandbox.lib',
                  '<(libchromiumcontent_dir)/devtools_discovery.lib',
                  '<(libchromiumcontent_dir)/devtools_http_handler.lib',
                  '<(libchromiumcontent_dir)/http_server.lib',
                ],
              },
            }, {
              # Link with system libraries.
              'link_settings': {
                'libraries': [
                  # content_browser.gypi:
                  '-lsensorsapi.lib',
                  '-lportabledeviceguids.lib',
                  # content_common.gypi:
                  '-ld3d9.lib',
                  '-ld3d11.lib',
                  '-ldxva2.lib',
                  '-lstrmiids.lib',
                  '-lmf.lib',
                  '-lmfplat.lib',
                  '-lmfuuid.lib',
                  # media.gyp:
                  '-lmfreadwrite.lib',
                ],
                'msvs_settings': {
                  'VCLinkerTool': {
                    'AdditionalDependencies': [
                      'Shlwapi.lib',
                      'Crypt32.lib',
                      'advapi32.lib',
                      'dbghelp.lib',
                      'delayimp.lib',
                      'dwmapi.lib',
                      'gdi32.lib',
                      'netapi32.lib',
                      'oleacc.lib',
                      'powrprof.lib',
                      'user32.lib',
                      'usp10.lib',
                      'version.lib',
                      'winspool.lib',
                    ],
                    'DelayLoadDLLs': [
                      # content_common.gypi:
                      'd3d9.dll',
                      'dxva2.dll',
                      # media.gyp:
                      'mf.dll',
                      'mfplat.dll',
                      'mfreadwrite.dll',
                      # bluetooth.gyp:
                      'BluetoothApis.dll',
                      'Bthprops.cpl',
                      'setupapi.dll',
                    ],
                  },
                },
              },
            }],  # libchromiumcontent_component
          ],
        }],  # OS=="win"
      ],
    },
  ],
}
