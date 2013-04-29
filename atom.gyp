{
  'variables': {
    'project_name': 'atom',
    'product_name': 'Atom',
    'app_sources': [
      'app/atom_main.cc',
    ],
    'coffee_sources': [
      'browser/api/lib/atom.coffee',
      'browser/api/lib/ipc.coffee',
      'browser/api/lib/window.coffee',
      'browser/atom/atom.coffee',
      'browser/atom/objects_registry.coffee',
      'browser/atom/rpc_server.coffee',
      'common/api/lib/id_weak_map.coffee',
      'common/api/lib/shell.coffee',
      'renderer/api/lib/ipc.coffee',
      'renderer/api/lib/remote.coffee',
    ],
    'lib_sources': [
      'app/atom_main_delegate.cc',
      'app/atom_main_delegate.h',
      'browser/api/atom_api_browser_ipc.cc',
      'browser/api/atom_api_browser_ipc.h',
      'browser/api/atom_api_event.cc',
      'browser/api/atom_api_event.h',
      'browser/api/atom_api_event_emitter.cc',
      'browser/api/atom_api_event_emitter.h',
      'browser/api/atom_api_window.cc',
      'browser/api/atom_api_window.h',
      'browser/api/atom_browser_bindings.cc',
      'browser/api/atom_browser_bindings.h',
      'browser/atom_browser_client.cc',
      'browser/atom_browser_client.h',
      'browser/atom_browser_context.cc',
      'browser/atom_browser_context.h',
      'browser/atom_browser_main_parts.cc',
      'browser/atom_browser_main_parts.h',
      'browser/atom_event_processing_window.h',
      'browser/atom_event_processing_window.mm',
      'browser/native_window.cc',
      'browser/native_window.h',
      'browser/native_window_mac.h',
      'browser/native_window_mac.mm',
      'browser/native_window_observer.h',
      'common/api/api_messages.cc',
      'common/api/api_messages.h',
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
      'common/platform_util_mac.mm',
      'common/platform_util.h',
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
  },
  'includes': [
    'vendor/brightray/brightray.gypi'
  ],
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
            'INFOPLIST_FILE': 'browser/mac/Info.plist',
            'LD_RUNPATH_SEARCH_PATHS': '@executable_path/../Frameworks',
          },
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)/<(product_name).app/Contents/Frameworks',
              'files': [
                '<(PRODUCT_DIR)/<(product_name) Helper.app',
                '<(PRODUCT_DIR)/<(product_name).framework',
              ],
            },
            {
              'destination': '<(PRODUCT_DIR)/<(product_name).app/Contents/Resources/browser',
              'files': [
                'browser/default_app',
              ],
            }
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
      'conditions': [
        ['OS=="mac"', {
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
            ],
          },
        }],
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
            'script/compile-coffee',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/<(product_name).app/Contents/Resources/<(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).js',
          ],
          'action': [
            'sh',
            'script/compile-coffee',
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
          'mac_bundle': 1,
          'mac_bundle_resources': [
            'browser/mac/MainMenu.xib',
            '<(libchromiumcontent_resources_dir)/content_shell.pak',
          ],
          'xcode_settings': {
            'LIBRARY_SEARCH_PATHS': '<(libchromiumcontent_library_dir)',
            'LD_DYLIB_INSTALL_NAME': '@rpath/<(product_name).framework/<(product_name)',
            'LD_RUNPATH_SEARCH_PATHS': '@loader_path/Libraries',
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
        },
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
            'LD_RUNPATH_SEARCH_PATHS': '@executable_path/../../../../Frameworks',
          },
        },
      ],
    }],
  ],
}
