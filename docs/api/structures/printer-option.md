# PrinterOption Object

* `id` string - Driver-specific identifier to pass to `webContents.print()` as `inputTray` or `mediaType`.
* `displayName` string - Human-readable name supplied by the printer driver. May be empty.
* `isDefault` boolean - Whether this is the driver's current default for the queried printer.

IDs are opaque strings scoped to the selected printer, driver and operating
system. Do not infer an ID from a display name or reuse it for a different printer.
An array may have
no default entry if the driver does not expose its default in that list.
