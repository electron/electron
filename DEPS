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
  'generate_location_tags',
]

vars = {
  'chromium_version':
    '96.0.4664.45',
  'node_version':
    'v16.13.0',
  'nan_version':
    # The following commit hash of NAN is v2.14.2 with *only* changes to the
    # test suite. This should be updated to a specific tag when one becomes
    # available.
    '65b32af46e9d7fab2e4ff657751205b3865f4920',
  'squirrel.mac_version':
    '0e5d146ba13101a1302d59ea6e6e0b3cace4ae38',

  'pyyaml_version': '3.12',

  'chromium_git': 'https://chromium.googlesource.com',
  'electron_git': 'https://github.com/electron',
  'nodejs_git': 'https://github.com/nodejs',
  'yaml_git': 'https://github.com/yaml',
  'squirrel_git': 'https://github.com/Squirrel',

  # KEEP IN SYNC WITH utils.js FILE
  'yarn_version': '1.15.2',

  # To be able to build clean Chromium from sources.
  'apply_patches': True,

  # To use an mtime cache for patched files to speed up builds.
  'use_mtime_cache': True,

  # To allow in-house builds to checkout those manually.
  'checkout_chromium': True,
  'checkout_node': True,
  'checkout_nan': True,
  'checkout_pgo_profiles': True,

  # It's only needed to parse the native tests configurations.
  'checkout_pyyaml': False,

  'use_rts': False,

  'mac_xcode_version': 'default',

  'generate_location_tags': False,

  # To allow running hooks without parsing the DEPS tree
  'process_deps': True,

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
  'checkout_clang_tidy':
    True,
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
  'src/third_party/pyyaml': {
    'url': (Var("yaml_git")) + '/pyyaml.git@' + (Var("pyyaml_version")),
    'condition': 'checkout_pyyaml and process_deps',
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

pre_deps_hooks = [
  {
    'name': 'generate_mtime_cache',
    'condition': '(checkout_chromium and apply_patches and use_mtime_cache) and process_deps',
    'pattern': 'src/electron',
    'action': [
      'python3',
      'src/electron/script/patches-mtime-cache.py',
      'generate',
      '--cache-file',
      'src/electron/patches/mtime-cache.json',
      '--patches-config',
      'src/electron/patches/config.json',
    ],
  },
]

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
    'name': 'apply_mtime_cache',
    'condition': '(checkout_chromium and apply_patches and use_mtime_cache) and process_deps',
    'pattern': 'src/electron',
    'action': [
      'python3',
      'src/electron/script/patches-mtime-cache.py',
      'apply',
      '--cache-file',
      'src/electron/patches/mtime-cache.json',
    ],
  },
  {
    'name': 'electron_npm_deps',
    'pattern': 'src/electron/package.json',
    'action': [
      'python3',
      '-c',
      'import os, subprocess; os.chdir(os.path.join("src", "electron")); subprocess.check_call(["python3", "script/lib/npx.py", "yarn@' + (Var("yarn_version")) + '", "install", "--frozen-lockfile"]);',
    ],
  },
]

recursedeps = [
  'src',
  'src/third_party/squirrel.mac',
]
