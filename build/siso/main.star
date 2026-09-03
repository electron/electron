# Electron's siso entry point, passed to `siso ninja -load`. build-tools ships
# a copy of this; set ELECTRON_BUILD_TOOLS_MAIN_STAR to use this one instead.
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

    # Add additional Windows SDK headers needed by Electron. Use
    # win_sdk.enabled() (target_os == "win") rather than runtime.os so this
    # also applies when cross-compiling Windows on a Linux host.
    if win_sdk.enabled(ctx):
      win_toolchain_dir = win_sdk.toolchain_dir(ctx)
      sdk_version = gn_logs.read(ctx).get("windows_sdk_version")
      if win_toolchain_dir and sdk_version and (win_toolchain_dir + ":headers") in step_config["input_deps"]:
        step_config["input_deps"][win_toolchain_dir + ":headers"].extend([
          # third_party/electron_node/deps/uv/include/uv/win.h includes mswsock.h
          path.join(win_toolchain_dir, "Windows Kits/10/Include", sdk_version, "um/mswsock.h"),
          # third_party/electron_node/src/debug_utils.cc includes lm.h
          path.join(win_toolchain_dir, "Windows Kits/10/Include", sdk_version, "um/Lm.h"),
        ])

    if runtime.os == "windows":
      # Update platforms to match our default siso config instead of reclient configs.
      step_config["platforms"].update({
          "clang-cl": step_config["platforms"]["default"],
          "clang-cl_large": step_config["platforms"]["default"],
          "lld-link": step_config["platforms"]["default"],
      })

    # Chromium gates remote execution of its python/node code generators
    # (mojom, Blink bindings, grit strings, TypeScript/WebUI) on its
    # "googlechrome" config; on Electron's RBE they run fine as-is (Linux
    # workers with python3/node, all inputs checked in), so enable them for
    # every host. Each of these was validated on rbe.notgoma.com: it executes
    # remotely, produces byte-identical outputs and cache-hits on rerun.
    # Besides taking ~1 CPU-hour of python off the 5-core macOS runners per
    # build, remote steps are cached, so unchanged generators become cache
    # hits on every host instead of running locally each time.
    #
    # The typescript/webui rules need their Starlark handlers (which add the
    # node_modules etc. inputs); Chromium only attaches them when remote is
    # enabled at config time, so attach them here.
    remote_generators = {
        "mojo/mojom_bindings_generator": None,
        "mojo/mojom_parser": None,
        "mojo/generate_type_mappings": None,
        "blink/generate_bindings": None,
        "grit/chrome_app_generated_resources": None,
        "grit/components_strings": None,
        "typescript/ts_library": "typescript_ts_library",
        "webui/minify_js": None,
        "webui/stylelint": None,
        "webui/eslint_ts": "webui_eslint_ts",
    }
    for rule in step_config["rules"]:
      if rule["name"] in remote_generators:
        rule["remote"] = True
        rule["remote_command"] = "python3"
        handler = remote_generators[rule["name"]]
        if handler:
          rule["handler"] = handler

    # clang-tidy build steps from electron/script/gen-clang-tidy-ninja.py. They
    # are shaped like compiles so scandeps finds their inputs, and are cached
    # and executed remotely like compiles.
    tidy_wrapper = "electron/build/run-clang-tidy-action.sh"
    tidy_rule = {
        "name": "electron/clang-tidy",
        "action": "clang_tidy",
        "command_prefix": "../../" + tidy_wrapper + " ",
        "inputs": [
            tidy_wrapper,
            "third_party/llvm-build/Release+Asserts/bin/clang-tidy",
            ".clang-tidy",
            "electron/.clang-tidy",
        ],
        "exclude_input_patterns": ["*.stamp"],
        "remote": True,
        "timeout": "10m",
    }
    step_config["input_deps"][tidy_rule["inputs"][1]] = step_config["input_deps"].get(
        "third_party/llvm-build/Release+Asserts/bin/clang++", [])
    step_config.setdefault("executables", [])
    step_config["executables"].extend(tidy_rule["inputs"][:2])
    if runtime.os != "linux":
      # Same arrangement as the clang rules above: the RBE workers are Linux, so
      # run through clang_remote_wrapper with the Linux toolchain staged next to
      # the host one. The clang-tidy job downloads the Linux clang-tidy there.
      linux_tools = [
          "buildtools/reclient_cfgs/chromium-browser-clang/clang_remote_wrapper",
          "third_party/llvm-build/Release+Asserts_linux/bin/clang",
          "third_party/llvm-build/Release+Asserts_linux/bin/clang-tidy",
      ]
      tidy_rule["remote_wrapper"] = "../../" + linux_tools[0]
      tidy_rule["inputs"].extend(linux_tools)
      step_config["executables"].extend(linux_tools)
    step_config["rules"].insert(0, tidy_rule)

    return module(
      "config",
      step_config = json.encode(step_config),
      filegroups = mod.filegroups,
      handlers = mod.handlers,
    )
