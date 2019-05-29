vars = {
  'chromium_version':
    '63.0.3239.150',
  'libchromiumcontent_revision':
    '55625e1de5d34a815921975f4d5556133b8142af',
  'node_version':
    'v9.7.0-33-g538a5023af',
  'native_mate_revision':
    '4cd7d113915de0cc08e9a218be35bff9c7361906',

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
