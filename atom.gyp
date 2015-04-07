{
  'variables': {
    'project_name%': 'atom',
    'product_name%': 'Atom',

    'atom_source_root': '<!(["python", "tools/atom_source_root.py"])',
  },
  'includes': [
    'filenames.gypi',
    'vendor/native_mate/native_mate_files.gypi',
  ],
  'target_defaults': {
    'mac_framework_dirs': [
      '<(atom_source_root)/external_binaries',
    ],
    'includes': [
       # Rules for excluding e.g. foo_win.cc from the build on non-Windows.
      'filename_rules.gypi',
    ],
    'conditions': [
      ['libchromiumcontent_component', {
        'configurations': {
          'Debug': {
            'defines': [ 'DEBUG' ],
            'cflags': [ '-g', '-O0' ],
          },
        },
      }],
    ],
  },
  'targets': [
    {
      'target_name': '<(project_name)',
      'type': 'executable',
      'dependencies': [
        'compile_coffee',
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
                '<(PRODUCT_DIR)/<(product_name) Framework.framework',
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
                'apply_locales_cmd': ['python', 'tools/mac/apply_locales.py'],
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
              'variables': {
                'conditions': [
                  ['libchromiumcontent_component', {
                    'copied_libraries': '<(libchromiumcontent_shared_libraries)',
                  }, {
                    'copied_libraries': [],
                  }],
                ],
              },
              'destination': '<(PRODUCT_DIR)',
              'files': [
                '<@(copied_libraries)',
                '<(libchromiumcontent_dir)/ffmpegsumo.dll',
                '<(libchromiumcontent_dir)/libEGL.dll',
                '<(libchromiumcontent_dir)/libGLESv2.dll',
                '<(libchromiumcontent_dir)/icudtl.dat',
                '<(libchromiumcontent_dir)/content_resources_200_percent.pak',
                '<(libchromiumcontent_dir)/content_shell.pak',
                '<(libchromiumcontent_dir)/ui_resources_200_percent.pak',
                '<(libchromiumcontent_dir)/natives_blob.bin',
                '<(libchromiumcontent_dir)/snapshot_blob.bin',
                'external_binaries/d3dcompiler_47.dll',
                'external_binaries/msvcp120.dll',
                'external_binaries/msvcr120.dll',
                'external_binaries/vccorlib120.dll',
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
                '<(libchromiumcontent_dir)/libffmpegsumo.so',
                '<(libchromiumcontent_dir)/icudtl.dat',
                '<(libchromiumcontent_dir)/content_shell.pak',
                '<(libchromiumcontent_dir)/natives_blob.bin',
                '<(libchromiumcontent_dir)/snapshot_blob.bin',
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
        'atom_coffee2c',
        'vendor/brightray/brightray.gyp:brightray',
        'vendor/node/node.gyp:node',
      ],
      'defines': [
        'PRODUCT_NAME="<(product_name)"',
        # This is defined in skia/skia_common.gypi.
        'SK_SUPPORT_LEGACY_GETTOPDEVICE',
        # Disable warnings for g_settings_list_schemas.
        'GLIB_DISABLE_DEPRECATION_WARNINGS',
        # Defined in Chromium but not exposed in its gyp file.
        'V8_USE_EXTERNAL_STARTUP_DATA',
        'ENABLE_PLUGINS',
        # Needed by Node.
        'NODE_WANT_INTERNALS=1',
      ],
      'sources': [
        '<@(lib_sources)',
      ],
      'include_dirs': [
        '.',
        'chromium_src',
        'vendor/brightray',
        'vendor/native_mate',
        # Include atom_natives.h.
        '<(SHARED_INTERMEDIATE_DIR)',
        # Include directories for uv and node.
        'vendor/node/src',
        'vendor/node/deps/http_parser',
        'vendor/node/deps/uv/include',
        # The `node.h` is using `#include"v8.h"`.
        'vendor/brightray/vendor/download/libchromiumcontent/src/v8/include',
        # The `node.h` is using `#include"ares.h"`.
        'vendor/node/deps/cares/include',
        # The `third_party/WebKit/Source/platform/weborigin/SchemeRegistry.h` is using `platform/PlatformExport.h`.
        'vendor/brightray/vendor/download/libchromiumcontent/src/third_party/WebKit/Source',
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
          'sources': [
            '<@(lib_sources_win)',
          ],
          'link_settings': {
            'libraries': [
              '-limm32.lib',
              '-loleacc.lib',
              '-lComdlg32.lib',
              '-lWininet.lib',
            ],
          },
          'dependencies': [
            # Node is built as static_library on Windows, so we also need to
            # include its dependencies here.
            'vendor/node/deps/cares/cares.gyp:cares',
            'vendor/node/deps/http_parser/http_parser.gyp:http_parser',
            'vendor/node/deps/uv/uv.gyp:libuv',
            'vendor/node/deps/zlib/zlib.gyp:zlib',
            # Build with breakpad support.
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
            '<!@(pkg-config --cflags dbus-1)',
            '-Wno-deprecated-register',
            '-Wno-empty-body',
            '-Wno-reserved-user-defined-literal',
          ],
          'dependencies': [
            'vendor/breakpad/breakpad.gyp:breakpad_client',
          ],
        }],  # OS=="linux"
      ],
    },  # target <(product_name)_lib
    {
      'target_name': 'compile_coffee',
      'type': 'none',
      'actions': [
        {
          'action_name': 'compile_coffee',
          'variables': {
            'conditions': [
              ['OS=="mac"', {
                'resources_path': '<(PRODUCT_DIR)/<(product_name).app/Contents/Resources',
              },{
                'resources_path': '<(PRODUCT_DIR)/resources',
              }],
            ],
          },
          'inputs': [
            '<@(coffee_sources)',
          ],
          'outputs': [
            '<(resources_path)/atom.asar',
          ],
          'action': [
            'python',
            'tools/coffee2asar.py',
            '<@(_outputs)',
            '<@(_inputs)',
          ],
        }
      ],
    },  # target compile_coffee
    {
      'target_name': 'atom_coffee2c',
      'type': 'none',
      'actions': [
        {
          'action_name': 'atom_coffee2c',
          'inputs': [
            '<@(coffee2c_sources)',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/atom_natives.h',
          ],
          'action': [
            'python',
            'tools/coffee2c.py',
            '<@(_outputs)',
            '<@(_inputs)',
          ],
        }
      ],
    },  # target atom_coffee2c
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
                '--libchromiumcontent-dir=<(libchromiumcontent_dir)',
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
                '<(libchromiumcontent_dir)',
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
                '--libchromiumcontent-dir=<(libchromiumcontent_dir)',
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
              'action': [
                'tools/posix/strip.sh',
                '<@(_inputs)',
              ],
            },
          ],
        }],  # OS=="linux"
      ],
    },  # target <(project_name>_dump_symbols
    {
      'target_name': 'copy_chromedriver',
      'type': 'none',
      'actions': [
        {
          'action_name': 'Copy ChromeDriver Binary',
          'variables': {
            'conditions': [
              ['OS=="win"', {
                'chromedriver_binary': 'chromedriver.exe',
              },{
                'chromedriver_binary': 'chromedriver',
              }],
            ],
          },
          'inputs': [
            '<(libchromiumcontent_dir)/<(chromedriver_binary)',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/<(chromedriver_binary)',
          ],
          'action': [
            'python',
            'tools/copy_binary.py',
            '<@(_inputs)',
            '<@(_outputs)',
          ],
        }
      ],
    },  # copy_chromedriver
  ],
  'conditions': [
    ['OS=="mac"', {
      'targets': [
        {
          'target_name': '<(project_name)_framework',
          'product_name': '<(product_name) Framework',
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
            '<(libchromiumcontent_src_dir)',
          ],
          'defines': [
            'PRODUCT_NAME="<(product_name)"',
          ],
          'export_dependent_settings': [
            '<(project_name)_lib',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
              '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
              'external_binaries/Squirrel.framework',
              'external_binaries/ReactiveCocoa.framework',
              'external_binaries/Mantle.framework',
            ],
          },
          'mac_bundle': 1,
          'mac_bundle_resources': [
            'atom/common/resources/mac/MainMenu.xib',
            '<(libchromiumcontent_dir)/content_shell.pak',
            '<(libchromiumcontent_dir)/icudtl.dat',
            '<(libchromiumcontent_dir)/natives_blob.bin',
            '<(libchromiumcontent_dir)/snapshot_blob.bin',
          ],
          'xcode_settings': {
            'INFOPLIST_FILE': 'atom/common/resources/mac/Info.plist',
            'LD_DYLIB_INSTALL_NAME': '@rpath/<(product_name) Framework.framework/<(product_name) Framework',
            'LD_RUNPATH_SEARCH_PATHS': [
              '@loader_path/Libraries',
            ],
            'OTHER_LDFLAGS': [
              '-ObjC',
            ],
          },
          'copies': [
            {
              'variables': {
                'conditions': [
                  ['libchromiumcontent_component', {
                    'copied_libraries': '<(libchromiumcontent_shared_libraries)',
                  }, {
                    'copied_libraries': [],
                  }],
                ],
              },
              'destination': '<(PRODUCT_DIR)/<(product_name) Framework.framework/Versions/A/Libraries',
              'files': [
                '<@(copied_libraries)',
                '<(libchromiumcontent_dir)/ffmpegsumo.so',
              ],
            },
            {
              'destination': '<(PRODUCT_DIR)/<(product_name) Framework.framework/Versions/A/Resources',
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
                '<(product_name) Framework',
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
                'tools/make_locale_paks.py',
              ],
              'outputs': [
                '<(PRODUCT_DIR)/locales'
              ],
              'action': [
                'python',
                'tools/make_locale_paks.py',
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
                '<(PRODUCT_DIR)/<(project_name).lib',
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
