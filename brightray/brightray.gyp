{
  'variables': {
    # The libraries brightray will be compiled to.
    'linux_system_libraries': 'gtk+-2.0 dbus-1 x11 x11-xcb xcb xi xcursor xdamage xrandr xcomposite xext xfixes xrender xtst xscrnsaver gconf-2.0 gmodule-2.0 nss'
  },
  'includes': [
    'filenames.gypi',
  ],
  'targets': [
    {
      'target_name': 'brightray',
      'type': 'static_library',
      'include_dirs': [
        '..',
        '<(libchromiumcontent_src_dir)',
        '<(libchromiumcontent_src_dir)/skia/config',
        '<(libchromiumcontent_src_dir)/third_party/boringssl/src/include',
        '<(libchromiumcontent_src_dir)/third_party/skia/include/core',
        '<(libchromiumcontent_src_dir)/third_party/mojo/src',
        '<(libchromiumcontent_src_dir)/third_party/WebKit',
        '<(libchromiumcontent_dir)/gen',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../vendor',
          '<(libchromiumcontent_src_dir)',
          '<(libchromiumcontent_src_dir)/gpu',
          '<(libchromiumcontent_src_dir)/skia/config',
          '<(libchromiumcontent_src_dir)/third_party/boringssl/src/include',
          '<(libchromiumcontent_src_dir)/third_party/skia/include/core',
          '<(libchromiumcontent_src_dir)/third_party/skia/include/config',
          '<(libchromiumcontent_src_dir)/third_party/icu/source/common',
          '<(libchromiumcontent_src_dir)/third_party/mojo/src',
          '<(libchromiumcontent_src_dir)/third_party/khronos',
          '<(libchromiumcontent_src_dir)/third_party/WebKit',
          '<(libchromiumcontent_dir)/gen',
          '<(libchromiumcontent_dir)/gen/third_party/WebKit',
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
            # Needed by using libgtkui:
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
                  '<(libchromiumcontent_dir)/libgtkui.a',
                  '<(libchromiumcontent_dir)/libhttp_server.a',
                  '<(libchromiumcontent_dir)/libdesktop_capture.a',
                  '<(libchromiumcontent_dir)/libdom_keycode_converter.a',
                  '<(libchromiumcontent_dir)/libsystem_wrappers.a',
                  '<(libchromiumcontent_dir)/librtc_base.a',
                  '<(libchromiumcontent_dir)/librtc_base_approved.a',
                  '<(libchromiumcontent_dir)/libwebrtc_common.a',
                  '<(libchromiumcontent_dir)/libyuv.a',
                  '<(libchromiumcontent_dir)/librenderer.a',
                  '<(libchromiumcontent_dir)/libsecurity_state.a',
                  # Friends of libpdf.a:
                  # On Linux we have to use "--whole-archive" to include
                  # all symbols, otherwise there will be plenty of
                  # unresolved symbols errors.
                  '-Wl,--whole-archive',
                  '<(libchromiumcontent_dir)/libpdf.a',
                  '<(libchromiumcontent_dir)/libppapi_cpp_objects.a',
                  '<(libchromiumcontent_dir)/libppapi_internal_module.a',
                  '<(libchromiumcontent_dir)/libjpeg.a',
                  '<(libchromiumcontent_dir)/libpdfium.a',
                  '<(libchromiumcontent_dir)/libfdrm.a',
                  '<(libchromiumcontent_dir)/libformfiller.a',
                  '<(libchromiumcontent_dir)/libfpdfapi.a',
                  '<(libchromiumcontent_dir)/libfpdfdoc.a',
                  '<(libchromiumcontent_dir)/libfpdftext.a',
                  '<(libchromiumcontent_dir)/libfxcodec.a',
                  '<(libchromiumcontent_dir)/libfxedit.a',
                  '<(libchromiumcontent_dir)/libfxge.a',
                  '<(libchromiumcontent_dir)/libfxjs.a',
                  '<(libchromiumcontent_dir)/libjavascript.a',
                  '<(libchromiumcontent_dir)/libpdfwindow.a',
                  '<(libchromiumcontent_dir)/libfx_agg.a',
                  '<(libchromiumcontent_dir)/libfx_lcms2.a',
                  '<(libchromiumcontent_dir)/libfx_libopenjpeg.a',
                  '<(libchromiumcontent_dir)/libfx_zlib.a',
                  '-Wl,--no-whole-archive',
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
              # Required by media:
              '$(SDKROOT)/System/Library/Frameworks/VideoToolbox.framework',
            ],
          },
          'conditions':  [
            ['libchromiumcontent_component', {
              'link_settings': {
                'libraries': [
                  # Following libraries are always linked statically.
                  '<(libchromiumcontent_dir)/libhttp_server.a',
                  '<(libchromiumcontent_dir)/libdesktop_capture.a',
                  '<(libchromiumcontent_dir)/libdom_keycode_converter.a',
                  '<(libchromiumcontent_dir)/librtc_base.a',
                  '<(libchromiumcontent_dir)/librtc_base_approved.a',
                  '<(libchromiumcontent_dir)/libsystem_wrappers.a',
                  '<(libchromiumcontent_dir)/libwebrtc_common.a',
                  '<(libchromiumcontent_dir)/libyuv.a',
                  '<(libchromiumcontent_dir)/librenderer.a',
                  '<(libchromiumcontent_dir)/libsecurity_state.a',
                  # Friends of libpdf.a:
                  '<(libchromiumcontent_dir)/libpdf.a',
                  '<(libchromiumcontent_dir)/libppapi_cpp_objects.a',
                  '<(libchromiumcontent_dir)/libppapi_internal_module.a',
                  '<(libchromiumcontent_dir)/libjpeg.a',
                  '<(libchromiumcontent_dir)/libpdfium.a',
                  '<(libchromiumcontent_dir)/libfdrm.a',
                  '<(libchromiumcontent_dir)/libformfiller.a',
                  '<(libchromiumcontent_dir)/libfpdfapi.a',
                  '<(libchromiumcontent_dir)/libfpdfdoc.a',
                  '<(libchromiumcontent_dir)/libfpdftext.a',
                  '<(libchromiumcontent_dir)/libfxcodec.a',
                  '<(libchromiumcontent_dir)/libfxcrt.a',
                  '<(libchromiumcontent_dir)/libfxedit.a',
                  '<(libchromiumcontent_dir)/libfxge.a',
                  '<(libchromiumcontent_dir)/libfxjs.a',
                  '<(libchromiumcontent_dir)/libjavascript.a',
                  '<(libchromiumcontent_dir)/libpdfwindow.a',
                  '<(libchromiumcontent_dir)/libfx_agg.a',
                  '<(libchromiumcontent_dir)/libfx_freetype.a',
                  '<(libchromiumcontent_dir)/libfx_lcms2.a',
                  '<(libchromiumcontent_dir)/libfx_libopenjpeg.a',
                  '<(libchromiumcontent_dir)/libfx_zlib.a',
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
                  '$(SDKROOT)/System/Library/Frameworks/AVFoundation.framework',
                  '$(SDKROOT)/System/Library/Frameworks/CoreAudio.framework',
                  '$(SDKROOT)/System/Library/Frameworks/CoreMedia.framework',
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
                  # device/gamepad/BUILD.gn:
                  '$(SDKROOT)/System/Library/Frameworks/GameController.framework',
                  # content_browser.gypi:
                  '-lbsm',
                  # content_common.gypi:
                  '-lsandbox',
                  # bluetooth.gyp:
                  '$(SDKROOT)/System/Library/Frameworks/IOBluetooth.framework',
                  # components/wifi/BUILD.gn:
                  '$(SDKROOT)/System/Library/Frameworks/CoreWLAN.framework',
                  # printing/BUILD.gn:
                  '-lcups',
                ],
              },
            }],
          ]
        }],  # OS=="mac"
        ['OS=="win"', {
          'conditions': [
            ['libchromiumcontent_component', {
              'link_settings': {
                'libraries': [
                  # Needed by desktop_capture.lib:
                  '-ld3d11.lib',
                  '-ldxgi.lib',
                  # Following libs are always linked statically.
                  '<(libchromiumcontent_dir)/base_static.lib',
                  '<(libchromiumcontent_dir)/sandbox.lib',
                  '<(libchromiumcontent_dir)/sandbox_helper_win.lib',
                  '<(libchromiumcontent_dir)/http_server.lib',
                  '<(libchromiumcontent_dir)/desktop_capture.lib',
                  '<(libchromiumcontent_dir)/dom_keycode_converter.lib',
                  '<(libchromiumcontent_dir)/rtc_base.lib',
                  '<(libchromiumcontent_dir)/rtc_base_approved.lib',
                  '<(libchromiumcontent_dir)/system_wrappers.lib',
                  '<(libchromiumcontent_dir)/webrtc_common.lib',
                  '<(libchromiumcontent_dir)/libyuv.lib',
                  '<(libchromiumcontent_dir)/renderer.lib',
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
                  '<(libchromiumcontent_dir)/fxjs.lib',
                  '<(libchromiumcontent_dir)/javascript.lib',
                  '<(libchromiumcontent_dir)/pdfwindow.lib',
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
                  '-ldxgi.lib',
                  '-ldxva2.lib',
                  '-lstrmiids.lib',
                  '-lmf.lib',
                  '-lmfplat.lib',
                  '-lmfuuid.lib',
                  # media.gyp:
                  '-ldxguid.lib',
                  '-lmfreadwrite.lib',
                  '-lmfuuid.lib',
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
                      'wtsapi32.lib',
                      # bluetooth.gyp:
                      'Bthprops.lib',
                      'BluetoothApis.lib',
                      # base.gyp:
                      'cfgmgr32.lib',
                      'powrprof.lib',
                      'setupapi.lib',
                      # net_common.gypi:
                      'crypt32.lib',
                      'dhcpcsvc.lib',
                      'ncrypt.lib',
                      'rpcrt4.lib',
                      'secur32.lib',
                      'urlmon.lib',
                      'winhttp.lib',
                      # ui/gfx/BUILD.gn:
                      'dwrite.lib',
                      # skia/BUILD.gn:
                      'fontsub.lib',
                    ],
                    'DelayLoadDLLs': [
                      'wtsapi32.dll',
                      # content_common.gypi:
                      'd3d9.dll',
                      'd3d11.dll',
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
                      'cfgmgr32.dll',
                      'powrprof.dll',
                      'setupapi.dll',
                      # net_common.gypi:
                      'crypt32.dll',
                      'dhcpcsvc.dll',
                      'rpcrt4.dll',
                      'secur32.dll',
                      'urlmon.dll',
                      'winhttp.dll',
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
