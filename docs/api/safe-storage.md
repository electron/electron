# SafeStorage

## Class: SafeStorage

> blah.

Process: [Main](../glossary.md#main-process)

### Methods

The `Menu` class has the following static methods:

#### `SafeStorage.isEncryptionAvailable()`

On Linux returns true iff the real secret key (not hardcoded one) is
available. On MacOS returns true if Keychain is available (for mock
Keychain it returns true if not using locked Keychain, false if using
locked mock Keychain). On Windows returns true if non mock encryption
key is available

Returns `Boolean` - describe.

#### `SafeStorage.encryptString(plainText, cipherText)`

* `plainText` String
* `cipherText` String

Returns `Boolean` -  Encrypt a string16. The output (second argument) is really an array of bytes, but we're passing it back as a std::string.

#### `SafeStorage.decryptString(cipherText, plainText)`

* `cipherText` String
* `plainText` String

Decrypt an array of bytes obtained with EnctryptString back into a string.
Note that the input (first argument) is a std::string, so you need to first
get your (binary) data into a string.
