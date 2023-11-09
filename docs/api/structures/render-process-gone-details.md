# RenderProcessGoneDetails Object

* `reason` string - The reason the render process is gone.  Possible values:
  * `clean-exit` - Process exited with an exit code of zero
  * `abnormal-exit` - Process exited with a non-zero exit code
  * `killed` - Process was sent a SIGTERM or otherwise killed externally
  * `crashed` - Process crashed
  * `oom` - Process ran out of memory
  * `launch-failed` - Process never successfully launched
  * `integrity-failure` - Windows code integrity checks failed
* `exitCode` Integer - The exit code of the process, unless `reason` is
  `launch-failed`, in which case `exitCode` will be a platform-specific
  launch failure error code.
