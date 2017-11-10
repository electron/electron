# PrinterInfo Object

* `name` String
* `description` String
* `status` Number
* `isDefault` Boolean

## Example

Below is an example of some of the additional options that may be set which
may be different on each platform.

```javascript
{
  name: 'Zebra_LP2844',
  description: 'Zebra LP2844',
  status: 3,
  isDefault: false,
  options: {
    copies: '1',
    'device-uri': 'usb://Zebra/LP2844?location=14200000',
    finishings: '3',
    'job-cancel-after': '10800',
    'job-hold-until': 'no-hold',
    'job-priority': '50',
    'job-sheets': 'none,none',
    'marker-change-time': '0',
    'number-up': '1',
    'printer-commands': 'none',
    'printer-info': 'Zebra LP2844',
    'printer-is-accepting-jobs': 'true',
    'printer-is-shared': 'true',
    'printer-location': '',
    'printer-make-and-model': 'Zebra EPL2 Label Printer',
    'printer-state': '3',
    'printer-state-change-time': '1484872644',
    'printer-state-reasons': 'offline-report',
    'printer-type': '36932',
    'printer-uri-supported': 'ipp://localhost/printers/Zebra_LP2844',
    system_driverinfo: 'Z'
  }
}
```
