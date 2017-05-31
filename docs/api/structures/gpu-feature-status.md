# GPUFeatureStatus Object

* `2d_canvas` String - Canvas
* `flash_3d` String - Flash
* `flash_stage3d` String - Flash Stage3D
* `flash_stage3d_baseline` String - Flash Stage3D Baseline profile
* `gpu_compositing` String - Compositing
* `multiple_raster_threads` String - Multiple Raster Threads
* `native_gpu_memory_buffers` String - Native GpuMemoryBuffers
* `rasterization` String - Rasterization
* `video_decode` String - Video Decode
* `video_encode` String - Video Encode
* `vpx_decode` String - VPx Video Decode
* `webgl` String - WebGL
* `webgl2` String - WebGL2

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
