# ProcessMetric Object

* `pid` Integer - Process id of the process.
* `type` String - Process type (Browser or Tab or GPU etc).
* `cpu` [CPUUsage](cpu-usage.md) - CPU usage of the process.
* `creationTime` Number - Creation time for this process. Since the `pid` can be reused after a process dies,
    it is useful to use both the `pid` and the `creationTime` to uniquely identify a process.
* `sandboxed` Boolean (optional) _macOS_ _Windows_ - Whether the process is sandboxed on OS level.
* `integrityLevel` String (optional) _Windows_ - One of the following values:
  * `untrusted`
  * `low`
  * `medium`
  * `high`
  * `unknown`
