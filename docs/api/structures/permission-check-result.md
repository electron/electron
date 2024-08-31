# PermissionCheckResult Object

* `status` string - Can be `granted`, `denied` or `ask`. Controls whether the permission check should be approved. Granted and Denied have their implied effects, `ask` will result in a permission request being fired to your permission request handler.

Note: For media permission checks `ask` is equivilant to `granted`.
