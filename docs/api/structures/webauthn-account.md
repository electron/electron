# WebAuthnAccount Object

* `credentialId` string - URL-safe base64-encoded (no padding) credential ID of
  the discoverable credential. Matches `PublicKeyCredential.id` returned by
  `navigator.credentials.get()` in the renderer.
* `userHandle` string (optional) - URL-safe base64-encoded (no padding) user
  handle (`user.id`) that was provided when the credential was created.
* `name` string (optional) - Human-palatable identifier for the account (for example, an email address or username).
* `displayName` string (optional) - Human-palatable name for the account, intended for display.
