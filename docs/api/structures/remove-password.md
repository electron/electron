# RemovePassword Object

* `type` String - `password`.
* `origin` String (optional) - When provided, the authentication info
  related to the origin will only be removed otherwise the entire cache
  will be cleared.
* `scheme` String (optional) - Scheme of the authentication.
  Can be `basic`, `digest`, `ntlm`, `negotiate`. Must be provided if
  removing by `origin`.
* `realm` String (optional) - Realm of the authentication. Must be provided if
  removing by `origin`.
* `username` String (optional) - Credentials of the authentication. Must be
  provided if removing by `origin`.
* `password` String (optional) - Credentials of the authentication. Must be
  provided if removing by `origin`.
