load("@builtin//encoding.star", "json")
load("@builtin//runtime.star", "runtime")
load("@builtin//struct.star", "module")
load("@config//main.star", upstream_init = "init")

def init(ctx):
    mod = upstream_init(ctx)
    step_config = json.decode(mod.step_config)

    # Only wrap clang rules with a remote wrapper if not on Linux. These are currently only
    # needed for X-Compile builds, which run on Windows and Mac.
    if runtime.os != "linux":
      for rule in step_config["rules"]:
        if rule["name"].startswith("clang/") or rule["name"].startswith("clang-cl/"):
          rule["remote_wrapper"] = "/bin/bash ../../buildtools/reclient_cfgs/chromium-browser-clang/clang_remote_wrapper"
          if "inputs" not in rule:
            rule["inputs"] = []
          rule["inputs"].append("buildtools/reclient_cfgs/chromium-browser-clang/clang_remote_wrapper")
          rule["inputs"].append("third_party/llvm-build/Release+Asserts_linux/bin/clang")

    return module(
      "config",
      step_config = json.encode(step_config),
      filegroups = mod.filegroups,
      handlers = mod.handlers,
    )
