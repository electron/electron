vars = {
  'chromium_version':
    '66.0.3359.181',
  'libchromiumcontent_revision':
    'a664c7088a87575e7567c7b0a814d4bfa74bb118',
  'node_version':
    '6d1bdd3ebf860ff878a1b5db4fd26fdda3932e21',

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
