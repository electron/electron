# safeStorage

> Manage cookie encryption.

Process: [Main](../glossary.md#main-process)

`safeStorage` is an [EventEmitter][event-emitter].

## Methods

The `safeStorage` module has the following methods:

### `safeStorage.isEncryptionAvailable()`

On Linux returns true iff the real secret key (not hardcoded one) is
available. On MacOS returns true if Keychain is available (for mock
Keychain it returns true if not using locked Keychain, false if using
locked mock Keychain). On Windows returns true if non mock encryption
key is available

Returns `Boolean` - Whether cookie encryption is available.

### `safeStorage.encryptString(plainText)`

* `plainText` String

Encrypt a string.

Returns `Buffer` -  An array of bytes representing the encrypted string.

### `safeStorage.decryptString(cipherText)`

* `cipherText` Buffer

Decrypt buffer obtained with `safeStorage.encryptString` back into a string.

Returns `String` - the decrypted string.
