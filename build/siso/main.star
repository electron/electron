load("@builtin//encoding.star", "json")
load("@builtin//path.star", "path")
load("@builtin//runtime.star", "runtime")
load("@builtin//struct.star", "module")
load("@config//main.star", upstream_init = "init")
load("@config//win_sdk.star", "win_sdk")
load("@config//gn_logs.star", "gn_logs")

def init(ctx):
    mod = upstream_init(ctx)
    step_config = json.decode(mod.step_config)

    # Buildbarn doesn't support input_root_absolute_path so disable that
    for rule in step_config["rules"]:    
      input_root_absolute_path = rule.get("input_root_absolute_path", False)
      if input_root_absolute_path:
        rule.pop("input_root_absolute_path", None)

    # Only wrap clang rules with a remote wrapper if not on Linux. These are currently only
    # needed for X-Compile builds, which run on Windows and Mac.
    if runtime.os != "linux":
      for rule in step_config["rules"]:
        if rule["name"].startswith("clang/") or rule["name"].startswith("clang-cl/"):
          rule["remote_wrapper"] = "../../buildtools/reclient_cfgs/chromium-browser-clang/clang_remote_wrapper"
          if "inputs" not in rule:
            rule["inputs"] = []
          rule["inputs"].append("buildtools/reclient_cfgs/chromium-browser-clang/clang_remote_wrapper")
          rule["inputs"].append("third_party/llvm-build/Release+Asserts_linux/bin/clang")

      if "executables" not in step_config:
        step_config["executables"] = []
      step_config["executables"].append("buildtools/reclient_cfgs/chromium-browser-clang/clang_remote_wrapper")
      step_config["executables"].append("third_party/llvm-build/Release+Asserts_linux/bin/clang")

    if runtime.os == "darwin":
      # Update platforms to match our default siso config instead of reclient configs.
      step_config["platforms"].update({
          "clang": step_config["platforms"]["default"],
          "clang_large": step_config["platforms"]["default"],          
      })      

    # Add additional Windows SDK headers needed by Electron      
    win_toolchain_dir = win_sdk.toolchain_dir(ctx)
    if win_toolchain_dir:
      sdk_version = gn_logs.read(ctx).get("windows_sdk_version")
      step_config["input_deps"][win_toolchain_dir + ":headers"].extend([
        # third_party/electron_node/deps/uv/include/uv/win.h includes mswsock.h
        path.join(win_toolchain_dir, "Windows Kits", "10/Include", sdk_version, "um/mswsock.h"),
        # third_party/electron_node/src/debug_utils.cc includes lm.h
        path.join(win_toolchain_dir, "Windows Kits", "10/Include", sdk_version, "um/Lm.h"),
      ])

    if runtime.os == "windows":      
      # Update platforms to match our default siso config instead of reclient configs.
      step_config["platforms"].update({
          "clang-cl": step_config["platforms"]["default"],
          "clang-cl_large": step_config["platforms"]["default"],
          "lld-link": step_config["platforms"]["default"],
      })

    return module(
      "config",
      step_config = json.encode(step_config),
      filegroups = mod.filegroups,
      handlers = mod.handlers,
    )
