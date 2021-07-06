# safeStorage

> Manage cookie encryption.

Process: [Main](../glossary.md#main-process)

## Methods

The `safeStorage` module has the following methods:

### `safeStorage.isEncryptionAvailable()`

On Linux returns true iff the real secret key (not hardcoded one) is
available. On MacOS returns true if Keychain is available. On Windows returns true.

Returns `Boolean` - Whether encryption is available.

### `safeStorage.encryptString(plainText)`

* `plainText` String

Encrypt a string.

Returns `Buffer` -  An array of bytes representing the encrypted string.

### `safeStorage.decryptString(cipherText)`

* `cipherText` Buffer

Decrypt buffer obtained with `safeStorage.encryptString` back into a string.

Returns `String` - the decrypted string.
