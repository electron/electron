{
  'variables': {
    # The libraries brightray will be compiled to.
    'linux_system_libraries': 'gtk+-3.0 atk-bridge-2.0 dbus-1 x11 x11-xcb xcb xi xcursor xdamage xrandr xcomposite xext xfixes xrender xtst xscrnsaver gconf-2.0 gmodule-2.0 nss',
    'conditions': [
      ['target_arch=="mips64el"', {
        'linux_system_libraries': '<(linux_system_libraries) libpulse',
      }],
    ],
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
        '<(libchromiumcontent_src_dir)/third_party/skia/include/gpu',
        '<(libchromiumcontent_src_dir)/third_party/mojo/src',
        '<(libchromiumcontent_src_dir)/third_party/WebKit',
        '<(libchromiumcontent_src_dir)/third_party/khronos',
        '<(libchromiumcontent_src_dir)/third_party/protobuf/src',
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
          '<(libchromiumcontent_src_dir)/third_party/skia/include/gpu',
          '<(libchromiumcontent_src_dir)/third_party/skia/include/config',
          '<(libchromiumcontent_src_dir)/third_party/icu/source/common',
          '<(libchromiumcontent_src_dir)/third_party/mojo/src',
          '<(libchromiumcontent_src_dir)/third_party/khronos',
          '<(libchromiumcontent_src_dir)/third_party/WebKit',
          '<(libchromiumcontent_dir)/gen',
          '<(libchromiumcontent_dir)/gen/third_party/WebKit',
        ],
      },
      'defines': [
        # See Chromium's "src/third_party/protobuf/BUILD.gn".
        'GOOGLE_PROTOBUF_NO_RTTI',
        'GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER',
      ],
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
              '-latomic',
              '<!@(<(pkg-config) --libs-only-l <(linux_system_libraries))',
            ],
          },
          'cflags': [
            '<!@(<(pkg-config) --cflags <(linux_system_libraries))',
          ],
          'direct_dependent_settings': {
            'cflags': [
              '<!@(<(pkg-config) --cflags <(linux_system_libraries))',
            ],
          },
          'conditions': [
            ['clang==1', {
              'cflags_cc': [
                '-Wno-reserved-user-defined-literal',
              ],
              'cflags': [
                # Needed by using libgtkui:
                '-Wno-deprecated-register',
                '-Wno-sentinel',
              ],
              'direct_dependent_settings': {
                'cflags': [
                  '-Wno-deprecated-register',
                  '-Wno-sentinel',
                ],
              },
            }],
            ['libchromiumcontent_component', {
              'link_settings': {
                'libraries': [
                  # Following libraries are always linked statically.
                  '<(libchromiumcontent_dir)/libbase_static.a',
                  '<(libchromiumcontent_dir)/libextras.a',
                  '<(libchromiumcontent_dir)/libgtkui.a',
                  '<(libchromiumcontent_dir)/libhttp_server.a',
                  '<(libchromiumcontent_dir)/libdevice_service.a',
                  '<(libchromiumcontent_dir)/libdom_keycode_converter.a',
                  '<(libchromiumcontent_dir)/libsystem_wrappers.a',
                  '<(libchromiumcontent_dir)/librtc_base.a',
                  '<(libchromiumcontent_dir)/librtc_base_generic.a',
                  '<(libchromiumcontent_dir)/libwebrtc_common.a',
                  '<(libchromiumcontent_dir)/libinit_webrtc.a',
                  '<(libchromiumcontent_dir)/libyuv.a',
                  '<(libchromiumcontent_dir)/librenderer.a',
                  '<(libchromiumcontent_dir)/libsecurity_state.a',
                  '<(libchromiumcontent_dir)/libviz_service.a',
                  # services/device/wake_lock/power_save_blocker/
                  '<(libchromiumcontent_dir)/libpower_save_blocker.a',
                  # Friends of libpdf.a:
                  # On Linux we have to use "--whole-archive" to include
                  # all symbols, otherwise there will be plenty of
                  # unresolved symbols errors.
                  '-Wl,--whole-archive',
                  '<(libchromiumcontent_dir)/libpdf.a',
                  '<(libchromiumcontent_dir)/libppapi_cpp_objects.a',
                  '<(libchromiumcontent_dir)/libppapi_internal_module.a',
                  '<(libchromiumcontent_dir)/libpdfium.a',
                  '<(libchromiumcontent_dir)/libpdfium_skia_shared.a',
                  '<(libchromiumcontent_dir)/libfdrm.a',
                  '<(libchromiumcontent_dir)/libformfiller.a',
                  '<(libchromiumcontent_dir)/libfpdfapi.a',
                  '<(libchromiumcontent_dir)/libfpdfdoc.a',
                  '<(libchromiumcontent_dir)/libfpdftext.a',
                  '<(libchromiumcontent_dir)/libfxcodec.a',
                  '<(libchromiumcontent_dir)/libfxge.a',
                  '<(libchromiumcontent_dir)/libfxjs.a',
                  '<(libchromiumcontent_dir)/libpwl.a',
                  '<(libchromiumcontent_dir)/libfx_agg.a',
                  '<(libchromiumcontent_dir)/libfx_lcms2.a',
                  '<(libchromiumcontent_dir)/libfx_libopenjpeg.a',
                  '-Wl,--no-whole-archive',
                  '<(libchromiumcontent_dir)/libchrome.a',
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
                  '-lexpat',
                ],
              },
            }],
            # This lib does not exist on arm.
            ['target_arch=="arm"', {
              'link_settings': {
                'libraries!': [
                  '<(libchromiumcontent_dir)/libdesktop_capture_differ_sse2.a',
                ],
              },
            }],
            # Due to strange linker behavior, component build of arm needs to
            # be linked with libjpeg.a explicitly.
            ['target_arch=="arm" and libchromiumcontent_component==1', {
              'link_settings': {
                'libraries': [
                  '<(libchromiumcontent_dir)/libjpeg.a',
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
                  '<(libchromiumcontent_dir)/libbase_static.a',
                  '<(libchromiumcontent_dir)/libextras.a',
                  '<(libchromiumcontent_dir)/libhttp_server.a',
                  '<(libchromiumcontent_dir)/libdevice_service.a',
                  '<(libchromiumcontent_dir)/libdom_keycode_converter.a',
                  '<(libchromiumcontent_dir)/librtc_base.a',
                  '<(libchromiumcontent_dir)/librtc_base_generic.a',
                  '<(libchromiumcontent_dir)/libsystem_wrappers.a',
                  '<(libchromiumcontent_dir)/libwebrtc_common.a',
                  '<(libchromiumcontent_dir)/libinit_webrtc.a',
                  '<(libchromiumcontent_dir)/libyuv.a',
                  '<(libchromiumcontent_dir)/libpdfium_skia_shared.a',
                  '<(libchromiumcontent_dir)/librenderer.a',
                  '<(libchromiumcontent_dir)/libsecurity_state.a',
                  '<(libchromiumcontent_dir)/libviz_service.a',
                  '<(libchromiumcontent_dir)/libchrome.a',
                  # services/device/wake_lock/power_save_blocker/
                  '<(libchromiumcontent_dir)/libpower_save_blocker.a',
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
                  '<(libchromiumcontent_dir)/libfxge.a',
                  '<(libchromiumcontent_dir)/libfxjs.a',
                  '<(libchromiumcontent_dir)/libpwl.a',
                  '<(libchromiumcontent_dir)/libfx_agg.a',
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
                  '$(SDKROOT)/System/Library/Frameworks/ForceFeedback.framework',
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
          'link_settings': {
            'msvs_settings': {
              'VCLinkerTool': {
                'AdditionalOptions': [
                  # warning /DELAYLOAD:dll ignored; no imports found from dll
                  '/ignore:4199',
                ],
                'AdditionalDependencies': [
                  'delayimp.lib',
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
          'conditions': [
            ['libchromiumcontent_component', {
              'link_settings': {
                'libraries': [
                  # Needed by desktop_capture.lib:
                  '-ld3d11.lib',
                  '-ldxgi.lib',
                  # Following libs are always linked statically.
                  '<(libchromiumcontent_dir)/base_static.lib',
                  '<(libchromiumcontent_dir)/extras.lib',
                  '<(libchromiumcontent_dir)/sandbox.lib',
                  '<(libchromiumcontent_dir)/sandbox_helper_win.lib',
                  '<(libchromiumcontent_dir)/http_server.lib',
                  '<(libchromiumcontent_dir)/device_service.lib',
                  '<(libchromiumcontent_dir)/dom_keycode_converter.lib',
                  '<(libchromiumcontent_dir)/rtc_base.lib',
                  '<(libchromiumcontent_dir)/rtc_base_generic.lib',
                  '<(libchromiumcontent_dir)/system_wrappers.lib',
                  '<(libchromiumcontent_dir)/webrtc_common.lib',
                  '<(libchromiumcontent_dir)/init_webrtc.lib',
                  '<(libchromiumcontent_dir)/libyuv.lib',
                  '<(libchromiumcontent_dir)/pdfium_skia_shared.lib',
                  '<(libchromiumcontent_dir)/renderer.lib',
                  '<(libchromiumcontent_dir)/security_state.lib',
                  '<(libchromiumcontent_dir)/viz_service.lib',
                  '<(libchromiumcontent_dir)/chrome.lib',
                  # services/device/wake_lock/power_save_blocker/
                  '<(libchromiumcontent_dir)/power_save_blocker.lib',
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
                  '<(libchromiumcontent_dir)/fxge.lib',
                  '<(libchromiumcontent_dir)/fxjs.lib',
                  '<(libchromiumcontent_dir)/pwl.lib',
                  '<(libchromiumcontent_dir)/fx_agg.lib',
                  '<(libchromiumcontent_dir)/fx_lcms2.lib',
                  '<(libchromiumcontent_dir)/fx_libopenjpeg.lib',
                  '<(libchromiumcontent_dir)/fx_zlib.lib',
                  '<(libchromiumcontent_dir)/desktop_capture_generic.lib',
                  '<(libchromiumcontent_dir)/desktop_capture.lib',
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
                      'dwmapi.lib',
                      'gdi32.lib',
                      'hid.lib',
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
