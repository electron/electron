{
  'variables': {
    # The libraries brightray will be compiled to.
    'linux_system_libraries': 'gtk+-2.0 libnotify dbus-1 x11 xrandr xext gconf-2.0'
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
        }, {
          'link_settings': {
            'libraries': [ '<@(libchromiumcontent_libraries)' ]
          },
        }],
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
          ],
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags <(linux_system_libraries))',
              '-Wno-deprecated-register',
            ],
          },
          'conditions': [
            ['libchromiumcontent_component', {
              'link_settings': {
                'libraries': [
                  # libgtk2ui is always linked statically.
                  '<(libchromiumcontent_dir)/libgtk2ui.a',
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
        }],
        ['OS=="mac"', {
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
            ],
          },
        }],
        ['OS=="mac" and libchromiumcontent_component==0', {
          'link_settings': {
            'libraries': [
              # ui_base.gypi:
              '$(SDKROOT)/System/Library/Frameworks/Accelerate.framework',
              # net.gypi:
              '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
              '$(SDKROOT)/System/Library/Frameworks/Security.framework',
              '$(SDKROOT)/System/Library/Frameworks/SystemConfiguration.framework',
              '$(SDKROOT)/usr/lib/libresolv.dylib',
              # media.gyp:
              '$(SDKROOT)/System/Library/Frameworks/AudioToolbox.framework',
              '$(SDKROOT)/System/Library/Frameworks/AudioUnit.framework',
              '$(SDKROOT)/System/Library/Frameworks/CoreAudio.framework',
              '$(SDKROOT)/System/Library/Frameworks/CoreMIDI.framework',
              '$(SDKROOT)/System/Library/Frameworks/CoreVideo.framework',
              '$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
              '$(SDKROOT)/System/Library/Frameworks/QTKit.framework',
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
              '$(SDKROOT)/usr/lib/libbsm.dylib',
            ],
          },
        }],
        ['OS=="win" and libchromiumcontent_component==1', {
          'link_settings': {
            'libraries': [
              '<(libchromiumcontent_dir)/base_static.lib',
              '<(libchromiumcontent_dir)/sandbox.lib',
            ],
          },
        }],
        ['OS=="win" and libchromiumcontent_component==0', {
          'link_settings': {
            'libraries': [
              '<(libchromiumcontent_dir)/ffmpegsumo.lib',
              '<(libchromiumcontent_dir)/libyuv.lib',
              # content_browser.gypi:
              '-lsensorsapi.lib',
              '-lportabledeviceguids.lib',
              # content_common.gypi:
              '-ld3d9.lib',
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
                  'advapi32.lib',
                  'dbghelp.lib',
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
                  'ffmpegsumo.dll',
                  # content_common.gypi:
                  'd3d9.dll',
                  'dxva2.dll',
                  'mf.dll',
                  'mfplat.dll',
                ],
              },
            },
          },
        }],
      ],
    },
  ],
}
