> 이 문서는 아직 Electron 기여자가 번역하지 않았습니다.
>
> Electron에 기여하고 싶다면 [기여 가이드](https://github.com/electron/electron/blob/master/CONTRIBUTING-ko.md)를
> 참고하세요.
>
> 문서의 번역이 완료되면 이 틀을 삭제해주세요.

# Debugging on macOS

If you experience crashes or issues in Electron that you believe are not caused
by your JavaScript application, but instead by Electron itself, debugging can
be a little bit tricky, especially for developers not used to native/C++
debugging. However, using lldb, and the Electron source code, it is fairly easy
to enable step-through debugging with breakpoints inside Electron's source code.

## Requirements

* **A debug build of Electron**: The easiest way is usually building it
  yourself, using the tools and prerequisites listed in the
  [build instructions for macOS](build-instructions-osx.md). While you can
  easily attach to and debug Electron as you can download it directly, you will
  find that it is heavily optimized, making debugging substantially more
  difficult: The debugger will not be able to show you the content of all
  variables and the execution path can seem strange because of inlining,
  tail calls, and other compiler optimizations.

* **Xcode**: In addition to Xcode, also install the Xcode command line tools.
  They include LLDB, the default debugger in Xcode on Mac OS X. It supports
  debugging C, Objective-C and C++ on the desktop and iOS devices and simulator.

## Attaching to and Debugging Electron

To start a debugging session, open up Terminal and start `lldb`, passing a debug
build of Electron as a parameter.

```bash
$ lldb ./out/D/Electron.app
(lldb) target create "./out/D/Electron.app"
Current executable set to './out/D/Electron.app' (x86_64).
```

### Setting Breakpoints

LLDB is a powerful tool and supports multiple strategies for code inspection. For
this basic introduction, let's assume that you're calling a command from JavaScript
that isn't behaving correctly - so you'd like to break on that command's C++
counterpart inside the Electron source.

Relevant code files can be found in `./atom/` as well as in Brightray, found in
`./vendor/brightray/browser` and `./vendor/brightray/common`. If you're hardcore,
you can also debug Chromium directly, which is obviously found in `chromium_src`.

Let's assume that you want to debug `app.setName()`, which is defined in `browser.cc`
as `Browser::SetName()`. Set the breakpoint using the `breakpoint` command, specifying
file and line to break on:

```bash
(lldb) breakpoint set --file browser.cc --line 117
Breakpoint 1: where = Electron Framework``atom::Browser::SetName(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > const&) + 20 at browser.cc:118, address = 0x000000000015fdb4
```

Then, start Electron:

```bash
(lldb) run
```

The app will immediately be paused, since Electron sets the app's name on launch:

```bash
(lldb) run
Process 25244 launched: '/Users/fr/Code/electron/out/D/Electron.app/Contents/MacOS/Electron' (x86_64)
Process 25244 stopped
* thread #1: tid = 0x839a4c, 0x0000000100162db4 Electron Framework`atom::Browser::SetName(this=0x0000000108b14f20, name="Electron") + 20 at browser.cc:118, queue = 'com.apple.main-thread', stop reason = breakpoint 1.1
    frame #0: 0x0000000100162db4 Electron Framework`atom::Browser::SetName(this=0x0000000108b14f20, name="Electron") + 20 at browser.cc:118
   115 	}
   116
   117 	void Browser::SetName(const std::string& name) {
-> 118 	  name_override_ = name;
   119 	}
   120
   121 	int Browser::GetBadgeCount() {
(lldb)
```

To show the arguments and local variables for the current frame, run `frame variable` (or `fr v`),
which will show you that the app is currently setting the name to "Electron".

```bash
(lldb) frame variable
(atom::Browser *) this = 0x0000000108b14f20
(const string &) name = "Electron": {
    [...]
}
```

To do a source level single step in the currently selected thread, execute `step` (or `s`).
This would take you into into `name_override_.empty()`. To proceed and do a step over,
run `next` (or `n`).

```bash
(lldb) step
Process 25244 stopped
* thread #1: tid = 0x839a4c, 0x0000000100162dcc Electron Framework`atom::Browser::SetName(this=0x0000000108b14f20, name="Electron") + 44 at browser.cc:119, queue = 'com.apple.main-thread', stop reason = step in
    frame #0: 0x0000000100162dcc Electron Framework`atom::Browser::SetName(this=0x0000000108b14f20, name="Electron") + 44 at browser.cc:119
   116
   117 	void Browser::SetName(const std::string& name) {
   118 	  name_override_ = name;
-> 119 	}
   120
   121 	int Browser::GetBadgeCount() {
   122 	  return badge_count_;
```

To finish debugging at this point, run `process continue`. You can also continue until a certain
line is hit in this thread (`thread until 100`). This command will run the thread in the current
frame till it reaches line 100 in this frame or stops if it leaves the current frame.

Now, if you open up Electron's developer tools and call `setName`, you will once again hit the
breakpoint.

### Further Reading
LLDB is a powerful tool with a great documentation. To learn more about it, consider
Apple's debugging documentation, for instance the [LLDB Command Structure Reference][lldb-command-structure]
or the introduction to [Using LLDB as a Standalone Debugger][lldb-standalone].

You can also check out LLDB's fantastic [manual and tutorial][lldb-tutorial], which
will explain more complex debugging scenarios.

[lldb-command-structure]: https://developer.apple.com/library/mac/documentation/IDEs/Conceptual/gdb_to_lldb_transition_guide/document/lldb-basics.html#//apple_ref/doc/uid/TP40012917-CH2-SW2
[lldb-standalone]: https://developer.apple.com/library/mac/documentation/IDEs/Conceptual/gdb_to_lldb_transition_guide/document/lldb-terminal-workflow-tutorial.html
[lldb-tutorial]: http://lldb.llvm.org/tutorial.html
