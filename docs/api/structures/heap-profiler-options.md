# HeapProfilerOptions Object

* `sampling_interval_bytes` number (optional) - The average number of bytes
  allocated between samples. Must be greater than `0`. Defaults to `131072`
  (128 KiB).
* `sampling_interval_ms` number (optional) - The interval in milliseconds at
  which samples are written to the trace. Set to `0` to only write samples when
  tracing stops. Defaults to `50`.
