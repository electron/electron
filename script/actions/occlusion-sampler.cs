// Diagnostic helper for script/actions/occlusion-sampler.ps1; see there.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

public static class OcclusionSampler {
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
  [StructLayout(LayoutKind.Sequential)] public struct LASTINPUTINFO { public uint cbSize; public uint dwTime; }
  [StructLayout(LayoutKind.Sequential)] public struct MSG { public IntPtr hwnd; public uint message; public IntPtr wParam; public IntPtr lParam; public uint time; public POINT pt; }
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  public delegate IntPtr WndProc(IntPtr h, uint msg, IntPtr w, IntPtr l);
  [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
  public struct WNDCLASSEX {
    public uint cbSize; public uint style; public WndProc lpfnWndProc; public int cbClsExtra; public int cbWndExtra;
    public IntPtr hInstance; public IntPtr hIcon; public IntPtr hCursor; public IntPtr hbrBackground;
    public string lpszMenuName; public string lpszClassName; public IntPtr hIconSm;
  }

  [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc cb, IntPtr l);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll", EntryPoint = "GetWindowLongW")] static extern int GetWindowLong(IntPtr h, int idx);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] static extern bool GetLastInputInfo(ref LASTINPUTINFO i);
  [DllImport("user32.dll")] static extern bool GetCursorPos(out POINT p);
  [DllImport("user32.dll", EntryPoint = "SystemParametersInfoW")] static extern bool SystemParametersInfo(uint action, uint param, out int value, uint winIni);
  [DllImport("dwmapi.dll")] static extern int DwmGetWindowAttribute(IntPtr h, int attr, out int value, int size);
  [DllImport("kernel32.dll")] static extern uint GetTickCount();
  [DllImport("kernel32.dll", CharSet = CharSet.Unicode)] static extern IntPtr GetModuleHandle(string name);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern ushort RegisterClassEx(ref WNDCLASSEX c);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern IntPtr CreateWindowEx(int exStyle, string cls, string name, uint style, int x, int y, int w, int h, IntPtr parent, IntPtr menu, IntPtr inst, IntPtr param);
  [DllImport("user32.dll")] static extern IntPtr DefWindowProc(IntPtr h, uint msg, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] static extern int GetMessage(out MSG m, IntPtr h, uint min, uint max);
  [DllImport("user32.dll")] static extern bool TranslateMessage(ref MSG m);
  [DllImport("user32.dll")] static extern IntPtr DispatchMessage(ref MSG m);
  [DllImport("user32.dll")] static extern IntPtr RegisterPowerSettingNotification(IntPtr h, ref Guid setting, int flags);
  [DllImport("wtsapi32.dll")] static extern bool WTSRegisterSessionNotification(IntPtr h, int flags);

  const int WS_EX_TRANSPARENT = 0x20, WS_EX_TOOLWINDOW = 0x80, WS_EX_TOPMOST = 0x8, WS_EX_LAYERED = 0x80000;
  const uint WS_POPUP = 0x80000000;

  static string logPath;
  static readonly object logLock = new object();
  static readonly Dictionary<uint, string> procNames = new Dictionary<uint, string>();
  static WndProc wndProc;  // Keeps the delegate alive for the native window.

  static void Log(string line) {
    lock (logLock) {
      File.AppendAllText(logPath, DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fff") + "Z " + line + Environment.NewLine);
    }
  }

  static string ProcName(uint pid) {
    string name;
    if (procNames.TryGetValue(pid, out name)) return name;
    try { name = Process.GetProcessById((int)pid).ProcessName; } catch (Exception) { name = "?"; }
    procNames[pid] = name;
    return name;
  }

  static bool IsElectron(string proc) {
    return string.Equals(proc, "electron", StringComparison.OrdinalIgnoreCase);
  }

  static string Describe(IntPtr h, bool withRect) {
    if (h == IntPtr.Zero) return "(none)";
    var cls = new StringBuilder(256);
    var title = new StringBuilder(256);
    GetClassName(h, cls, 256);
    GetWindowText(h, title, 256);
    uint pid;
    GetWindowThreadProcessId(h, out pid);
    string proc = ProcName(pid);
    if (IsElectron(proc)) return "electron";
    string t = title.ToString();
    if (t.Length > 60) t = t.Substring(0, 60);
    int ex = GetWindowLong(h, -20);
    uint style = (uint)GetWindowLong(h, -16);
    string flags = ((ex & WS_EX_TOPMOST) != 0 ? "T" : "") + ((style & WS_POPUP) != 0 ? "P" : "") + ((ex & WS_EX_LAYERED) != 0 ? "L" : "") + (IsIconic(h) ? "I" : "");
    string s = string.Format("{0} '{1}' {2}({3})", cls, t, proc, pid);
    if (flags.Length > 0) s += " " + flags;
    if (withRect) {
      RECT r;
      GetWindowRect(h, out r);
      s += string.Format(" [{0},{1},{2},{3}]", r.Left, r.Top, r.Right, r.Bottom);
    }
    return s;
  }

  // Roughly gfx::IsWindowVisibleAndFullyOpaque(), the filter Chromium's
  // native occlusion tracker applies before a window may occlude another.
  static bool CanOcclude(IntPtr h) {
    if (!IsWindowVisible(h) || IsIconic(h)) return false;
    int ex = GetWindowLong(h, -20);
    if ((ex & (WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW)) != 0) return false;
    int cloaked;
    if (DwmGetWindowAttribute(h, 14, out cloaked, 4) == 0 && cloaked != 0) return false;
    RECT r;
    if (!GetWindowRect(h, out r) || r.Right <= r.Left || r.Bottom <= r.Top) return false;
    if (((uint)GetWindowLong(h, -16) & WS_POPUP) != 0) {
      var cls = new StringBuilder(64);
      GetClassName(h, cls, 64);
      if (cls.ToString() != "Shell_TrayWnd") return false;
    }
    return true;
  }

  // The occluding non-Electron windows above the topmost Electron window,
  // top first, and how many Electron windows could be occluded.
  static string Snapshot(out int electronWindows) {
    var above = new List<string>();
    int count = 0;
    bool reachedElectron = false;
    EnumWindows(delegate (IntPtr h, IntPtr l) {
      if (!CanOcclude(h)) return true;
      uint pid;
      GetWindowThreadProcessId(h, out pid);
      if (IsElectron(ProcName(pid))) { count++; reachedElectron = true; return true; }
      if (!reachedElectron) {
        var cls = new StringBuilder(64);
        GetClassName(h, cls, 64);
        if (cls.ToString() != "Shell_TrayWnd") above.Add(Describe(h, true));
      }
      return true;
    }, IntPtr.Zero);
    electronWindows = count;
    return string.Join("; ", above.ToArray());
  }

  static void Heartbeat() {
    var info = new LASTINPUTINFO();
    info.cbSize = (uint)Marshal.SizeOf(typeof(LASTINPUTINFO));
    GetLastInputInfo(ref info);
    int saver, lockTimeout;
    SystemParametersInfo(0x72, 0, out saver, 0);        // SPI_GETSCREENSAVERRUNNING
    SystemParametersInfo(0x2000, 0, out lockTimeout, 0);  // SPI_GETFOREGROUNDLOCKTIMEOUT
    POINT p;
    GetCursorPos(out p);
    Log(string.Format("heartbeat: idle {0}s, screensaver running {1}, foreground lock timeout {2}ms, cursor ({3},{4})",
        (GetTickCount() - info.dwTime) / 1000, saver, lockTimeout, p.X, p.Y));
  }

  static IntPtr OnMessage(IntPtr h, uint msg, IntPtr w, IntPtr l) {
    if (msg == 0x218 && w.ToInt64() == 0x8013) {  // WM_POWERBROADCAST, PBT_POWERSETTINGCHANGE
      var setting = (Guid)Marshal.PtrToStructure(l, typeof(Guid));
      Log(string.Format("power setting {0} = {1}", setting, Marshal.ReadInt32(l, 20)));
    } else if (msg == 0x2B1) {  // WM_WTSSESSION_CHANGE
      Log(string.Format("session change {0}", w.ToInt64()));
    }
    return DefWindowProc(h, msg, w, l);
  }

  // A hidden window that logs display on/off (what Chromium's tracker marks
  // every window occluded for) and session lock changes.
  static void WatchDisplay() {
    try {
      wndProc = OnMessage;
      var wc = new WNDCLASSEX();
      wc.cbSize = (uint)Marshal.SizeOf(typeof(WNDCLASSEX));
      wc.lpfnWndProc = wndProc;
      wc.hInstance = GetModuleHandle(null);
      wc.lpszClassName = "ElectronCIOcclusionSampler";
      RegisterClassEx(ref wc);
      IntPtr h = CreateWindowEx(WS_EX_TOOLWINDOW, wc.lpszClassName, "", WS_POPUP, 0, 0, 0, 0, IntPtr.Zero, IntPtr.Zero, wc.hInstance, IntPtr.Zero);
      var sessionDisplay = new Guid("2B84C20E-AD23-4ddf-93DB-05FFBD7EFCA5");  // GUID_SESSION_DISPLAY_STATUS
      var consoleDisplay = new Guid("6FE69556-704A-47A0-8F24-C28D936FDA47");  // GUID_CONSOLE_DISPLAY_STATE
      RegisterPowerSettingNotification(h, ref sessionDisplay, 0);
      RegisterPowerSettingNotification(h, ref consoleDisplay, 0);
      WTSRegisterSessionNotification(h, 0);
      MSG m;
      while (GetMessage(out m, IntPtr.Zero, 0, 0) > 0) {
        TranslateMessage(ref m);
        DispatchMessage(ref m);
      }
    } catch (Exception e) {
      Log("display watcher failed: " + e.Message);
    }
  }

  public static void Run(string path, int seconds) {
    logPath = path;
    Log("sampler started");
    var t = new Thread(WatchDisplay);
    t.IsBackground = true;
    t.Start();
    string last = null;
    DateTime deadline = DateTime.UtcNow.AddSeconds(seconds);
    DateTime nextBeat = DateTime.UtcNow;
    while (DateTime.UtcNow < deadline) {
      try {
        if (DateTime.UtcNow >= nextBeat) { Heartbeat(); nextBeat = DateTime.UtcNow.AddSeconds(30); }
        int electronWindows;
        string above = Snapshot(out electronWindows);
        string state = "foreground " + Describe(GetForegroundWindow(), false) + " | above electron: " + (above.Length > 0 ? above : "-");
        if (state != last) {
          Log(state + " | electron windows " + electronWindows);
          last = state;
        }
      } catch (Exception e) {
        Log("sample failed: " + e.Message);
      }
      Thread.Sleep(200);
    }
    Log("sampler finished");
  }
}
