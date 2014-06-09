{
  'variables': {
    'includes': [
      'vendor/native_mate/native_mate_files.gypi',
    ],
    'project_name': 'atom',
    'product_name': 'Atom',
    'framework_name': 'Atom Framework',
    'app_sources': [
      'atom/app/atom_main.cc',
      'atom/app/atom_main.h',
    ],
    'bundle_sources': [
      'atom/browser/resources/mac/atom.icns',
    ],
    'coffee_sources': [
      'atom/browser/api/lib/app.coffee',
      'atom/browser/api/lib/atom-delegate.coffee',
      'atom/browser/api/lib/auto-updater.coffee',
      'atom/browser/api/lib/browser-window.coffee',
      'atom/browser/api/lib/dialog.coffee',
      'atom/browser/api/lib/ipc.coffee',
      'atom/browser/api/lib/menu.coffee',
      'atom/browser/api/lib/menu-item.coffee',
      'atom/browser/api/lib/power-monitor.coffee',
      'atom/browser/api/lib/protocol.coffee',
      'atom/browser/api/lib/tray.coffee',
      'atom/browser/api/lib/web-contents.coffee',
      'atom/browser/lib/init.coffee',
      'atom/browser/lib/objects-registry.coffee',
      'atom/browser/lib/rpc-server.coffee',
      'atom/common/api/lib/callbacks-registry.coffee',
      'atom/common/api/lib/clipboard.coffee',
      'atom/common/api/lib/crash-reporter.coffee',
      'atom/common/api/lib/id-weak-map.coffee',
      'atom/common/api/lib/screen.coffee',
      'atom/common/api/lib/shell.coffee',
      'atom/common/lib/init.coffee',
      'atom/renderer/lib/init.coffee',
      'atom/renderer/lib/inspector.coffee',
      'atom/renderer/lib/override.coffee',
      'atom/renderer/api/lib/ipc.coffee',
      'atom/renderer/api/lib/remote.coffee',
    ],
    'lib_sources': [
      'atom/app/atom_main_delegate.cc',
      'atom/app/atom_main_delegate.h',
      'atom/app/atom_main_delegate_mac.mm',
      'atom/browser/api/atom_api_app.cc',
      'atom/browser/api/atom_api_app.h',
      'atom/browser/api/atom_api_auto_updater.cc',
      'atom/browser/api/atom_api_auto_updater.h',
      'atom/browser/api/atom_api_dialog.cc',
      'atom/browser/api/atom_api_menu.cc',
      'atom/browser/api/atom_api_menu.h',
      'atom/browser/api/atom_api_menu_gtk.cc',
      'atom/browser/api/atom_api_menu_gtk.h',
      'atom/browser/api/atom_api_menu_mac.h',
      'atom/browser/api/atom_api_menu_mac.mm',
      'atom/browser/api/atom_api_menu_win.cc',
      'atom/browser/api/atom_api_menu_win.h',
      'atom/browser/api/atom_api_power_monitor.cc',
      'atom/browser/api/atom_api_power_monitor.h',
      'atom/browser/api/atom_api_protocol.cc',
      'atom/browser/api/atom_api_protocol.h',
      'atom/browser/api/atom_api_tray.cc',
      'atom/browser/api/atom_api_tray.h',
      'atom/browser/api/atom_api_web_contents.cc',
      'atom/browser/api/atom_api_web_contents.h',
      'atom/browser/api/atom_api_window.cc',
      'atom/browser/api/atom_api_window.h',
      'atom/browser/api/event.cc',
      'atom/browser/api/event.h',
      'atom/browser/api/event_emitter.cc',
      'atom/browser/api/event_emitter.h',
      'atom/browser/auto_updater.cc',
      'atom/browser/auto_updater.h',
      'atom/browser/auto_updater_delegate.h',
      'atom/browser/auto_updater_linux.cc',
      'atom/browser/auto_updater_mac.mm',
      'atom/browser/auto_updater_win.cc',
      'atom/browser/atom_browser_client.cc',
      'atom/browser/atom_browser_client.h',
      'atom/browser/atom_browser_context.cc',
      'atom/browser/atom_browser_context.h',
      'atom/browser/atom_browser_main_parts.cc',
      'atom/browser/atom_browser_main_parts.h',
      'atom/browser/atom_browser_main_parts_mac.mm',
      'atom/browser/atom_javascript_dialog_manager.cc',
      'atom/browser/atom_javascript_dialog_manager.h',
      'atom/browser/browser.cc',
      'atom/browser/browser.h',
      'atom/browser/browser_linux.cc',
      'atom/browser/browser_mac.mm',
      'atom/browser/browser_win.cc',
      'atom/browser/browser_observer.h',
      'atom/browser/devtools_delegate.cc',
      'atom/browser/devtools_delegate.h',
      'atom/browser/mac/atom_application.h',
      'atom/browser/mac/atom_application.mm',
      'atom/browser/mac/atom_application_delegate.h',
      'atom/browser/mac/atom_application_delegate.mm',
      'atom/browser/native_window.cc',
      'atom/browser/native_window.h',
      'atom/browser/native_window_gtk.cc',
      'atom/browser/native_window_gtk.h',
      'atom/browser/native_window_mac.h',
      'atom/browser/native_window_mac.mm',
      'atom/browser/native_window_win.cc',
      'atom/browser/native_window_win.h',
      'atom/browser/native_window_observer.h',
      'atom/browser/net/adapter_request_job.cc',
      'atom/browser/net/adapter_request_job.h',
      'atom/browser/net/atom_url_request_context_getter.cc',
      'atom/browser/net/atom_url_request_context_getter.h',
      'atom/browser/net/atom_url_request_job_factory.cc',
      'atom/browser/net/atom_url_request_job_factory.h',
      'atom/browser/net/url_request_string_job.cc',
      'atom/browser/net/url_request_string_job.h',
      'atom/browser/ui/accelerator_util.cc',
      'atom/browser/ui/accelerator_util.h',
      'atom/browser/ui/accelerator_util_gtk.cc',
      'atom/browser/ui/accelerator_util_mac.mm',
      'atom/browser/ui/accelerator_util_win.cc',
      'atom/browser/ui/cocoa/atom_menu_controller.h',
      'atom/browser/ui/cocoa/atom_menu_controller.mm',
      'atom/browser/ui/cocoa/event_processing_window.h',
      'atom/browser/ui/cocoa/event_processing_window.mm',
      'atom/browser/ui/file_dialog.h',
      'atom/browser/ui/file_dialog_gtk.cc',
      'atom/browser/ui/file_dialog_mac.mm',
      'atom/browser/ui/file_dialog_win.cc',
      'atom/browser/ui/gtk/app_indicator_icon.cc',
      'atom/browser/ui/gtk/app_indicator_icon.h',
      'atom/browser/ui/gtk/status_icon.cc',
      'atom/browser/ui/gtk/status_icon.h',
      'atom/browser/ui/message_box.h',
      'atom/browser/ui/message_box_gtk.cc',
      'atom/browser/ui/message_box_mac.mm',
      'atom/browser/ui/message_box_win.cc',
      'atom/browser/ui/tray_icon.cc',
      'atom/browser/ui/tray_icon.h',
      'atom/browser/ui/tray_icon_gtk.cc',
      'atom/browser/ui/tray_icon_cocoa.h',
      'atom/browser/ui/tray_icon_cocoa.mm',
      'atom/browser/ui/tray_icon_observer.h',
      'atom/browser/ui/tray_icon_win.cc',
      'atom/browser/ui/win/menu_2.cc',
      'atom/browser/ui/win/menu_2.h',
      'atom/browser/ui/win/native_menu_win.cc',
      'atom/browser/ui/win/native_menu_win.h',
      'atom/browser/ui/win/notify_icon_host.cc',
      'atom/browser/ui/win/notify_icon_host.h',
      'atom/browser/ui/win/notify_icon.cc',
      'atom/browser/ui/win/notify_icon.h',
      'atom/browser/window_list.cc',
      'atom/browser/window_list.h',
      'atom/browser/window_list_observer.h',
      'atom/common/api/api_messages.cc',
      'atom/common/api/api_messages.h',
      'atom/common/api/atom_api_clipboard.cc',
      'atom/common/api/atom_api_crash_reporter.cc',
      'atom/common/api/atom_api_id_weak_map.cc',
      'atom/common/api/atom_api_id_weak_map.h',
      'atom/common/api/atom_api_screen.cc',
      'atom/common/api/atom_api_screen.h',
      'atom/common/api/atom_api_shell.cc',
      'atom/common/api/atom_api_v8_util.cc',
      'atom/common/api/atom_bindings.cc',
      'atom/common/api/atom_bindings.h',
      'atom/common/api/atom_extensions.cc',
      'atom/common/api/atom_extensions.h',
      'atom/common/api/object_life_monitor.cc',
      'atom/common/api/object_life_monitor.h',
      'atom/common/browser_v8_locker.cc',
      'atom/common/browser_v8_locker.h',
      'atom/common/crash_reporter/crash_reporter.cc',
      'atom/common/crash_reporter/crash_reporter.h',
      'atom/common/crash_reporter/crash_reporter_linux.cc',
      'atom/common/crash_reporter/crash_reporter_linux.h',
      'atom/common/crash_reporter/crash_reporter_mac.h',
      'atom/common/crash_reporter/crash_reporter_mac.mm',
      'atom/common/crash_reporter/crash_reporter_win.cc',
      'atom/common/crash_reporter/crash_reporter_win.h',
      'atom/common/crash_reporter/linux/crash_dump_handler.cc',
      'atom/common/crash_reporter/linux/crash_dump_handler.h',
      'atom/common/crash_reporter/win/crash_service.cc',
      'atom/common/crash_reporter/win/crash_service.h',
      'atom/common/crash_reporter/win/crash_service_main.cc',
      'atom/common/crash_reporter/win/crash_service_main.h',
      'atom/common/draggable_region.cc',
      'atom/common/draggable_region.h',
      'atom/common/linux/application_info.cc',
      'atom/common/native_mate_converters/file_path_converter.h',
      'atom/common/native_mate_converters/function_converter.h',
      'atom/common/native_mate_converters/gurl_converter.h',
      'atom/common/native_mate_converters/image_converter.cc',
      'atom/common/native_mate_converters/image_converter.h',
      'atom/common/native_mate_converters/string16_converter.h',
      'atom/common/native_mate_converters/v8_value_converter.cc',
      'atom/common/native_mate_converters/v8_value_converter.h',
      'atom/common/native_mate_converters/value_converter.cc',
      'atom/common/native_mate_converters/value_converter.h',
      'atom/common/node_bindings.cc',
      'atom/common/node_bindings.h',
      'atom/common/node_bindings_linux.cc',
      'atom/common/node_bindings_linux.h',
      'atom/common/node_bindings_mac.cc',
      'atom/common/node_bindings_mac.h',
      'atom/common/node_bindings_win.cc',
      'atom/common/node_bindings_win.h',
      'atom/common/node_includes.h',
      'atom/common/options_switches.cc',
      'atom/common/options_switches.h',
      'atom/common/platform_util.h',
      'atom/common/platform_util_linux.cc',
      'atom/common/platform_util_mac.mm',
      'atom/common/platform_util_win.cc',
      'atom/renderer/api/atom_api_renderer_ipc.cc',
      'atom/renderer/api/atom_renderer_bindings.cc',
      'atom/renderer/api/atom_renderer_bindings.h',
      'atom/renderer/atom_render_view_observer.cc',
      'atom/renderer/atom_render_view_observer.h',
      'atom/renderer/atom_renderer_client.cc',
      'atom/renderer/atom_renderer_client.h',
      'atom/renderer/autofill/page_click_listener.h',
      'atom/renderer/autofill/page_click_tracker.cc',
      'atom/renderer/autofill/page_click_tracker.h',
      'chrome/browser/ui/gtk/event_utils.cc',
      'chrome/browser/ui/gtk/event_utils.h',
      'chrome/browser/ui/gtk/gtk_custom_menu.cc',
      'chrome/browser/ui/gtk/gtk_custom_menu.h',
      'chrome/browser/ui/gtk/gtk_custom_menu_item.cc',
      'chrome/browser/ui/gtk/gtk_custom_menu_item.h',
      'chrome/browser/ui/gtk/gtk_util.cc',
      'chrome/browser/ui/gtk/gtk_util.h',
      'chrome/browser/ui/gtk/gtk_window_util.cc',
      'chrome/browser/ui/gtk/gtk_window_util.h',
      'chrome/browser/ui/gtk/menu_gtk.cc',
      'chrome/browser/ui/gtk/menu_gtk.h',
      'chrome/browser/ui/views/status_icons/status_tray_state_changer_win.cc',
      'chrome/browser/ui/views/status_icons/status_tray_state_changer_win.h',
      '<@(native_mate_files)',
    ],
    'framework_sources': [
      'atom/app/atom_library_main.cc',
      'atom/app/atom_library_main.h',
    ],
    'locales': [
      'am', 'ar', 'bg', 'bn', 'ca', 'cs', 'da', 'de', 'el', 'en-GB',
      'en-US', 'es-419', 'es', 'et', 'fa', 'fi', 'fil', 'fr', 'gu', 'he',
      'hi', 'hr', 'hu', 'id', 'it', 'ja', 'kn', 'ko', 'lt', 'lv',
      'ml', 'mr', 'ms', 'nb', 'nl', 'pl', 'pt-BR', 'pt-PT', 'ro', 'ru',
      'sk', 'sl', 'sr', 'sv', 'sw', 'ta', 'te', 'th', 'tr', 'uk',
      'vi', 'zh-CN', 'zh-TW',
    ],
    'atom_source_root': '<!(python tools/atom_source_root.py)',
    'conditions': [
      ['OS=="win"', {
        'app_sources': [
          'atom/browser/resources/win/resource.h',
          'atom/browser/resources/win/atom.ico',
          'atom/browser/resources/win/atom.rc',
          '<(libchromiumcontent_src_dir)/content/app/startup_helper_win.cc',
        ],
      }],  # OS=="win"
      ['OS=="mac"', {
        'apply_locales_cmd': ['python', 'tools/mac/apply_locales.py'],
      }],  # OS=="mac"
    ],
  },
  'target_defaults': {
    'mac_framework_dirs': [
      '<(atom_source_root)/external_binaries',
    ],
    'includes': [
       # Rules for excluding e.g. foo_win.cc from the build on non-Windows.
      'filename_rules.gypi',
    ],
    'configurations': {
      'Debug': {
        'defines': [ 'DEBUG' ],
        'cflags': [ '-g', '-O0' ],
      },
    },
  },
  'targets': [
    {
      'target_name': '<(project_name)',
      'type': 'executable',
      'dependencies': [
        'generated_sources',
        '<(project_name)_lib',
      ],
      'sources': [
        '<@(app_sources)',
      ],
      'include_dirs': [
        '.',
      ],
      'conditions': [
        ['OS=="mac"', {
          'product_name': '<(product_name)',
          'mac_bundle': 1,
          'dependencies!': [
            '<(project_name)_lib',
          ],
          'dependencies': [
            '<(project_name)_framework',
            '<(project_name)_helper',
          ],
          'xcode_settings': {
            'INFOPLIST_FILE': 'atom/browser/resources/mac/Info.plist',
            'LD_RUNPATH_SEARCH_PATHS': [
              '@executable_path/../Frameworks',
            ],
          },
          'mac_bundle_resources': [
            '<@(bundle_sources)',
          ],
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)/<(product_name).app/Contents/Frameworks',
              'files': [
                '<(PRODUCT_DIR)/<(product_name) Helper.app',
                '<(PRODUCT_DIR)/<(framework_name).framework',
                'external_binaries/Squirrel.framework',
                'external_binaries/ReactiveCocoa.framework',
                'external_binaries/Mantle.framework',
              ],
            },
            {
              'destination': '<(PRODUCT_DIR)/<(product_name).app/Contents/Resources',
              'files': [
                'atom/browser/default_app',
              ],
            },
          ],
          'postbuilds': [
            {
              # This postbuid step is responsible for creating the following
              # helpers:
              #
              # <(product_name) EH.app and <(product_name) NP.app are created
              # from <(product_name).app.
              #
              # The EH helper is marked for an executable heap. The NP helper
              # is marked for no PIE (ASLR).
              'postbuild_name': 'Make More Helpers',
              'action': [
                'vendor/brightray/tools/mac/make_more_helpers.sh',
                'Frameworks',
                '<(product_name)',
              ],
            },
              # The application doesn't have real localizations, it just has
              # empty .lproj directories, which is enough to convince Cocoa
              # atom-shell supports those languages.
            {
              'postbuild_name': 'Make Empty Localizations',
              'variables': {
                'locale_dirs': [
                  '>!@(<(apply_locales_cmd) -d ZZLOCALE.lproj <(locales))',
                ],
              },
              'action': [
                'tools/mac/make_locale_dirs.sh',
                '<@(locale_dirs)',
              ],
            },
          ]
        }, {  # OS=="mac"
          'dependencies': [
            'make_locale_paks',
          ],
        }],  # OS!="mac"
        ['OS=="win"', {
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)',
              'files': [
                '<(libchromiumcontent_library_dir)/chromiumcontent.dll',
                '<(libchromiumcontent_library_dir)/ffmpegsumo.dll',
                '<(libchromiumcontent_library_dir)/icudt.dll',
                '<(libchromiumcontent_library_dir)/libEGL.dll',
                '<(libchromiumcontent_library_dir)/libGLESv2.dll',
                '<(libchromiumcontent_resources_dir)/content_shell.pak',
                'external_binaries/d3dcompiler_43.dll',
                'external_binaries/xinput1_3.dll',
              ],
            },
            {
              'destination': '<(PRODUCT_DIR)/resources',
              'files': [
                'atom/browser/default_app',
              ]
            },
          ],
        }],  # OS=="win"
        ['OS=="linux"', {
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)',
              'files': [
                '<(libchromiumcontent_library_dir)/libchromiumcontent.so',
                '<(libchromiumcontent_library_dir)/libffmpegsumo.so',
                '<(libchromiumcontent_resources_dir)/content_shell.pak',
              ],
            },
            {
              'destination': '<(PRODUCT_DIR)/resources',
              'files': [
                'atom/browser/default_app',
              ]
            },
          ],
        }],  # OS=="linux"
      ],
    },  # target <(project_name)
    {
      'target_name': '<(project_name)_lib',
      'type': 'static_library',
      'dependencies': [
        'vendor/brightray/brightray.gyp:brightray',
        'vendor/node/node.gyp:node_lib',
      ],
      'sources': [
        '<@(lib_sources)',
      ],
      'include_dirs': [
        '.',
        'vendor/brightray',
        'vendor/native_mate',
        # Include directories for uv and node.
        'vendor/node/src',
        'vendor/node/deps/http_parser',
        'vendor/node/deps/uv/include',
        # The `node.h` is using `#include"v8.h"`.
        'vendor/brightray/vendor/download/libchromiumcontent/src/v8/include',
        # The `node.h` is using `#include"ares.h"`.
        'vendor/node/deps/cares/include',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '.',
        ],
      },
      'export_dependent_settings': [
        'vendor/brightray/brightray.gyp:brightray',
      ],
      'conditions': [
        ['OS=="win"', {
          'link_settings': {
            'libraries': [
              '-limm32.lib',
              '-loleacc.lib',
              '-lComdlg32.lib',
              '-lWininet.lib',
              '<(atom_source_root)/<(libchromiumcontent_library_dir)/chromiumviews.lib',
            ],
          },
          'dependencies': [
            'vendor/breakpad/breakpad.gyp:breakpad_handler',
            'vendor/breakpad/breakpad.gyp:breakpad_sender',
          ],
        }],  # OS=="win"
        ['OS=="mac"', {
          'dependencies': [
            'vendor/breakpad/breakpad.gyp:breakpad',
          ],
        }],  # OS=="mac"
        ['OS=="linux"', {
          'link_settings': {
            'ldflags': [
              # Make binary search for libraries under current directory, so we
              # don't have to manually set $LD_LIBRARY_PATH:
              # http://serverfault.com/questions/279068/cant-find-so-in-the-same-directory-as-the-executable
              '-rpath \$$ORIGIN',
              # Make native module dynamic loading work.
              '-rdynamic',
            ],
          },
          # Required settings of using breakpad.
          'include_dirs': [
            'vendor/breakpad/src',
          ],
          'cflags': [
            '-Wno-empty-body',
          ],
          'dependencies': [
            'vendor/breakpad/breakpad.gyp:breakpad_client',
          ],
        }],  # OS=="linux"
      ],
    },  # target <(product_name)_lib
    {
      'target_name': 'generated_sources',
      'type': 'none',
      'sources': [
        '<@(coffee_sources)',
      ],
      'rules': [
        {
          'rule_name': 'coffee',
          'extension': 'coffee',
          'inputs': [
            'script/compile-coffee.py',
          ],
          'conditions': [
            ['OS=="mac"', {
              'outputs': [
                '<(PRODUCT_DIR)/<(product_name).app/Contents/Resources/<(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).js',
              ],
              'action': [
                'python',
                'script/compile-coffee.py',
                '<(RULE_INPUT_PATH)',
                '<(PRODUCT_DIR)/<(product_name).app/Contents/Resources/<(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).js',
              ],
            },{  # OS=="mac"
              'outputs': [
                '<(PRODUCT_DIR)/resources/<(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).js',
              ],
              'action': [
                'python',
                'script/compile-coffee.py',
                '<(RULE_INPUT_PATH)',
                '<(PRODUCT_DIR)/resources/<(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).js',
              ],
            }],  # OS=="win" or OS=="linux"
          ],
        },
      ],
    },  # target generated_sources
    {
      'target_name': '<(project_name)_dump_symbols',
      'type': 'none',
      'dependencies': [
        '<(project_name)',
      ],
      'conditions': [
        ['OS=="mac"', {
          'dependencies': [
            'vendor/breakpad/breakpad.gyp:dump_syms',
          ],
          'actions': [
            {
              'action_name': 'Dump Symbols',
              'inputs': [
                '<(PRODUCT_DIR)/<(product_name).app/Contents/MacOS/<(product_name)',
              ],
              'outputs': [
                '<(PRODUCT_DIR)/Atom-Shell.breakpad.syms',
              ],
              'action': [
                'python',
                'tools/posix/generate_breakpad_symbols.py',
                '--build-dir=<(PRODUCT_DIR)',
                '--binary=<(PRODUCT_DIR)/<(product_name).app/Contents/MacOS/<(product_name)',
                '--symbols-dir=<(PRODUCT_DIR)/Atom-Shell.breakpad.syms',
                '--libchromiumcontent-dir=<(libchromiumcontent_library_dir)',
                '--clear',
                '--jobs=16',
              ],
            },
          ],
        }],  # OS=="mac"
        ['OS=="win"', {
          'actions': [
            {
              'action_name': 'Dump Symbols',
              'inputs': [
                '<(PRODUCT_DIR)/<(project_name).exe',
              ],
              'outputs': [
                '<(PRODUCT_DIR)/Atom-Shell.breakpad.syms',
              ],
              'action': [
                'python',
                'tools/win/generate_breakpad_symbols.py',
                '--symbols-dir=<(PRODUCT_DIR)/Atom-Shell.breakpad.syms',
                '--jobs=16',
                '<(PRODUCT_DIR)',
                '<(libchromiumcontent_library_dir)',
              ],
            },
          ],
        }],  # OS=="win"
        ['OS=="linux"', {
          'dependencies': [
            'vendor/breakpad/breakpad.gyp:dump_syms',
          ],
          'actions': [
            {
              'action_name': 'Dump Symbols',
              'inputs': [
                '<(PRODUCT_DIR)/<(project_name)',
              ],
              'outputs': [
                '<(PRODUCT_DIR)/Atom-Shell.breakpad.syms',
              ],
              'action': [
                'python',
                'tools/posix/generate_breakpad_symbols.py',
                '--build-dir=<(PRODUCT_DIR)',
                '--binary=<(PRODUCT_DIR)/<(project_name)',
                '--symbols-dir=<(PRODUCT_DIR)/Atom-Shell.breakpad.syms',
                '--libchromiumcontent-dir=<(libchromiumcontent_library_dir)',
                '--clear',
                '--jobs=16',
              ],
            },
            {
              'action_name': 'Strip Binary',
              'inputs': [
                '<(PRODUCT_DIR)/libchromiumcontent.so',
                '<(PRODUCT_DIR)/libffmpegsumo.so',
                '<(PRODUCT_DIR)/<(project_name)',
                # Add the syms folder as input would force this action to run
                # after the 'Dump Symbols' action. And since it is a folder,
                # it would be ignored by the 'strip' command.
                '<(PRODUCT_DIR)/Atom-Shell.breakpad.syms',
              ],
              'outputs': [
                # Gyp action requires a output file, add a fake one here.
                '<(PRODUCT_DIR)/dummy_file',
              ],
              'action': [ 'strip', '<@(_inputs)' ],
            },
          ],
        }],  # OS=="linux"
      ],
    },  # target <(project_name>_dump_symbols
  ],
  'conditions': [
    ['OS=="mac"', {
      'targets': [
        {
          'target_name': '<(project_name)_framework',
          'product_name': '<(framework_name)',
          'type': 'shared_library',
          'dependencies': [
            '<(project_name)_lib',
          ],
          'sources': [
            '<@(framework_sources)',
          ],
          'include_dirs': [
            '.',
            'vendor',
            '<(libchromiumcontent_include_dir)',
          ],
          'export_dependent_settings': [
            '<(project_name)_lib',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
              'external_binaries/Squirrel.framework',
              'external_binaries/ReactiveCocoa.framework',
              'external_binaries/Mantle.framework',
            ],
          },
          'mac_bundle': 1,
          'mac_bundle_resources': [
            'atom/common/resources/mac/MainMenu.xib',
            '<(libchromiumcontent_resources_dir)/content_shell.pak',
          ],
          'xcode_settings': {
            'INFOPLIST_FILE': 'atom/common/resources/mac/Info.plist',
            'LIBRARY_SEARCH_PATHS': [
              '<(libchromiumcontent_library_dir)',
            ],
            'LD_DYLIB_INSTALL_NAME': '@rpath/<(framework_name).framework/<(framework_name)',
            'LD_RUNPATH_SEARCH_PATHS': [
              '@loader_path/Libraries',
            ],
            'OTHER_LDFLAGS': [
              '-ObjC',
            ],
          },
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)/<(framework_name).framework/Versions/A/Libraries',
              'files': [
                '<(libchromiumcontent_library_dir)/ffmpegsumo.so',
                '<(libchromiumcontent_library_dir)/libchromiumcontent.dylib',
              ],
            },
            {
              'destination': '<(PRODUCT_DIR)/<(framework_name).framework/Versions/A/Resources',
              'files': [
                '<(PRODUCT_DIR)/Inspector',
                '<(PRODUCT_DIR)/crash_report_sender.app',
              ],
            },
          ],
          'postbuilds': [
            {
              'postbuild_name': 'Add symlinks for framework subdirectories',
              'action': [
                'tools/mac/create-framework-subdir-symlinks.sh',
                '<(framework_name)',
                'Libraries',
                'Frameworks',
              ],
            },
          ],
        },  # target framework
        {
          'target_name': '<(project_name)_helper',
          'product_name': '<(product_name) Helper',
          'type': 'executable',
          'dependencies': [
            '<(project_name)_framework',
          ],
          'sources': [
            '<@(app_sources)',
          ],
          'include_dirs': [
            '.',
          ],
          'mac_bundle': 1,
          'xcode_settings': {
            'INFOPLIST_FILE': 'atom/renderer/resources/mac/Info.plist',
            'LD_RUNPATH_SEARCH_PATHS': [
              '@executable_path/../../..',
            ],
          },
        },  # target helper
      ],
    }, {  # OS=="mac"
      'targets': [
        {
          'target_name': 'make_locale_paks',
          'type': 'none',
          'actions': [
            {
              'action_name': 'Make Empty Paks',
              'inputs': [
                'tools/posix/make_locale_paks.sh',
              ],
              'outputs': [
                '<(PRODUCT_DIR)/locales'
              ],
              'action': [
                'tools/posix/make_locale_paks.sh',
                '<(PRODUCT_DIR)',
                '<@(locales)',
              ],
              'msvs_cygwin_shell': 0,
            },
          ],
        },
      ],
    }],  # OS!="mac"
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'generate_node_lib',
          'type': 'none',
          'dependencies': [
            '<(project_name)',
          ],
          'actions': [
            {
              'action_name': 'Create node.lib',
              'inputs': [
                '<(PRODUCT_DIR)/atom.lib',
                '<(libchromiumcontent_library_dir)/chromiumcontent.dll.lib',
              ],
              'outputs': [
                '<(PRODUCT_DIR)/node.lib',
              ],
              'action': [
                'lib.exe',
                '/nologo',
                # We can't use <(_outputs) here because that escapes the
                # backslash in the path, which confuses lib.exe.
                '/OUT:<(PRODUCT_DIR)\\node.lib',
                '<@(_inputs)',
              ],
              'msvs_cygwin_shell': 0,
            },
          ],
        },  # target generate_node_lib
      ],
    }],  # OS==win
  ],
}
