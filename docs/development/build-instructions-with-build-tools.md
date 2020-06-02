# Build Instructions (Build-Tools)

[Electron Build Tools](https://github.com/electron/build-tools) provides an easy way to build Electron. This is a step-by-step guide use them.

## Prerequisites

Check the build prerequisites for your platform before proceeding

  * [macOS](build-instructions-macos.md#prerequisites)
  * [Linux](build-instructions-linux.md#prerequisites)
  * [Windows](build-instructions-windows.md#prerequisites)

## Installation


```
npm i -g @electron/build-tools
```

**Note(Windows):** build-tools (due to nested dependencies) might not work properly in `powershell`, please use `cmd` on Windows for optimum results.

## Getting the Code and Building Electron

To create a new build configuration and initialize a [GN](https://chromium.googlesource.com/chromium/src/tools/gn/+/48062805e19b4697c5fbd926dc649c78b6aaa138/README.md) directory:

```
e init --root=~/electron testing
```

**root:** is the place where electron source and build files reside. the default is (`~/projects/electron`)

The next step is running:
```
e sync
```

`e sync` is a wrapper around `gclient sync` from (Depot Tools)[https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up]. If you're starting from scratch, this will (slowly) fetch all the source code.

The final step is running:

```
e build
```

This command starts buillding Electron `e build` supports these builds:

| Target        | Description                                              |
|:--------------|:---------------------------------------------------------|
| breakpad      | Builds the breakpad `dump_syms` binary                   |
| chromedriver  | Builds the `chromedriver` binary                         |
| electron      | Builds the Electron binary **(Default)**                 |
| electron:dist | Builds the Electron binary and generates a dist zip file |
| mksnapshot    | Builds the `mksnapshot` binary                           |
| node:headers  | Builds the node headers `.tar.gz` file                   |

**Note**: Any extra args are passed along to (ninja)[https://ninja-build.org/].

**Note**: you can run `e sync` and `e build` immediately after creating the build configuration by passing `--bootstrap` flag ex: `e init --root=~/electron --bootstrap testing`.


## Using Electron

After you've built Electron, it's time to use it!

| Command | Description                                  |
|:--------|:---------------------------------------------|
| e start | Run the current Electron build               |
| e node  | Run the current Electron build as Node       |
| e debug | Run the current Electron build in a debugger |
| e test  | Run current Electron's spec runner           |

To switch between build configurations, you can view the previous build configurations by running:
```
e show configs
```

To choose a configuration:
```
e use <config-name>
```

To show the current configuration:
```
e show current
```

You can show other information as well:

| Command           | Description                                                    |
|:------------------|:---------------------------------------------------------------|
| e show env        | Show environment variables injected by the active build config |
| e show exe        | The path of the built Electron executable                      |
| e show root       | The path of the root directory from `e init --root`.           |
| e show src [name] | The path of the named (default: electron) source dir           |
| e show stats      | Build statistics                                               |


## Troubleshooting

### Build Tools Freezes after `goma_ctl.py ensure_start`

Make sure that you've installed the Prerequisites, running `pip install pywin32` will fix the issue.
