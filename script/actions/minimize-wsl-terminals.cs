// Minimizes the WSL update terminals windows-11-arm keeps opening; see
// minimize-wsl-terminals.ps1.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

public static class WslTerminalMinimizer {
  public delegate bool EnumProc(IntPtr h, IntPtr l);

  [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
  struct PROCESSENTRY32 {
    public uint dwSize; public uint cntUsage; public uint th32ProcessID; public IntPtr th32DefaultHeapID;
    public uint th32ModuleID; public uint cntThreads; public uint th32ParentProcessID; public int pcPriClassBase;
    public uint dwFlags;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)] public string szExeFile;
  }

  [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc cb, IntPtr l);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr h, int cmd);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("kernel32.dll")] static extern IntPtr CreateToolhelp32Snapshot(uint flags, uint pid);
  [DllImport("kernel32.dll", CharSet = CharSet.Unicode, EntryPoint = "Process32FirstW")] static extern bool Process32First(IntPtr snap, ref PROCESSENTRY32 e);
  [DllImport("kernel32.dll", CharSet = CharSet.Unicode, EntryPoint = "Process32NextW")] static extern bool Process32Next(IntPtr snap, ref PROCESSENTRY32 e);
  [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);

  static string logPath;
  static readonly HashSet<IntPtr> seen = new HashSet<IntPtr>();

  static void Log(string line) {
    File.AppendAllText(logPath, DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fff") + "Z " + line + Environment.NewLine);
  }

  // Name and parent of every running process, keyed by pid.
  static Dictionary<uint, KeyValuePair<string, uint>> Processes() {
    var result = new Dictionary<uint, KeyValuePair<string, uint>>();
    IntPtr snap = CreateToolhelp32Snapshot(2, 0);  // TH32CS_SNAPPROCESS
    if (snap == new IntPtr(-1)) return result;
    var e = new PROCESSENTRY32();
    e.dwSize = (uint)Marshal.SizeOf(typeof(PROCESSENTRY32));
    for (bool ok = Process32First(snap, ref e); ok; ok = Process32Next(snap, ref e)) {
      result[e.th32ProcessID] = new KeyValuePair<string, uint>(e.szExeFile, e.th32ParentProcessID);
    }
    CloseHandle(snap);
    return result;
  }

  // A Windows Terminal window is titled "Terminal" until its tab reports the
  // command line; a console window hosting wsl.exe directly is titled with it.
  static bool IsWslTerminal(string cls, string title) {
    bool wsl = title.IndexOf("wsl.exe", StringComparison.OrdinalIgnoreCase) >= 0;
    if (cls == "CASCADIA_HOSTING_WINDOW_CLASS") return wsl || title == "Terminal";
    return cls == "ConsoleWindowClass" && wsl;
  }

  static void Sweep() {
    EnumWindows(delegate (IntPtr h, IntPtr l) {
      if (!IsWindowVisible(h) || IsIconic(h)) return true;
      var cls = new StringBuilder(64);
      var title = new StringBuilder(256);
      GetClassName(h, cls, 64);
      GetWindowText(h, title, 256);
      if (!IsWslTerminal(cls.ToString(), title.ToString())) return true;
      ShowWindow(h, 6);  // SW_MINIMIZE
      if (seen.Add(h)) {
        uint pid;
        GetWindowThreadProcessId(h, out pid);
        var procs = Processes();
        KeyValuePair<string, uint> proc, parent;
        string name = procs.TryGetValue(pid, out proc) ? proc.Key : "?";
        uint parentPid = proc.Value;
        string parentName = procs.TryGetValue(parentPid, out parent) ? parent.Key : "(exited)";
        Log(string.Format("minimized '{0}' {1} ({2}), parent {3} ({4})", title, name, pid, parentName, parentPid));
      }
      return true;
    }, IntPtr.Zero);
  }

  public static void Run(string path, int seconds) {
    logPath = path;
    Log("watching for WSL terminals");
    DateTime deadline = DateTime.UtcNow.AddSeconds(seconds);
    while (DateTime.UtcNow < deadline) {
      try { Sweep(); } catch (Exception e) { Log("sweep failed: " + e.Message); }
      Thread.Sleep(100);
    }
  }
}
