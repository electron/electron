# HIDDevice Object

* `deviceId` string - Unique identifier for the device.
* `name` string - Name of the device.
* `vendorId` Integer - The USB vendor ID.
* `productId` Integer - The USB product ID.
* `serialNumber` string (optional) - The USB device serial number.
* `guid` string (optional) - Unique identifier for the HID interface.  A device may have multiple HID interfaces.
* `collections` Object[] - an array of report formats. See [MDN documentation](https://developer.mozilla.org/en-US/docs/Web/API/HIDDevice/collections) for more.
  * `usage` Integer - An integer representing the usage ID component of the HID usage associated with this collection.
  * `usagePage` Integer - An integer representing the usage page component of the HID usage associated with this collection.
  * `type` Integer - An 8-bit value representing the collection type, which describes a different relationship between the grouped items.
  * `children` Object[] - An array of sub-collections which takes the same format as a top-level collection.
  * `inputReports` Object[] - An array of inputReport items which represent individual input reports described in this collection.
  * `outputReports` Object[] - An array of outputReport items which represent individual output reports described in this collection.
  * `featureReports` Object[] - An array of featureReport items which represent individual feature reports described in this collection.
