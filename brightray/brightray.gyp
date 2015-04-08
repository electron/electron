{
  'includes': [
    'brightray.gypi',
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
      'sources': [
        'browser/brightray_paths.h',
        'browser/browser_client.cc',
        'browser/browser_client.h',
        'browser/browser_context.cc',
        'browser/browser_context.h',
        'browser/browser_main_parts.cc',
        'browser/browser_main_parts.h',
        'browser/browser_main_parts_mac.mm',
        'browser/default_web_contents_delegate.cc',
        'browser/default_web_contents_delegate.h',
        'browser/default_web_contents_delegate_mac.mm',
        'browser/devtools_contents_resizing_strategy.cc',
        'browser/devtools_contents_resizing_strategy.h',
        'browser/devtools_embedder_message_dispatcher.cc',
        'browser/devtools_embedder_message_dispatcher.h',
        'browser/devtools_manager_delegate.cc',
        'browser/devtools_manager_delegate.h',
        'browser/devtools_ui.cc',
        'browser/devtools_ui.h',
        'browser/inspectable_web_contents.cc',
        'browser/inspectable_web_contents.h',
        'browser/inspectable_web_contents_delegate.cc',
        'browser/inspectable_web_contents_delegate.h',
        'browser/inspectable_web_contents_impl.cc',
        'browser/inspectable_web_contents_impl.h',
        'browser/inspectable_web_contents_view.h',
        'browser/inspectable_web_contents_view_mac.h',
        'browser/inspectable_web_contents_view_mac.mm',
        'browser/mac/bry_application.h',
        'browser/mac/bry_application.mm',
        'browser/mac/bry_inspectable_web_contents_view.h',
        'browser/mac/bry_inspectable_web_contents_view.mm',
        'browser/media/media_capture_devices_dispatcher.cc',
        'browser/media/media_capture_devices_dispatcher.h',
        'browser/media/media_stream_devices_controller.cc',
        'browser/media/media_stream_devices_controller.h',
        'browser/network_delegate.cc',
        'browser/network_delegate.h',
        'browser/notification_presenter.h',
        'browser/notification_presenter_mac.h',
        'browser/notification_presenter_mac.mm',
        'browser/platform_notification_service_impl.cc',
        'browser/platform_notification_service_impl.h',
        'browser/linux/notification_presenter_linux.h',
        'browser/linux/notification_presenter_linux.cc',
        'browser/url_request_context_getter.cc',
        'browser/url_request_context_getter.h',
        'browser/views/inspectable_web_contents_view_views.h',
        'browser/views/inspectable_web_contents_view_views.cc',
        'browser/views/views_delegate.cc',
        'browser/views/views_delegate.h',
        'browser/web_ui_controller_factory.cc',
        'browser/web_ui_controller_factory.h',
        'common/application_info.h',
        'common/application_info_mac.mm',
        'common/application_info_win.cc',
        'common/content_client.cc',
        'common/content_client.h',
        'common/mac/foundation_util.h',
        'common/mac/main_application_bundle.h',
        'common/mac/main_application_bundle.mm',
        'common/main_delegate.cc',
        'common/main_delegate.h',
        'common/main_delegate_mac.mm',
      ],
      'link_settings': {
        'libraries': [ '<@(libchromiumcontent_libraries)' ]
      },
      'conditions': [
        ['OS=="linux"', {
          'cflags_cc': [
            '-Wno-deprecated-register',
            '-fno-rtti',
          ],
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other gtk+-2.0 libnotify dbus-1 x11 xrandr xext gconf-2.0)',
            ],
            'libraries': [
              '-lpthread',
              '<!@(pkg-config --libs-only-l gtk+-2.0 libnotify dbus-1 x11 xrandr xext gconf-2.0)',
            ],
          },
        }],
        ['OS=="linux" and libchromiumcontent_component==0', {
          '<(libchromiumcontent_dir)/libboringssl.so',
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
              # This library is built as shared library to avoid symbols
              # conflict with Node.
              '<(libchromiumcontent_dir)/libboringssl.dylib',
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
              '<(libchromiumcontent_dir)/boringssl.dll',
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
