# safeStorage

> Allows access to simple encryption and decryption of strings for storage on the local machine.

Process: [Main](../glossary.md#main-process)

This module adds extra protection to data being stored on disk by using OS-provided cryptography systems. Current
security semantics for each platform are outlined below.

> [!NOTE]
> We recommend using the asynchronous API (`encryptStringAsync`/`decryptStringAsync`) over the synchronous API.
> The async API is non-blocking, supports key rotation, and handles temporary unavailability gracefully.
> The synchronous API may be deprecated in a future version of Electron.

## Platform-Specific Key Providers

### Synchronous API

* **macOS**: Encryption keys are stored for your app in [Keychain Access](https://support.apple.com/en-ca/guide/keychain-access/kyca1083/mac) in a way that prevents
other applications from loading them without user override. Therefore, content is protected from other users and other apps running in the same userspace.
* **Windows**: Encryption keys are generated via [DPAPI](https://learn.microsoft.com/en-us/windows/win32/api/dpapi/nf-dpapi-cryptprotectdata). As per the Windows documentation: "Typically, only a user with the same logon credential as the user who encrypted the data can typically decrypt the data". Therefore, content is protected from other users on the same machine, but not from other apps running in the
same userspace.
* **Linux**: Encryption keys are generated and stored in a secret store that varies depending on your window manager and system setup. Options currently supported are `kwallet`, `kwallet5`, `kwallet6` and `gnome-libsecret`, but more may be available in future versions of Electron. As such, the
security semantics of content protected via the `safeStorage` API vary between window managers and secret stores.
  * Note that not all Linux setups have an available secret store. If no secret store is available, items stored in using the `safeStorage` API will be unprotected as they are encrypted via hardcoded plaintext password. You can detect when this happens when `safeStorage.getSelectedStorageBackend()` returns `basic_text`.

Note that on macOS, access to the system Keychain is required and
these calls can block the current thread to collect user input.
The same is true for Linux, if a password management tool is available.

### Asynchronous API

The asynchronous API uses pluggable key providers that vary by platform:

* **macOS**: Encryption keys are stored and retrieved from [Keychain Access](https://developer.apple.com/documentation/security/keychain-items). This provides the same security model as the synchronous API, protecting content from other users and other apps running in the same userspace.
* **Windows**: Encryption keys are protected via [DPAPI](https://learn.microsoft.com/en-us/windows/win32/api/dpapi). This provides the same security model as the synchronous API, protecting content from other users on the same machine but not from other apps running in the same userspace.
* **Linux**: Multiple key providers may be available depending on the desktop environment:
  * [`org.freedesktop.portal.Secret`](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Secret.html): Uses the Portal Secret D-Bus interface to retrieve application-specific secrets. This is the preferred provider for sandboxed environments like Flatpak.
  * [Secret Service API](https://specifications.freedesktop.org/secret-service/latest/): Uses the freedesktop.org Secret Service API (e.g., GNOME Keyring) for key storage.
  * A fallback provider is used for environments without a secret service available.

Unlike the synchronous API, these operations are non-blocking and support additional features like key rotation (indicated by `shouldReEncrypt`) and temporary unavailability handling (indicated by `isTemporarilyUnavailable`).

## Events

The `safeStorage` module emits the following events:

## Methods

The `safeStorage` module has the following methods:

### `safeStorage.isEncryptionAvailable()`

Returns `boolean` - Whether encryption is available.

On Linux, returns true if the app has emitted the `ready` event and the secret key is available.
On MacOS, returns true if Keychain is available.
On Windows, returns true once the app has emitted the `ready` event.

### `safeStorage.isAsyncEncryptionAvailable()`

Returns `Promise<Boolean>` - Whether encryption is available for asynchronous safeStorage operations.

### `safeStorage.encryptString(plainText)`

* `plainText` string

Returns `Buffer` -  An array of bytes representing the encrypted string.

This function will throw an error if encryption fails.

### `safeStorage.decryptString(encrypted)`

* `encrypted` Buffer

Returns `string` - the decrypted string. Decrypts the encrypted buffer
obtained  with `safeStorage.encryptString` back into a string.

### `safeStorage.encryptStringAsync(plainText)`

* `plainText` string

Returns `Promise<Buffer>` -  An array of bytes representing the encrypted string.

### `safeStorage.decryptStringAsync(encrypted)`

* `encrypted` Buffer

Returns `Promise<Object>` - Resolve with an object containing the following:

* `shouldReEncrypt` boolean - whether data that has just been returned from the decrypt operation should be
  re-encrypted, as the key has been rotated or a new  key is available that provides a different security level. If `true`, you should call `decryptStringAsync` again to receive the new decrypted string.
* `result` string - the decrypted string.

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
* `kwallet6` - When the desktop session is `kde6` or if the following command line flag
is provided `--password-store="kwallet6"`.
* `unknown` - When the function is called before app has emitted the `ready` event.
