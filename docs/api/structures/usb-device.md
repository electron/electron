# USBDevice Object

* `configuration` Object (optional) - A [USBConfiguration](https://developer.mozilla.org/en-US/docs/Web/API/USBConfiguration) object containing information about the currently selected configuration of a USB device.
  * `configurationValue` Integer - the configuration value of this configuration.
  * `configurationName` string - the name provided by the device to describe this configuration.
  * `interfaces` Object[] - An array of [USBInterface](https://developer.mozilla.org/en-US/docs/Web/API/USBInterface) objects containing information about an interface provided by the USB device.
    * `interfaceNumber` Integer - the interface number of this interface.
    * `alternate` Object - the currently selected alternative configuration of this interface.
      * `alternateSetting` Integer - the alternate setting number of this interface.
      * `interfaceClass` Integer - the class of this interface. See [USB.org](https://www.usb.org/defined-class-codes) for class code descriptions.
      * `interfaceSubclass` Integer - the subclass of this interface.
      * `interfaceProtocol` Integer - the protocol supported by this interface.
      * `interfaceName` string (optional) - the name of the interface, if one is provided by the device.
      * `endpoints` Object[] - an array containing instances of the [USBEndpoint interface](https://developer.mozilla.org/en-US/docs/Web/API/USBEndpoint) describing each of the endpoints that are part of this interface.
        * `endpointNumber` Integer - this endpoint's "endpoint number" which is a value from 1 to 15.
        * `direction` string - the direction in which this endpoint transfers data - can be either 'in' or 'out'.
        * `type` string - the type of this endpoint - can be either 'bulk', 'interrupt', or 'isochronous'.
        * `packetSize` Integer - the size of the packets that data sent through this endpoint will be divided into.
    * `alternates` Object[] - an array containing instances of the [USBAlternateInterface](https://developer.mozilla.org/en-US/docs/Web/API/USBAlternateInterface) interface describing each of the alternative configurations possible for this interface.
* `configurations` Object[] - An array of [USBConfiguration](https://developer.mozilla.org/en-US/docs/Web/API/USBConfiguration) interfaces for controlling a paired USB device.
* `deviceClass` Integer - The device class for the communication interface supported by the device.
* `deviceId` string - Unique identifier for the device.
* `deviceProtocol` Integer - The device protocol for the communication interface supported by the device.
* `deviceSubclass` Integer - The device subclass for the communication interface supported by the device.
* `deviceVersionMajor` Integer - The major version number of the device as defined by the device manufacturer.
* `deviceVersionMinor` Integer - The minor version number of the device as defined by the device manufacturer.
* `deviceVersionSubminor` Integer - The subminor version number of the device as defined by the device manufacturer.
* `manufacturerName` string (optional) - The manufacturer name of the device.
* `productId` Integer - The USB product ID.
* `productName` string (optional) - Name of the device.
* `serialNumber` string (optional) - The USB device serial number.
* `usbVersionMajor` Integer - The USB protocol major version supported by the device.
* `usbVersionMinor` Integer - The USB protocol minor version supported by the device.
* `usbVersionSubminor` Integer - The USB protocol subminor version supported by the device.
* `vendorId` Integer - The USB vendor ID.
