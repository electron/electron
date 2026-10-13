# NativeHeapProfilerOptions Object

* `dumpInterval` number (optional) - The interval in milliseconds at
  which samples are written to the trace. Each write is a full snapshot of
  every sampled allocation that is still live, so short intervals can fill the
  trace buffer quickly in long recordings. Defaults to `0`, which writes a
  single snapshot when tracing stops.
* `samplingInterval` number (optional) - The average number of bytes
  allocated between samples. Lower values give a more precise profile at a
  higher performance cost. Must be greater than `0`. Defaults to Chromium's
  default, currently `131072` (128 KiB).
