# EnableHeapProfilingOptions Object

* `mode` string (optional) - Controls which processes are profiled. Equivalent to `--memlog` in
  Chrome. Default is `all`.
  * `all` - Profile all processes.
  * `browser` - Profile only the browser process.
  * `gpu` - Profile only the GPU process.
  * `minimal` - Profile only the browser and GPU processes.
  * `renderer-sampling` - Profile at most 1 renderer process. Each renderer process has a fixed
    probability of being profiled when the renderer process is started or, for existing processes,
    when heap profiling is enabled.
  * `all-renderers` - Profile all renderer processes.
  * `utility-sampling` - Each utility process has a fixed probability of being profiled.
  * `all-utilities` - Profile all utility processes.
  * `utility-and-browser` - Profile all utility processes and the browser process.
* `samplingRate` number (optional) - Controls the sampling interval in bytes. The lower the
  interval, the more precise the profile is. However it comes at the cost of performance. Default
  is `100000` (100KB). That is enough to observe allocation sites that make allocations >500KB
  total, where total equals to a single allocation size times the number of such allocations at the
  same call site. Equivalent to `--memlog-sampling-rate` in Chrome. Must be an integer between
  `1000` and `10000000`.
* `stackMode` string (optional) - Controls the type of metadata recorded for each allocation.
  Equivalent to `--memlog-stack-mode` in Chrome. Default is `native`.
  * `native` - Instruction addresses from unwinding the stack.
  * `native-with-thread-names` - Instruction addresses from unwinding the stack. Includes the thread
    name as the first frame.
