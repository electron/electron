{
  'variables': {
    # Node disables the inspector unless icu is enabled. But node doesn't know
    # that we're building v8 with icu, so force it on.
    'v8_enable_inspector': 1,

    # By default, node will build a dylib called something like
    # libnode.$node_module_version.dylib, which is inconvenient for our
    # purposes (since it makes the library's name unpredictable). This forces
    # it to drop the module_version from the filename and just produce
    # `libnode.dylib`.
    'shlib_suffix': 'dylib',
  },
  'target_defaults': {
    'target_conditions': [
      ['_target_name=="node_lib"', {
        'include_dirs': [
          '../../../v8',
          '../../../v8/include',
          '../../../third_party/icu/source/common',
          '../../../third_party/icu/source/i18n',
        ],
        'libraries': [
          '../../../../../../libv8.dylib',
          '../../../../../../libv8_libbase.dylib',
          '../../../../../../libv8_libplatform.dylib',
          '../../../../../../libicuuc.dylib',
        ],
        'defines': [
          # These will no longer be necessary once
          # https://github.com/google/boringssl/commit/a02ed04d527e1b57b4efaa0b4f9bdbc1ed5975b2
          # is in the past for Electron
          'EVP_CIPH_CCM_MODE=0',
          'EVP_CIPH_WRAP_MODE=0',
          'EVP_CIPHER_CTX_FLAG_WRAP_ALLOW=0',
          'EVP_CIPHER_CTX_set_flags(...)',
        ],
      }],
    ],
  },
}
