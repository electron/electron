vars = {
  'chromium_version':
    '67.0.3396.99',
  'libchromiumcontent_revision':
    '1fff3a0b3ebbbb8ded55333a7381300a1614b9b3',
  'node_version':
    '66d85a6e4cdcbb1f3322ee386d01d0354e174a94',

  'chromium_git':
    'https://chromium.googlesource.com',

  'electron_git':
    'https://github.com/electron',

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
  'src/libchromiumcontent':
    (Var("electron_git")) + '/libchromiumcontent.git@' + (Var("libchromiumcontent_revision")),
  'src/third_party/electron_node':
    (Var("electron_git")) + '/node.git@' + (Var("node_version")),
}

hooks = [
  {
    'action': [
      'python',
      'src/libchromiumcontent/script/apply-patches',
      '--project-root=.',
      '--commit'
    ],
    'pattern':
      'src/libchromiumcontent',
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
