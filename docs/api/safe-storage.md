# safeStorage

> Allows access to simple encryption and decryption of strings for storage on the local machine.

Process: [Main](../glossary.md#main-process)

This module protects data stored on disk from being accessed by other applications or users with full disk access.

Note that on Mac, access to the system Keychain is required and
these calls can block the current thread to collect user input.
The same is true for Linux, if a password management tool is available.

## Methods

The `safeStorage` module has the following methods:

### `safeStorage.isEncryptionAvailable()`

Returns `boolean` - Whether encryption is available.

On Linux, returns true if the app has emitted the `ready` event and the secret key is available.
On MacOS, returns true if Keychain is available.
On Windows, returns true once the app has emitted the `ready` event.

### `safeStorage.encryptString(plainText)`

* `plainText` string

Returns `Buffer` -  An array of bytes representing the encrypted string.

This function will throw an error if encryption fails.

### `safeStorage.decryptString(encrypted)`

* `encrypted` Buffer

Returns `string` - the decrypted string. Decrypts the encrypted buffer
obtained  with `safeStorage.encryptString` back into a string.

This function will throw an error if decryption fails.

### `safeStorage.setUsePlainTextEncryption(usePlainText)`

* `usePlainText` boolean

This function on Linux will force the module to use an in memory password for creating
symmetric key that is used for encrypt/decrypt functions when a valid OS password
manager cannot be determined for the current active desktop environment. This function
is a no-op on Windows and MacOS.

### `safeStorage.getSelectedStorageBackend()` _Linux_

Returns `string` - User friendly name of the password manager selected on Linux.

This function will return one of the following values:

* `basic_text` - When the desktop environment is not recognised or if the following
command line flag is provided `--password-store="basic"`.
* `gnome_libsecret` - When the desktop environment is `X-Cinnamon`, `Deepin`, `GNOME`, `Pantheon`, `XFCE`, `UKUI`, `unity` or if the following command line flag is provided `--password-store="gnome-libsecret"`.
* `kwallet` - When the desktop session is `kde4` or if the following command line flag
is provided `--password-store="kwallet"`.
* `kwallet5` - When the desktop session is `kde5` or if the following command line flag
is provided `--password-store="kwallet5"`.
* `kwallet6` - When the desktop session is `kde6`.
* `unknown` - When the function is called before app has emitted the `ready` event.
