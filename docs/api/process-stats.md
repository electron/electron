# processStats

> Collect memory statistics about the system as well as the current process (either
> the main process or any renderer process)

```js
import {processStats} from 'electron';

processStats.getProcessMemoryInfo();
>>> Object {workingSetSize: 86052, peakWorkingSetSize: 92244, privateBytes: 25932, sharedBytes: 60248}
```

## Methods

The processStats has the following methods:

### getProcessMemoryInfo()

Return an object giving memory usage statistics about the current process. Note that
all statistics are reported in Kilobytes.

* `workingSetSize` - The amount of memory currently pinned to actual physical RAM
* `peakWorkingSetSize` - The maximum amount of memory that has ever been pinned to actual physical RAM
* `privateBytes` - The amount of memory not shared by other processes, such as JS heap or HTML content.
* `sharedBytes` - The amount of memory shared between processes, typically memory consumed by the Electron code itself

### getSystemMemoryInfo()

Return an object giving memory usage statistics about the entire system. Note that
all statistics are reported in Kilobytes.

* `total` - The total amount of physical memory in Kilobytes available to the system
* `free` - The total amount of memory not being used by applications or disk cache

On Windows / Linux:

* `swapTotal` - The total amount of swap memory in Kilobytes available to the system
* `swapFree` - The free amount of swap memory in Kilobytes available to the system
