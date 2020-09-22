# ProcessMetric Object

* `pid` Integer - Process id of the process.
* `type` String - Process type. One of the following values:
  * `Browser`
  * `Tab`
  * `Utility`
  * `Zygote`
  * `Sandbox helper`
  * `GPU`
  * `Pepper Plugin`
  * `Pepper Plugin Broker`
  * `Unknown`
* `name` String (optional) - The name of the process. i.e. for plugins it might be Flash.
    Examples for utility: `Audio Service`, `Content Decryption Module Service`, `Network Service`, `Video Capture`, etc.
* `cpu` [CPUUsage](cpu-usage.md) - CPU usage of the process.
* `creationTime` Number - Creation time for this process.
    The time is represented as number of milliseconds since epoch.
    Since the `pid` can be reused after a process dies,
    it is useful to use both the `pid` and the `creationTime` to uniquely identify a process.
* `memory` [MemoryInfo](memory-info.md) - Memory information for the process.
* `sandboxed` Boolean (optional) _macOS_ _Windows_ - Whether the process is sandboxed on OS level.
* `integrityLevel` String (optional) _Windows_ - One of the following values:
  * `untrusted`
  * `low`
  * `medium`
  * `high`
  * `unknown`
