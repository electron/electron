{
  'variables': {
    'project_name%': 'brave',
    'product_name%': 'Brave',
    'company_name%': 'Brave Software',
    'company_abbr%': 'brave',
    'version%': '1.3.12',
  },
  'includes': [
    'filenames.gypi',
    'vendor/native_mate/native_mate_files.gypi',
    'extensions.gypi',
    'autofill.gypi',
  ],
  'target_defaults': {
    'msvs_disabled_warnings': [
      4456,
    ],
    'defines': [
      'ATOM_PRODUCT_NAME="<(product_name)"',
      'ATOM_PROJECT_NAME="<(project_name)"',
    ],
    'conditions': [
      ['OS=="mac"', {
        'mac_framework_dirs': [
          '<(source_root)/external_binaries',
        ],
      }],
    ],
  },
  'targets': [
    {
      'target_name': '<(project_name)',
      'type': 'executable',
      'dependencies': [
        'js2asar',
        'app2asar',
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
            'ATOM_BUNDLE_ID': 'com.<(company_abbr).<(project_name)',
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
              # that Electron supports those languages.
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
          ],
          'conditions': [
            ['mas_build==0', {
              'copies': [
                {
                  'destination': '<(PRODUCT_DIR)/<(product_name).app/Contents/Frameworks',
                  'files': [
                    'external_binaries/Squirrel.framework',
                    'external_binaries/ReactiveCocoa.framework',
                    'external_binaries/Mantle.framework',
                  ],
                },
              ],
            }],
          ],
        }],  # OS!="mac"
        ['OS=="win"', {
          'include_dirs': [
            '<(libchromiumcontent_dir)/gen/ui/resources',
          ],
          'msvs_settings': {
            'VCManifestTool': {
              'EmbedManifest': 'true',
              'AdditionalManifestFiles': 'atom/browser/resources/win/atom.manifest',
            }
          },
          'copies': [
            {
              'variables': {
                'conditions': [
                  ['libchromiumcontent_component', {
                    'copied_libraries': [
                      '<@(libchromiumcontent_shared_libraries)',
                      '<@(libchromiumcontent_shared_v8_libraries)',
                    ],
                  }, {
                    'copied_libraries': [
                      '<(libchromiumcontent_dir)/ffmpeg.dll',
                    ],
                  }],
                ],
              },
              'destination': '<(PRODUCT_DIR)',
              'files': [
                '<@(copied_libraries)',
                '<(libchromiumcontent_dir)/locales',
                '<(libchromiumcontent_dir)/libEGL.dll',
                '<(libchromiumcontent_dir)/libGLESv2.dll',
                '<(libchromiumcontent_dir)/icudtl.dat',
                '<(libchromiumcontent_dir)/blink_image_resources_200_percent.pak',
                '<(libchromiumcontent_dir)/content_resources_200_percent.pak',
                '<(libchromiumcontent_dir)/content_shell.pak',
                '<(libchromiumcontent_dir)/ui_resources_200_percent.pak',
                '<(libchromiumcontent_dir)/views_resources_200_percent.pak',
                '<(libchromiumcontent_dir)/natives_blob.bin',
                '<(libchromiumcontent_dir)/snapshot_blob.bin',
                'external_binaries/d3dcompiler_47.dll',
                'external_binaries/xinput1_3.dll',
              ],
            },
          ],
        }, {
          'dependencies': [
            'vendor/breakpad/breakpad.gyp:dump_syms#host',
          ],
        }],  # OS=="win"
        ['OS=="linux"', {
          'copies': [
            {
              'variables': {
                'conditions': [
                  ['libchromiumcontent_component', {
                    'copied_libraries': [
                      '<(PRODUCT_DIR)/lib/libnode.so',
                      '<@(libchromiumcontent_shared_libraries)',
                      '<@(libchromiumcontent_shared_v8_libraries)',
                    ],
                  }, {
                    'copied_libraries': [
                      '<(PRODUCT_DIR)/lib/libnode.so',
                      '<(libchromiumcontent_dir)/libffmpeg.so',
                    ],
                  }],
                ],
              },
              'destination': '<(PRODUCT_DIR)',
              'files': [
                '<@(copied_libraries)',
                '<(libchromiumcontent_dir)/locales',
                '<(libchromiumcontent_dir)/icudtl.dat',
                '<(libchromiumcontent_dir)/blink_image_resources_200_percent.pak',
                '<(libchromiumcontent_dir)/content_resources_200_percent.pak',
                '<(libchromiumcontent_dir)/content_shell.pak',
                '<(libchromiumcontent_dir)/ui_resources_200_percent.pak',
                '<(libchromiumcontent_dir)/views_resources_200_percent.pak',
                '<(libchromiumcontent_dir)/natives_blob.bin',
                '<(libchromiumcontent_dir)/snapshot_blob.bin',
              ],
            },
          ],
        }],  # OS=="linux"
      ],
    },  # target <(project_name)
    {
      'target_name': '<(project_name)_lib',
      'type': 'static_library',
      'dependencies': [
        'atom_js2c',
        'vendor/brightray/brightray.gyp:brightray',
        'vendor/node/node.gyp:node',
      ],
      'defines': [
        # We need to access internal implementations of Node.
        'NODE_WANT_INTERNALS=1',
        'NODE_SHARED_MODE',
        # This is defined in skia/skia_common.gypi.
        'SK_SUPPORT_LEGACY_GETTOPDEVICE',
        # Disable warnings for g_settings_list_schemas.
        'GLIB_DISABLE_DEPRECATION_WARNINGS',
        # Defined in Chromium but not exposed in its gyp file.
        'V8_USE_EXTERNAL_STARTUP_DATA',
        'ENABLE_PLUGINS',
        'ENABLE_PEPPER_CDMS',
        'USE_PROPRIETARY_CODECS',
        '__STDC_FORMAT_MACROS',
      ],
      'sources': [
        '<@(lib_sources)',
        '<@(autofill_sources)',
      ],
      'include_dirs': [
        '.',
        'chromium_src',
        # autofill - TODO(bridiver) - move to autofill.gypi
        '<(libchromiumcontent_dir)/gen/protoc_out',
        '<(libchromiumcontent_src_dir)/third_party/protobuf/src',
        # end autofill
        'vendor/brightray',
        'vendor/native_mate',
        # Include atom_natives.h.
        '<(SHARED_INTERMEDIATE_DIR)',
        # Include directories for uv and node.
        'vendor/node/src',
        'vendor/node/deps/http_parser',
        'vendor/node/deps/uv/include',
        # The `node.h` is using `#include"v8.h"`.
        '<(libchromiumcontent_src_dir)/v8/include',
        # The `node.h` is using `#include"ares.h"`.
        'vendor/node/deps/cares/include',
        # The `third_party/WebKit/Source/platform/weborigin/SchemeRegistry.h` is using `platform/PlatformExport.h`.
        '<(libchromiumcontent_src_dir)/third_party/WebKit/Source',
        # The 'third_party/libyuv/include/libyuv/scale_argb.h' is using 'libyuv/basic_types.h'.
        '<(libchromiumcontent_src_dir)/third_party/libyuv/include',
        # The 'third_party/webrtc/modules/desktop_capture/desktop_frame.h' is using 'webrtc/base/scoped_ptr.h'.
        '<(libchromiumcontent_src_dir)/third_party/',
        '<(libchromiumcontent_src_dir)/components/cdm',
        '<(libchromiumcontent_src_dir)/third_party/widevine',
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
        ['enable_extensions==1', {
          'dependencies': [ 'extensions.gyp:atom_resources' ],
          'sources': [ '<@(extension_sources)' ],
          'include_dirs': [
            '<(libchromiumcontent_dir)/gen/extensions',
          ],
          'conditions': [
            ['OS=="linux"', {
              'dependencies': [
                'extensions.gyp:xscrnsaver',
              ],
            }],
          ]
        }],
        ['OS!="linux" and libchromiumcontent_component', {
          'link_settings': {
            # Following libraries are always linked statically.
            'libraries': [ '<@(autofill_libraries)' ],
          },
        }],
        ['OS=="linux" and libchromiumcontent_component', {
          'link_settings': {
            'libraries': [
              # hack to handle cyclic dependencies
              '-Wl,--start-group',
              '<@(autofill_libraries)',
              '-Wl,--end-group',
            ],
          }
        }],
        ['OS!="linux" and enable_extensions==1 and libchromiumcontent_component', {
          'link_settings': {
            # Following libraries are always linked statically.
            'libraries': [ '<@(extension_libraries)' ],
          },
        }],
        ['OS=="linux" and enable_extensions==1 and libchromiumcontent_component', {
          'link_settings': {
            'libraries': [
              # hack to handle cyclic dependencies
              '-Wl,--start-group',
              '<@(extension_libraries)',
              '-Wl,--end-group',
            ],
          }
        }],
        ['libchromiumcontent_component', {
          'link_settings': {
            'libraries': [ '<@(libchromiumcontent_v8_libraries)' ],
          },
        }],
        ['OS=="win"', {
          'sources': [
            '<@(lib_sources_win)',
          ],
          'link_settings': {
            'libraries': [
              '-limm32.lib',
              '-loleacc.lib',
              '-lcomctl32.lib',
              '-lcomdlg32.lib',
              '-lwininet.lib',
              '-lwinmm.lib',
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
        ['OS=="mac" and mas_build==0', {
          'dependencies': [
            'vendor/crashpad/client/client.gyp:crashpad_client',
            'vendor/crashpad/handler/handler.gyp:crashpad_handler',
          ],
          'link_settings': {
            # Do not link with QTKit for mas build.
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/QTKit.framework',
            ],
          },
          'xcode_settings': {
            # ReactiveCocoa which is used by Squirrel requires using __weak.
            'CLANG_ENABLE_OBJC_WEAK': 'YES',
          },
        }],  # OS=="mac" and mas_build==0
        ['OS=="mac" and mas_build==1', {
          'defines': [
            'MAS_BUILD',
          ],
          'sources!': [
            'atom/browser/auto_updater_mac.mm',
            'atom/common/crash_reporter/crash_reporter_mac.h',
            'atom/common/crash_reporter/crash_reporter_mac.mm',
          ],
        }],  # OS=="mac" and mas_build==1
        ['OS=="linux"', {
          'sources': [
            '<@(lib_sources_nss)',
          ],
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
          'cflags_cc': [
            '-Wno-empty-body',
            '-Wno-reserved-user-defined-literal',
          ],
          'include_dirs': [
            'vendor/breakpad/src',
          ],
          'dependencies': [
            'vendor/breakpad/breakpad.gyp:breakpad_client',
          ],
        }],  # OS=="linux"
      ],
    },  # target <(product_name)_lib
    {
      'target_name': 'js2asar',
      'type': 'none',
      'actions': [
        {
          'action_name': 'js2asar',
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
            '<@(js_sources)',
            '<@(extension_js_sources)'
          ],
          'outputs': [
            '<(resources_path)/electron.asar',
          ],
          'action': [
            'python',
            'tools/js2asar.py',
            '<@(_outputs)',
            'lib',
            '<@(_inputs)',
          ],
        }
      ],
    },  # target js2asar
    {
      'target_name': 'app2asar',
      'type': 'none',
      'actions': [
        {
          'action_name': 'app2asar',
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
            '<@(default_app_sources)',
          ],
          'outputs': [
            '<(resources_path)/default_app.asar',
          ],
          'action': [
            'python',
            'tools/js2asar.py',
            '<@(_outputs)',
            'default_app',
            '<@(_inputs)',
          ],
        }
      ],
    },  # target app2asar
    {
      'target_name': 'atom_js2c',
      'type': 'none',
      'actions': [
        {
          'action_name': 'atom_js2c',
          'inputs': [
            '<@(js2c_sources)',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/atom_natives.h',
          ],
          'action': [
            'python',
            'tools/js2c.py',
            '<@(_outputs)',
            '<@(_inputs)',
          ],
        }
      ],
    },  # target atom_js2c
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
            'extensions.gyp:atom_resources',
          ],
          'sources': [
            '<@(framework_sources)',
          ],
          'include_dirs': [
            '.',
            'vendor',
            '<(libchromiumcontent_src_dir)',
          ],
          'export_dependent_settings': [
            '<(project_name)_lib',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
              '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
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
            'ATOM_BUNDLE_ID': 'com.<(company_abbr).<(project_name).framework',
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
                    'copied_libraries': [
                      '<(PRODUCT_DIR)/libnode.dylib',
                      '<@(libchromiumcontent_shared_libraries)',
                      '<@(libchromiumcontent_shared_v8_libraries)',
                    ],
                  }, {
                    'copied_libraries': [
                      '<(PRODUCT_DIR)/libnode.dylib',
                      '<(libchromiumcontent_dir)/libffmpeg.dylib',
                    ],
                  }],
                ],
              },
              'destination': '<(PRODUCT_DIR)/<(product_name) Framework.framework/Versions/A/Libraries',
              'files': [
                '<@(copied_libraries)',
              ],
            },
          ],
          'postbuilds': [
            {
              'postbuild_name': 'Fix path of libnode',
              'action': [
                'install_name_tool',
                '-change',
                '/usr/local/lib/libnode.dylib',
                '@rpath/libnode.dylib',
                '${BUILT_PRODUCTS_DIR}/<(product_name) Framework.framework/Versions/A/<(product_name) Framework',
              ],
            },
            {
              'postbuild_name': 'Fix path of ffmpeg',
              'action': [
                'install_name_tool',
                '-change',
                '/usr/local/lib/libffmpeg.dylib',
                '@rpath/libffmpeg.dylib',
                '${BUILT_PRODUCTS_DIR}/<(product_name) Framework.framework/Versions/A/<(product_name) Framework',
              ],
            },
            {
              'postbuild_name': 'Add symlinks for framework subdirectories',
              'action': [
                'tools/mac/create-framework-subdir-symlinks.sh',
                '<(product_name) Framework',
                'Libraries',
              ],
            },
            {
              'postbuild_name': 'Copy locales',
              'action': [
                'tools/mac/copy-locales.py',
                '-d',
                '<(libchromiumcontent_dir)/locales',
                '${BUILT_PRODUCTS_DIR}/<(product_name) Framework.framework/Resources',
                '<@(locales)',
              ],
            },
          ],
          'conditions': [
            ['mas_build==0', {
              'link_settings': {
                'libraries': [
                  'external_binaries/Squirrel.framework',
                  'external_binaries/ReactiveCocoa.framework',
                  'external_binaries/Mantle.framework',
                ],
              },
              'copies': [
                {
                  'destination': '<(PRODUCT_DIR)/<(product_name) Framework.framework/Versions/A/Resources',
                  'files': [
                    '<(PRODUCT_DIR)/crashpad_handler',
                  ],
                },
              ],
            }],
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
            'ATOM_BUNDLE_ID': 'com.<(company_abbr).<(project_name).helper',
            'INFOPLIST_FILE': 'atom/renderer/resources/mac/Info.plist',
            'LD_RUNPATH_SEARCH_PATHS': [
              '@executable_path/../../..',
            ],
          },
        },  # target helper
      ],
    }],  # OS!="mac"
  ],
}
