# ProcessMemoryInfo Object

* `residentSet` Integer _Linux_ _Windows_ - The amount of memory
currently pinned to actual physical RAM in Kilobytes.
* `private` Integer - The amount of memory not shared by other processes, such as JS heap or HTML content in Kilobytes.
* `shared` Integer - The amount of memory shared between processes, typically
  memory consumed by the Electron code itself in Kilobytes.
