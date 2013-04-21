{
  'variables': {
    'project_name': 'atom',
    'product_name': 'Atom',
    'app_sources': [
      'app/atom_main.cc',
    ],
    'coffee_sources': [
      'browser/api/lib/atom.coffee',
      'browser/api/lib/window.coffee',
      'browser/atom/atom.coffee',
    ],
    'lib_sources': [
      'app/atom_main_delegate.cc',
      'app/atom_main_delegate.h',
      'browser/api/atom_api_event.cc',
      'browser/api/atom_api_event.h',
      'browser/api/atom_api_event_emitter.cc',
      'browser/api/atom_api_event_emitter.h',
      'browser/api/atom_api_extensions.cc',
      'browser/api/atom_api_extensions.h',
      'browser/api/atom_api_objects_registry.cc',
      'browser/api/atom_api_objects_registry.h',
      'browser/api/atom_api_recorded_object.cc',
      'browser/api/atom_api_recorded_object.h',
      'browser/api/atom_api_window.cc',
      'browser/api/atom_api_window.h',
      'browser/api/atom_bindings.cc',
      'browser/api/atom_bindings.h',
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
      'common/api_messages.cc',
      'common/api_messages.h',
      'common/node_bindings.cc',
      'common/node_bindings.h',
      'common/node_bindings_mac.h',
      'common/node_bindings_mac.mm',
      'common/options_switches.cc',
      'common/options_switches.h',
      'common/v8_value_converter_impl.cc',
      'common/v8_value_converter_impl.h',
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
