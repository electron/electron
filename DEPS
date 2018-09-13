vars = {
  'chromium_version':
    '68.0.3440.128',
  'node_version':
    '9dcbed23f016d3ad081be6ec7fb5122e57862da7',

  'pyyaml_version':
    '3.12',

  'chromium_git':
    'https://chromium.googlesource.com',

  'electron_git':
    'https://github.com/electron',

  'yaml_git':
    'https://github.com/yaml',

  'checkout_nacl':
    False,
  'checkout_libaom':
    True,
  'checkout_oculus_sdk':
    False,
}

deps = {
  'src':
    (Var("chromium_git")) + '/chromium/src.git@' + (Var("chromium_version")),
  'src/third_party/electron_node':
    (Var("electron_git")) + '/node.git@' + (Var("node_version")),
  'src/electron/vendor/pyyaml':
    (Var("yaml_git")) + '/pyyaml.git@' + (Var("pyyaml_version")),
}

hooks = [
  {
    'action': [
      'python',
      'src/electron/script/apply-patches',
      '--project-root=.',
      '--commit'
    ],
    'pattern':
      'src/electron',
    'name':
      'patch_chromium'
  },
  {
    'action': [
      'python',
      'src/electron/script/update-external-binaries.py'
    ],
    'pattern':
      'src/electron/script/update-external-binaries.py',
    'name':
      'electron_external_binaries'
  },
  {
    'action': [
      'python',
      '-c',
      'import os; os.chdir("src"); os.chdir("electron"); os.system("npm install")',
    ],
    'pattern': 'src/electron/package.json',
    'name': 'electron_npm_deps'
  },
]

recursedeps = [
  'src',
  'src/libchromiumcontent',
]

gclient_gn_args = [
  'checkout_libaom',
  'checkout_nacl',
  'checkout_oculus_sdk'
]
gclient_gn_args_file =  'src/build/config/gclient_args.gni'
