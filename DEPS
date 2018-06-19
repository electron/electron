vars = {
  'chromium_version':
    '66.0.3359.181',
  'libchromiumcontent_revision':
    '2006b899d1927dd260ea0f170bdf01c2d5eda6a1',
  'node_version':
    'v10.2.0-31-gce2efb3516',
  'native_mate_revision':
    '875706f66008e03a0c7a699de16d7e2bde0efb90',

  'chromium_git':
    'https://chromium.googlesource.com',

  'electron_git':
    'https://github.com/electron',
}

deps = {
  'src':
    (Var("chromium_git")) + '/chromium/src.git@' + (Var("chromium_version")),
  'src/libchromiumcontent':
    (Var("electron_git")) + '/libchromiumcontent.git@' + (Var("libchromiumcontent_revision")),
  'src/third_party/electron_node':
    (Var("electron_git")) + '/node.git@' + (Var("node_version")),
  'src/third_party/native_mate':
    (Var("electron_git")) + '/native-mate.git@' + (Var("native_mate_revision")),
}

hooks = [
  {
    'action': [
      'src/libchromiumcontent/script/apply-patches'
    ],
    'pattern':
      'src/libchromiumcontent',
    'name':
      'patch_chromium'
  },
  {
    'action': [
      'src/electron/script/update-external-binaries.py'
    ],
    'pattern':
      'src/electron/script/update-external-binaries.py',
    'name':
      'electron_external_binaries'
  },
  {
    'action': [
      'bash',
      '-c',
      # NOTE(nornagon): this ridiculous {{}} stuff is because these strings get
      # variable-substituted twice by gclient.
      'echo -e "#\\n{{{{\'variables\':{{{{}}}}}}}}" > src/third_party/electron_node/config.gypi',
    ],
    'pattern': 'src/third_party/electron_node',
    'name': 'touch_node_config_gypi'
  },
  {
    'action': [
      'bash',
      '-c',
      'cd src/electron; npm install',
    ],
    'pattern': 'src/electron/package.json',
    'name': 'electron_npm_deps'
  },
]

recursedeps = [
  'src',
  'src/libchromiumcontent',
]
