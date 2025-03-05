# ServiceWorkerInfo Object

* `scriptUrl` string - The full URL to the script that this service worker runs
* `scope` string - The base URL that this service worker is active for.
* `renderProcessId` number - The virtual ID of the process that this service worker is running in.  This is not an OS level PID.  This aligns with the ID set used for `webContents.getProcessId()`.
* `versionId` number - ID of the service worker version
