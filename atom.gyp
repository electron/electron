{
  'variables': {
    'project_name': 'atom',
    'product_name': 'Atom',
    'app_sources': [
      'app/atom_main.cc',
      '<(libchromiumcontent_src_dir)/content/app/startup_helper_win.cc',
    ],
    'bundle_sources': [
      'browser/mac/atom.icns',
    ],
    'coffee_sources': [
      'browser/api/lib/app.coffee',
      'browser/api/lib/atom-delegate.coffee',
      'browser/api/lib/auto-updater.coffee',
      'browser/api/lib/browser-window.coffee',
      'browser/api/lib/crash-reporter.coffee',
      'browser/api/lib/dialog.coffee',
      'browser/api/lib/ipc.coffee',
      'browser/api/lib/menu.coffee',
      'browser/api/lib/menu-item.coffee',
      'browser/atom/atom.coffee',
      'browser/atom/objects-registry.coffee',
      'browser/atom/rpc-server.coffee',
      'common/api/lib/callbacks-registry.coffee',
      'common/api/lib/clipboard.coffee',
      'common/api/lib/id-weak-map.coffee',
      'common/api/lib/shell.coffee',
      'renderer/api/lib/ipc.coffee',
      'renderer/api/lib/remote.coffee',
    ],
    'lib_sources': [
      'app/atom_main_delegate.cc',
      'app/atom_main_delegate.h',
      'browser/accelerator_util.cc',
      'browser/accelerator_util.h',
      'browser/accelerator_util_mac.mm',
      'browser/api/atom_api_app.cc',
      'browser/api/atom_api_app.h',
      'browser/api/atom_api_auto_updater.cc',
      'browser/api/atom_api_auto_updater.h',
      'browser/api/atom_api_browser_ipc.cc',
      'browser/api/atom_api_browser_ipc.h',
      'browser/api/atom_api_crash_reporter.h',
      'browser/api/atom_api_crash_reporter.cc',
      'browser/api/atom_api_dialog.cc',
      'browser/api/atom_api_dialog.h',
      'browser/api/atom_api_event.cc',
      'browser/api/atom_api_event.h',
      'browser/api/atom_api_event_emitter.cc',
      'browser/api/atom_api_event_emitter.h',
      'browser/api/atom_api_menu.cc',
      'browser/api/atom_api_menu.h',
      'browser/api/atom_api_menu_mac.h',
      'browser/api/atom_api_menu_mac.mm',
      'browser/api/atom_api_window.cc',
      'browser/api/atom_api_window.h',
      'browser/api/atom_browser_bindings.cc',
      'browser/api/atom_browser_bindings.h',
      'browser/auto_updater.cc',
      'browser/auto_updater.h',
      'browser/auto_updater_delegate.cc',
      'browser/auto_updater_delegate.h',
      'browser/auto_updater_mac.mm',
      'browser/auto_updater_win.cc',
      'browser/atom_application_mac.h',
      'browser/atom_application_mac.mm',
      'browser/atom_application_delegate_mac.h',
      'browser/atom_application_delegate_mac.mm',
      'browser/atom_browser_client.cc',
      'browser/atom_browser_client.h',
      'browser/atom_browser_context.cc',
      'browser/atom_browser_context.h',
      'browser/atom_browser_main_parts.cc',
      'browser/atom_browser_main_parts.h',
      'browser/atom_browser_main_parts_mac.mm',
      'browser/atom_event_processing_window.h',
      'browser/atom_event_processing_window.mm',
      'browser/atom_javascript_dialog_manager.cc',
      'browser/atom_javascript_dialog_manager.h',
      'browser/browser.cc',
      'browser/browser.h',
      'browser/browser_mac.mm',
      'browser/browser_observer.h',
      'browser/crash_reporter.h',
      'browser/crash_reporter_mac.mm',
      'browser/crash_reporter_win.cc',
      'browser/file_dialog.h',
      'browser/file_dialog_mac.mm',
      'browser/media/media_capture_devices_dispatcher.cc',
      'browser/media/media_capture_devices_dispatcher.h',
      'browser/media/media_stream_devices_controller.cc',
      'browser/media/media_stream_devices_controller.h',
      'browser/message_box.h',
      'browser/message_box_mac.mm',
      'browser/native_window.cc',
      'browser/native_window.h',
      'browser/native_window_mac.h',
      'browser/native_window_mac.mm',
      'browser/native_window_observer.h',
      'browser/nsalert_synchronous_sheet.h',
      'browser/nsalert_synchronous_sheet.mm',
      'browser/window_list.cc',
      'browser/window_list.h',
      'browser/window_list_observer.h',
      'common/api/api_messages.cc',
      'common/api/api_messages.h',
      'common/api/atom_api_clipboard.cc',
      'common/api/atom_api_clipboard.h',
      'common/api/atom_api_idle_gc.cc',
      'common/api/atom_api_id_weak_map.cc',
      'common/api/atom_api_id_weak_map.h',
      'common/api/atom_api_shell.cc',
      'common/api/atom_api_shell.h',
      'common/api/atom_api_v8_util.cc',
      'common/api/atom_bindings.cc',
      'common/api/atom_bindings.h',
      'common/api/atom_extensions.cc',
      'common/api/atom_extensions.h',
      'common/api/object_life_monitor.cc',
      'common/api/object_life_monitor.h',
      'common/node_bindings.cc',
      'common/node_bindings.h',
      'common/node_bindings_mac.h',
      'common/node_bindings_mac.mm',
      'common/options_switches.cc',
      'common/options_switches.h',
      'common/platform_util.h',
      'common/platform_util_mac.mm',
      'common/platform_util_win.cc',
      'common/v8_value_converter_impl.cc',
      'common/v8_value_converter_impl.h',
      'renderer/api/atom_api_renderer_ipc.cc',
      'renderer/api/atom_api_renderer_ipc.h',
      'renderer/api/atom_renderer_bindings.cc',
      'renderer/api/atom_renderer_bindings.h',
      'renderer/atom_render_view_observer.cc',
      'renderer/atom_render_view_observer.h',
      'renderer/atom_renderer_client.cc',
      'renderer/atom_renderer_client.h',
    ],
    'framework_sources': [
      'app/atom_library_main.cc',
      'app/atom_library_main.h',
    ],
    'fix_framework_link_command': [
      'install_name_tool',
      '-change',
      '@loader_path/../Frameworks/Sparkle.framework/Versions/A/Sparkle',
      '@rpath/Sparkle.framework/Versions/A/Sparkle',
      '-change',
      '@executable_path/../Frameworks/Quincy.framework/Versions/A/Quincy',
      '@rpath/Quincy.framework/Versions/A/Quincy',
      '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}'
    ],
    'atom_source_root': '<!(python tools/atom_source_root.py)',
  },
  'target_defaults': {
    'mac_framework_dirs': [
      '<(atom_source_root)/frameworks',
    ],
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
      'mac_bundle_resources': [
        '<@(bundle_sources)',
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
            'INFOPLIST_FILE': 'browser/mac/Info.plist',
            'LD_RUNPATH_SEARCH_PATHS': [
              '@executable_path/../Frameworks',
            ],
          },
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)/<(product_name).app/Contents/Frameworks',
              'files': [
                '<(PRODUCT_DIR)/<(product_name) Helper.app',
                '<(PRODUCT_DIR)/<(product_name).framework',
                'frameworks/Sparkle.framework',
                'frameworks/Quincy.framework',
              ],
            },
            {
              'destination': '<(PRODUCT_DIR)/<(product_name).app/Contents/Resources/browser',
              'files': [
                'browser/default_app',
              ],
            },
            {
              # Copy node binary for worker process support.
              'destination': '<(PRODUCT_DIR)/<(product_name).app/Contents/Resources',
              'files': [
                'node/node',
              ],
            },
          ],
          'postbuilds': [
            {
              'postbuild_name': 'Fix Framework Link',
              'action': [
                '<@(fix_framework_link_command)',
              ],
            },
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
          ]
        }],
      ],
    },
    {
      'target_name': '<(project_name)_lib',
      'type': 'static_library',
      'dependencies': [
        'vendor/brightray/brightray.gyp:brightray',
        'vendor/node/node.gyp:node',
      ],
      'sources': [
        '<@(lib_sources)',
      ],
      'include_dirs': [
        '.',
        'vendor',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '.',
        ],
      },
      'export_dependent_settings': [
        'vendor/brightray/brightray.gyp:brightray',
      ],
    },
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
          'outputs': [
            '<(PRODUCT_DIR)/<(product_name).app/Contents/Resources/<(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).js',
          ],
          'action': [
            'python',
            'script/compile-coffee.py',
            '<(RULE_INPUT_PATH)',
            '<(PRODUCT_DIR)/<(product_name).app/Contents/Resources/<(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).js',
          ],
        },
      ],
    },
  ],
  'conditions': [
    ['OS=="mac"', {
      'targets': [
        {
          'target_name': '<(project_name)_framework',
          'product_name': '<(product_name)',
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
              'frameworks/Sparkle.framework',
              'frameworks/Quincy.framework',
            ],
          },
          'mac_bundle': 1,
          'mac_bundle_resources': [
            'browser/mac/MainMenu.xib',
            '<(libchromiumcontent_resources_dir)/content_shell.pak',
          ],
          'xcode_settings': {
            'LIBRARY_SEARCH_PATHS': [
              '<(libchromiumcontent_library_dir)',
            ],
            'LD_DYLIB_INSTALL_NAME': '@rpath/<(product_name).framework/<(product_name)',
            'LD_RUNPATH_SEARCH_PATHS': [
              '@loader_path/Libraries',
            ],
            'OTHER_LDFLAGS': [
              '-ObjC',
            ],
          },
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)/<(product_name).framework/Versions/A/Libraries',
              'files': [
                '<(libchromiumcontent_library_dir)/ffmpegsumo.so',
                '<(libchromiumcontent_library_dir)/libchromiumcontent.dylib',
              ],
            },
          ],
          'postbuilds': [
            {
              'postbuild_name': 'Fix Framework Link',
              'action': [
                '<@(fix_framework_link_command)',
              ],
            },
            {
              'postbuild_name': 'Add symlinks for framework subdirectories',
              'action': [
                'tools/mac/create-framework-subdir-symlinks.sh',
                '<(product_name)',
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
            'INFOPLIST_FILE': 'renderer/mac/Info.plist',
            'LD_RUNPATH_SEARCH_PATHS': [
              '@executable_path/../../..',
            ],
          },
          'postbuilds': [
            {
              'postbuild_name': 'Fix Framework Link',
              'action': [
                '<@(fix_framework_link_command)',
              ],
            },
          ],
        },  # target helper
      ],
    }],  # OS==Mac
  ],
}
