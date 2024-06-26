# safeStorage

> Allows access to simple encryption and decryption of strings for storage on the local machine.

Process: [Main](../glossary.md#main-process)

This module adds extra protection to data being stored on disk by using OS-provided cryptography systems. Current
security semantics for each platform are outlined below.

* **macOS**: Encryption keys are stored for your app in [Keychain Access](https://support.apple.com/en-ca/guide/keychain-access/kyca1083/mac) in a way that prevents
other applications from loading them without user override. Therefore, content is protected from other users and other apps running in the same userspace.
* **Windows**: Encryption keys are generated via [DPAPI](https://learn.microsoft.com/en-us/windows/win32/api/dpapi/nf-dpapi-cryptprotectdata).
As per the Windows documentation: "Typically, only a user with the same logon credential as the user who encrypted the data can typically
decrypt the data". Therefore, content is protected from other users on the same machine, but not from other apps running in the
same userspace.
* **Linux**: Encryption keys are generated and stored in a secret store that varies depending on your window manager and system setup. Options currently supported are `kwallet`, `kwallet5`, `kwallet6` and `gnome-libsecret`, but more may be available in future versions of Electron. As such, the
security semantics of content protected via the `safeStorage` API vary between window managers and secret stores.
  * Note that not all Linux setups have an available secret store. If no secret store is available, items stored in using the `safeStorage` API will be unprotected
as they are encrypted via hardcoded plaintext password. You can detect when this happens when `safeStorage.getSelectedStorageBackend()` returns `basic_text`.

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
