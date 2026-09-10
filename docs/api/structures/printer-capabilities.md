# PrinterCapabilities Object

* `inputTrays` [PrinterOption[]](printer-option.md) - Input trays reported by the printer driver or CUPS server.
* `mediaTypes` [PrinterOption[]](printer-option.md) - Media types, such as plain paper or labels, reported by the printer driver or CUPS server.

An empty array means the driver does not expose that capability. These lists
describe supported choices, not the stock currently loaded in the printer.
Support for a combination of tray, media type and paper size depends on the
driver. Re-query capabilities when the printer or its configuration changes.
