# -*- bazel-starlark -*-

load("@builtin//struct.star", "module")

def __platform_properties(ctx):
    container_image = "docker://gcr.io/chops-public-images-prod/rbe/siso-chromium/linux@sha256:ef35d347f4a4a2d32b76fd908e66e96f59bf8ba7379fd5626548244c45343b2b"
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
