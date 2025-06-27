# WindowStateRestoreOptions Object

* `stateId` string - A unique identifier used for saving and restoring window state.

>[!NOTE]
> Window state (position, size, etc.) is automatically captured during move and resize events and
> written to the Preferences folder inside app.getPath('userData') (when the main thread is available). This ensures > window state can be restored even after unexpected application termination or crashes.
