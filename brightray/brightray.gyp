{
  'variables': {
    # The libraries brightray will be compiled to.
    'linux_system_libraries': 'gtk+-2.0 dbus-1 x11 xi xcursor xdamage xrandr xcomposite xext xfixes xrender xtst xscrnsaver gconf-2.0 gmodule-2.0 nss'
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
              '<!@(<(pkg-config) --libs-only-L --libs-only-other <(linux_system_libraries))',
            ],
            'libraries': [
              '-lpthread',
              '<!@(<(pkg-config) --libs-only-l <(linux_system_libraries))',
            ],
          },
          'cflags': [
            '<!@(<(pkg-config) --cflags <(linux_system_libraries))',
            # Needed by using libgtk2ui:
            '-Wno-deprecated-register',
            '-Wno-sentinel',
          ],
          'cflags_cc': [
            '-Wno-reserved-user-defined-literal',
          ],
          'direct_dependent_settings': {
            'cflags': [
              '<!@(<(pkg-config) --cflags <(linux_system_libraries))',
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
                  '<(libchromiumcontent_dir)/libdesktop_capture.a',
                  '<(libchromiumcontent_dir)/libdesktop_capture_differ_sse2.a',
                  '<(libchromiumcontent_dir)/libsystem_wrappers.a',
                  '<(libchromiumcontent_dir)/librtc_base.a',
                  '<(libchromiumcontent_dir)/librtc_base_approved.a',
                  '<(libchromiumcontent_dir)/libwebrtc_common.a',
                  '<(libchromiumcontent_dir)/libyuv.a',
                  '<(libchromiumcontent_dir)/libcdm_renderer.a',
                  '<(libchromiumcontent_dir)/libsecurity_state.a',
                ],
              },
            }, {
              'link_settings': {
                'libraries': [
                  # Link with ffmpeg.
                  '<(libchromiumcontent_dir)/libffmpeg.so',
                  # Following libraries are required by libchromiumcontent:
                  '-lasound',
                  '-lcap',
                  '-lcups',
                  '-lrt',
                  '-ldl',
                  '-lresolv',
                  '-lfontconfig',
                  '-lfreetype',
                  '-lexpat',
                ],
              },
            }],
            ['target_arch=="arm"', {
              'link_settings': {
                'libraries!': [
                  '<(libchromiumcontent_dir)/libdesktop_capture_differ_sse2.a',
                ],
              },
            }],
          ],
        }],  # OS=="linux"
        ['OS=="mac"', {
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              # Required by webrtc:
              '$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
              '$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
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
                  '<(libchromiumcontent_dir)/libdesktop_capture.a',
                  '<(libchromiumcontent_dir)/libdesktop_capture_differ_sse2.a',
                  '<(libchromiumcontent_dir)/librtc_base.a',
                  '<(libchromiumcontent_dir)/librtc_base_approved.a',
                  '<(libchromiumcontent_dir)/libsystem_wrappers.a',
                  '<(libchromiumcontent_dir)/libwebrtc_common.a',
                  '<(libchromiumcontent_dir)/libyuv.a',
                  '<(libchromiumcontent_dir)/libcdm_renderer.a',
                  '<(libchromiumcontent_dir)/libsecurity_state.a',
                ],
              },
            }, {
              'link_settings': {
                'libraries': [
                  # Link with ffmpeg.
                  '<(libchromiumcontent_dir)/libffmpeg.dylib',
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
                  # surface.gyp:
                  '$(SDKROOT)/System/Library/Frameworks/IOSurface.framework',
                  # content_common.gypi:
                  '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
                  # base.gyp:
                  '$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework',
                  '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
                  '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
                  # content_browser.gypi:
                  '-lbsm',
                  # content_common.gypi:
                  '-lsandbox',
                  # bluetooth.gyp:
                  '$(SDKROOT)/System/Library/Frameworks/IOBluetooth.framework',
                ],
              },
            }],
          ]
        }],  # OS=="mac"
        ['OS=="win"', {
          'conditions': [
            ['libchromiumcontent_component', {
              # sandbox, base_static, devtools_discovery, devtools_http_handler,
              # http_server are always linked statically.
              'link_settings': {
                'libraries': [
                  '<(libchromiumcontent_dir)/base_static.lib',
                  '<(libchromiumcontent_dir)/sandbox.lib',
                  '<(libchromiumcontent_dir)/sandbox_helper_win.lib',
                  '<(libchromiumcontent_dir)/devtools_discovery.lib',
                  '<(libchromiumcontent_dir)/devtools_http_handler.lib',
                  '<(libchromiumcontent_dir)/http_server.lib',
                  '<(libchromiumcontent_dir)/desktop_capture.lib',
                  '<(libchromiumcontent_dir)/desktop_capture_differ_sse2.lib',
                  '<(libchromiumcontent_dir)/rtc_base.lib',
                  '<(libchromiumcontent_dir)/rtc_base_approved.lib',
                  '<(libchromiumcontent_dir)/system_wrappers.lib',
                  '<(libchromiumcontent_dir)/webrtc_common.lib',
                  '<(libchromiumcontent_dir)/libyuv.lib',
                  '<(libchromiumcontent_dir)/cdm_renderer.lib',
                  '<(libchromiumcontent_dir)/security_state.lib',
                  # Friends of pdf.lib:
                  '<(libchromiumcontent_dir)/pdf.lib',
                  '<(libchromiumcontent_dir)/ppapi_cpp_objects.lib',
                  '<(libchromiumcontent_dir)/ppapi_internal_module.lib',
                  '<(libchromiumcontent_dir)/libjpeg.lib',
                  '<(libchromiumcontent_dir)/pdfium.lib',
                  '<(libchromiumcontent_dir)/fdrm.lib',
                  '<(libchromiumcontent_dir)/formfiller.lib',
                  '<(libchromiumcontent_dir)/fpdfapi.lib',
                  '<(libchromiumcontent_dir)/fpdfdoc.lib',
                  '<(libchromiumcontent_dir)/fpdftext.lib',
                  '<(libchromiumcontent_dir)/fpdftext.lib',
                  '<(libchromiumcontent_dir)/fxcodec.lib',
                  '<(libchromiumcontent_dir)/fxcrt.lib',
                  '<(libchromiumcontent_dir)/fxedit.lib',
                  '<(libchromiumcontent_dir)/fxge.lib',
                  '<(libchromiumcontent_dir)/javascript.lib',
                  '<(libchromiumcontent_dir)/pdfwindow.lib',
                  '<(libchromiumcontent_dir)/bigint.lib',
                  '<(libchromiumcontent_dir)/fx_agg.lib',
                  '<(libchromiumcontent_dir)/fx_freetype.lib',
                  '<(libchromiumcontent_dir)/fx_lcms2.lib',
                  '<(libchromiumcontent_dir)/fx_libopenjpeg.lib',
                  '<(libchromiumcontent_dir)/fx_zlib.lib',
                ],
              },
            }, {
              # Link with system libraries.
              'link_settings': {
                'libraries': [
                  # Link with ffmpeg.
                  '<(libchromiumcontent_dir)/ffmpeg.dll',
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
                      'advapi32.lib',
                      'dbghelp.lib',
                      'delayimp.lib',
                      'dwmapi.lib',
                      'gdi32.lib',
                      'netapi32.lib',
                      'oleacc.lib',
                      'user32.lib',
                      'usp10.lib',
                      'version.lib',
                      'winspool.lib',
                      # base.gyp:
                      'powrprof.lib',
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
                      # base.gyp:
                      'powrprof.dll',
                      # windows runtime
                      'API-MS-WIN-CORE-WINRT-L1-1-0.DLL',
                      'API-MS-WIN-CORE-WINRT-STRING-L1-1-0.DLL',
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
