{
  'includes': [
    'extensions.gypi',
  ],
  'conditions': [
    ['OS=="linux"', {
      'targets': [
        {
          'target_name': 'xscrnsaver',
          'type': 'none',
          'direct_dependent_settings': {
            'cflags': [
              '<!@(<(pkg-config) --cflags xscrnsaver)',
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(<(pkg-config) --libs-only-L --libs-only-other xscrnsaver)',
            ],
            'libraries': [
              '<!@(<(pkg-config) --libs-only-l xscrnsaver)',
            ],
          },
        },
      ]
    }]
  ],
  'targets': [
    {
      'target_name': 'atom_resources',
      'type': 'none',
      'variables': {
        'grit_dir': '<(libchromiumcontent_src_dir)/tools/grit',
        'grd_file': 'atom/atom_resources.grd',
      },
      'sources': [
        '<(grd_file)',
        'atom/common/api/resources/_api_features.json',
        'atom/common/api/resources/_permission_features.json',
        'atom/common/api/resources/browser_action_bindings.js',
        'atom/common/api/resources/content_settings_bindings.js',
        'atom/common/api/resources/context_menus_bindings.js',
        'atom/common/api/resources/event_emitter.js',
        'atom/common/api/resources/ipc.json',
        'atom/common/api/resources/ipc_bindings.js',
        'atom/common/api/resources/ipc_utils.js',
        'atom/common/api/resources/privacy_bindings.js',
        'atom/common/api/resources/tabs_bindings.js',
        'atom/common/api/resources/web_frame.json',
        'atom/common/api/resources/web_frame_bindings.js',
        'atom/common/api/resources/windows_bindings.js',
      ],
      'actions': [
        {
          'action_name': 'generate_atom_resources',
          'inputs': [
            '<(grit_dir)/grit.py',
            '<(grd_file)',
            'atom/common/api/resources/_api_features.json',
            'atom/common/api/resources/_permission_features.json',
            'atom/common/api/resources/browser_action_bindings.js',
            'atom/common/api/resources/content_settings_bindings.js',
            'atom/common/api/resources/context_menus_bindings.js',
            'atom/common/api/resources/event_emitter.js',
            'atom/common/api/resources/ipc.json',
            'atom/common/api/resources/ipc_bindings.js',
            'atom/common/api/resources/ipc_utils.js',
            'atom/common/api/resources/privacy_bindings.js',
            'atom/common/api/resources/tabs_bindings.js',
            'atom/common/api/resources/web_frame.json',
            'atom/common/api/resources/web_frame_bindings.js',
            'atom/common/api/resources/windows_bindings.js',
          ],
          'outputs': [
            '<(grit_out_dir)/grit/atom_resources.h',
            '<(grit_out_dir)/atom_resources.pak',
          ],
          'action': [
            'python',
            '<(grit_dir)/grit.py',
            '-i',
            '<(grd_file)',
            'build',
            '-f',
            'resource_ids',
            '-o',
            '<(grit_out_dir)',
          ],
        },
      ],
      'direct_dependent_settings': {
        'conditions': [
          ['OS=="mac"', {
            'mac_bundle_resources': [
              '<(libchromiumcontent_dir)/extensions_resources.pak',
              '<(grit_out_dir)/atom_resources.pak',
              '<(libchromiumcontent_dir)/extensions_renderer_resources.pak',
              '<(libchromiumcontent_dir)/extensions_api_resources.pak',
            ]
          }],
          ['OS=="win" or OS=="linux"', {
            'copies': [
              {
                'destination': '<(PRODUCT_DIR)',
                'files': [
                  '<(libchromiumcontent_dir)/extensions_resources.pak',
                  '<(grit_out_dir)/atom_resources.pak',
                  '<(libchromiumcontent_dir)/extensions_renderer_resources.pak',
                  '<(libchromiumcontent_dir)/extensions_api_resources.pak',
                ],
              },
            ]
          }]
        ]
      }
    },
  ]
}
