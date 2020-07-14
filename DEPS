gclient_gn_args_file = 'src/build/config/gclient_args.gni'
gclient_gn_args = [
  'build_with_chromium',
  'checkout_android',
  'checkout_android_native_support',
  'checkout_libaom',
  'checkout_nacl',
  'checkout_pgo_profiles',
  'checkout_oculus_sdk',
  'checkout_openxr',
  'checkout_google_benchmark',
  'mac_xcode_version',
]

vars = {
  'chromium_version':
    '0ee01724797ab0f6deadb33bb3a0201cd2d21602',
  'node_version':
    'v12.18.2',
  'nan_version':
    '2c4ee8a32a299eada3cd6e468bbd0a473bfea96d',
  'squirrel.mac_version':
    '44468f858ce0d25c27bd5e674abfa104e0119738',

  'boto_version': 'f7574aa6cc2c819430c1f05e9a1a1a666ef8169b',
  'pyyaml_version': '3.12',
  'requests_version': 'e4d59bedfd3c7f4f254f4f5d036587bcd8152458',

  'boto_git': 'https://github.com/boto',
  'chromium_git': 'https://chromium.googlesource.com',
  'electron_git': 'https://github.com/electron',
  'nodejs_git': 'https://github.com/nodejs',
  'requests_git': 'https://github.com/kennethreitz',
  'yaml_git': 'https://github.com/yaml',
  'squirrel_git': 'https://github.com/Squirrel',

  # KEEP IN SYNC WITH utils.js FILE
  'yarn_version': '1.15.2',

  # To be able to build clean Chromium from sources.
  'apply_patches': True,

  # Python interface to Amazon Web Services. Is used for releases only.
  'checkout_boto': False,

  # To allow in-house builds to checkout those manually.
  'checkout_chromium': True,
  'checkout_node': True,
  'checkout_nan': True,
  'checkout_pgo_profiles': True,

  # It's only needed to parse the native tests configurations.
  'checkout_pyyaml': False,

  # Python "requests" module is used for releases only.
  'checkout_requests': False,

  'mac_xcode_version': 'default',

  # To allow running hooks without parsing the DEPS tree
  'process_deps': True,

  # It is always needed for normal Electron builds,
  # but might be impossible for custom in-house builds.
  'download_external_binaries': True,

  'checkout_nacl':
    False,
  'checkout_libaom':
    True,
  'checkout_oculus_sdk':
    False,
  'checkout_openxr':
    False,
  'build_with_chromium':
    True,
  'checkout_android':
    False,
  'checkout_android_native_support':
    False,
  'checkout_google_benchmark':
    False,
}

deps = {
  'src': {
    'url': (Var("chromium_git")) + '/chromium/src.git@' + (Var("chromium_version")),
    'condition': 'checkout_chromium and process_deps',
  },
  'src/third_party/nan': {
    'url': (Var("nodejs_git")) + '/nan.git@' + (Var("nan_version")),
    'condition': 'checkout_nan and process_deps',
  },
  'src/third_party/electron_node': {
    'url': (Var("nodejs_git")) + '/node.git@' + (Var("node_version")),
    'condition': 'checkout_node and process_deps',
  },
  'src/electron/vendor/pyyaml': {
    'url': (Var("yaml_git")) + '/pyyaml.git@' + (Var("pyyaml_version")),
    'condition': 'checkout_pyyaml and process_deps',
  },
  'src/electron/vendor/boto': {
    'url': Var('boto_git') + '/boto.git' + '@' +  Var('boto_version'),
    'condition': 'checkout_boto and process_deps',
  },
  'src/electron/vendor/requests': {
    'url': Var('requests_git') + '/requests.git' + '@' +  Var('requests_version'),
    'condition': 'checkout_requests and process_deps',
  },
  'src/third_party/squirrel.mac': {
    'url': Var("squirrel_git") + '/Squirrel.Mac.git@' + Var("squirrel.mac_version"),
    'condition': 'process_deps',
  },
  'src/third_party/squirrel.mac/vendor/ReactiveObjC': {
    'url': 'https://github.com/ReactiveCocoa/ReactiveObjC.git@74ab5baccc6f7202c8ac69a8d1e152c29dc1ea76',
    'condition': 'process_deps'
  },
  'src/third_party/squirrel.mac/vendor/Mantle': {
    'url': 'https://github.com/Mantle/Mantle.git@78d3966b3c331292ea29ec38661b25df0a245948',
    'condition': 'process_deps',
  }
}

hooks = [
  {
    'name': 'patch_chromium',
    'condition': '(checkout_chromium and apply_patches) and process_deps',
    'pattern': 'src/electron',
    'action': [
      'python3',
      'src/electron/script/apply_all_patches.py',
      'src/electron/patches/config.json',
    ],
  },
  {
    'name': 'electron_external_binaries',
    'pattern': 'src/electron/script/update-external-binaries.py',
    'condition': 'download_external_binaries',
    'action': [
      'python3',
      'src/electron/script/update-external-binaries.py',
    ],
  },
  {
    'name': 'electron_npm_deps',
    'pattern': 'src/electron/package.json',
    'action': [
      'python3',
      '-c',
      'import os, subprocess; os.chdir(os.path.join("src", "electron")); subprocess.check_call(["python", "script/lib/npx.py", "yarn@' + (Var("yarn_version")) + '", "install", "--frozen-lockfile"]);',
    ],
  },
  {
    'name': 'setup_boto',
    'pattern': 'src/electron',
    'condition': 'checkout_boto and process_deps',
    'action': [
      'python3',
      '-c',
      'import os, subprocess; os.chdir(os.path.join("src", "electron", "vendor", "boto")); subprocess.check_call(["python", "setup.py", "build"]);',
    ],
  },
  {
    'name': 'setup_requests',
    'pattern': 'src/electron',
    'condition': 'checkout_requests and process_deps',
    'action': [
      'python3',
      '-c',
      'import os, subprocess; os.chdir(os.path.join("src", "electron", "vendor", "requests")); subprocess.check_call(["python", "setup.py", "build"]);',
    ],
  },
]

recursedeps = [
  'src',
  'src/third_party/squirrel.mac',
]
