# -*- bazel-starlark -*-

load("@builtin//struct.star", "module")

def __platform_properties(ctx):
    container_image = "docker://gcr.io/chops-public-images-prod/rbe/siso-chromium/linux@sha256:d7cb1ab14a0f20aa669c23f22c15a9dead761dcac19f43985bf9dd5f41fbef3a"
    return {
        "default": {
            "OSFamily": "Linux",
            "container-image": container_image,
        },
        "large": {
            "OSFamily": "Linux",
            "container-image": container_image,
        },
    }

backend = module(
    "backend",
    platform_properties = __platform_properties,
)
