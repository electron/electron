# GPUFeatureStatus Object

* `2d_canvas` string - Canvas.
* `flash_3d` string - Flash.
* `flash_stage3d` string - Flash Stage3D.
* `flash_stage3d_baseline` string - Flash Stage3D Baseline profile.
* `gpu_compositing` string - Compositing.
* `multiple_raster_threads` string - Multiple Raster Threads.
* `native_gpu_memory_buffers` string - Native GpuMemoryBuffers.
* `rasterization` string - Rasterization.
* `video_decode` string - Video Decode.
* `video_encode` string - Video Encode.
* `vpx_decode` string - VPx Video Decode.
* `webgl` string - WebGL.
* `webgl2` string - WebGL2.

Possible values:

* `disabled_software` - Software only. Hardware acceleration disabled (yellow)
* `disabled_off` - Disabled (red)
* `disabled_off_ok` - Disabled (yellow)
* `unavailable_software` - Software only, hardware acceleration unavailable (yellow)
* `unavailable_off` - Unavailable (red)
* `unavailable_off_ok` - Unavailable (yellow)
* `enabled_readback` - Hardware accelerated but at reduced performance (yellow)
* `enabled_force` - Hardware accelerated on all pages (green)
* `enabled` - Hardware accelerated (green)
* `enabled_on` - Enabled (green)
* `enabled_force_on` - Force enabled (green)
