#!/usr/bin/env python3
"""Analyse a Chrome JSON startup trace: process/thread map, milestone timeline, top slices per thread."""
import json, sys, re, gzip, collections, argparse

def load(path):
    op = gzip.open if path.endswith('.gz') else open
    with op(path, 'rt') as f: data = json.load(f)
    ev = data['traceEvents'] if isinstance(data, dict) else data
    return ev, (data.get('metadata', {}) if isinstance(data, dict) else {})

class Trace:
    def __init__(self, path):
        self.ev, self.meta = load(path)
        self.pname = {}; self.tname = {}; self.plabels = {}
        for e in self.ev:
            if e.get('ph') == 'M':
                if e['name'] == 'process_name': self.pname[e['pid']] = e['args'].get('name')
                elif e['name'] == 'thread_name': self.tname[(e['pid'], e['tid'])] = e['args'].get('name')
                elif e['name'] == 'process_labels': self.plabels[e['pid']] = e['args'].get('labels')
        ts = [e['ts'] for e in self.ev if e.get('ph') in ('X','B','E','I','i','R','n','b','e') and 'ts' in e and e['ts'] > 0]
        ts.sort(); med = ts[len(ts)//2]
        self.t0 = next(t for t in ts if med - t < 120e6)  # ignore bogus near-zero timestamps
        self.X = [e for e in self.ev if e.get('ph') == 'X' and 'dur' in e]
    def procs(self):
        c = collections.Counter(e['pid'] for e in self.ev if 'pid' in e)
        out = []
        for pid, n in c.most_common():
            first = min((e['ts'] for e in self.ev if e.get('pid') == pid and e.get('ts', 0) >= self.t0), default=None)
            out.append((pid, self.pname.get(pid), self.plabels.get(pid), n, (first - self.t0)/1000 if first else None))
        return out
    def pid_of(self, kind):
        for pid, name in self.pname.items():
            if name and kind.lower() in name.lower(): return pid
        return None
    def tid_of(self, pid, tname):
        for (p, t), n in self.tname.items():
            if p == pid and n == tname: return t
        return None
    def rel(self, ts): return (ts - self.t0) / 1000.0
    def first(self, pattern, pid=None, cat=None):
        rx = re.compile(pattern)
        best = None
        for e in self.ev:
            if 'ts' not in e or e['ts'] < self.t0: continue
            if pid is not None and e.get('pid') != pid: continue
            if not rx.search(e.get('name', '')): continue
            if cat and cat not in e.get('cat', ''): continue
            if best is None or e['ts'] < best['ts']: best = e
        return best
    def top_slices(self, pid, tid=None, t_from=None, t_to=None, n=40, min_ms=1.0, name_filter=None):
        out = []
        for e in self.X:
            if e['pid'] != pid: continue
            if tid is not None and e['tid'] != tid: continue
            st = self.rel(e['ts']); d = e['dur'] / 1000.0
            if t_from is not None and st < t_from: continue
            if t_to is not None and st > t_to: continue
            if d < min_ms: continue
            if name_filter and not re.search(name_filter, e['name']): continue
            out.append((st, d, e['name'], e.get('cat'), e['tid'], e.get('args')))
        out.sort(key=lambda x: -x[1])
        return out[:n]
    def self_time(self, pid, tid, t_from=None, t_to=None):
        """Aggregate self time by slice name on one thread (X events only)."""
        sl = [e for e in self.X if e['pid'] == pid and e['tid'] == tid]
        sl.sort(key=lambda e: (e['ts'], -e['dur']))
        # compute self time via stack
        selft = collections.Counter(); total = collections.Counter()
        stack = []  # (end_ts, name, child_time_accum as list)
        for e in sl:
            ts, dur = e['ts'], e['dur']; end = ts + dur
            if t_from is not None and self.rel(ts) < t_from: continue
            if t_to is not None and self.rel(ts) > t_to: continue
            while stack and stack[-1][0] <= ts:
                se, sn, sc, sd = stack.pop(); selft[sn] += (sd - sc[0]) / 1000.0
            if stack: stack[-1][2][0] += dur
            stack.append((end, e['name'], [0], dur)); total[e['name']] += dur / 1000.0
        while stack:
            se, sn, sc, sd = stack.pop(); selft[sn] += (sd - sc[0]) / 1000.0
        return selft, total

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('trace'); ap.add_argument('--top', type=int, default=30); ap.add_argument('--until', type=float, default=None, help='ms rel; limit analysis window')
    a = ap.parse_args()
    T = Trace(a.trace)
    print('trace t0 (first event) = %.3f s (abs us %d); clock-domain etc: %s' % (0, T.t0, {k: T.meta.get(k) for k in ['clock-domain','trace-config','command_line'] if k in T.meta}))
    print('\n== processes ==')
    for pid, name, labels, n, first in T.procs():
        print('  pid %-7s %-28s labels=%-30s events=%-7d first_event=+%s ms' % (pid, name, labels, n, '%.1f' % first if first is not None else '?'))
    bp = T.pid_of('Browser'); rp = T.pid_of('Renderer'); gp = T.pid_of('GPU')
    milestones = [
        ('browser', bp, r'^ContentMainRunnerImpl::Initialize$'), ('browser', bp, r'^BrowserMainRunnerImpl::Initialize'), ('browser', bp, r'^BrowserMainLoop::EarlyInitialization'),
        ('browser', bp, r'PostEarlyInitialization'), ('browser', bp, r'NodeBindings::Initialize'), ('browser', bp, r'^BrowserMainLoop::CreateStartupTasks'),
        ('browser', bp, r'^BrowserMainLoop::PreCreateThreads'), ('browser', bp, r'^BrowserMainLoop::CreateThreads'), ('browser', bp, r'^BrowserMainLoop::PostCreateThreads'),
        ('browser', bp, r'ToolkitInitialized'), ('browser', bp, r'^BrowserMainLoop::PreMainMessageLoopRun'), ('browser', bp, r'GpuProcessHost::LaunchGpuProcess|LaunchGpuProcess'),
        ('browser', bp, r'RenderProcessHostImpl::Init'), ('browser', bp, r'ChildProcessLauncher|LaunchOnLauncherThread'), ('browser', bp, r'NavigationRequest::NavigationRequest|NavigationRequest::BeginNavigation'),
        ('browser', bp, r'NavigationRequest::CommitNavigation'), ('browser', bp, r'DidCommitProvisionalLoad|DidCommitNavigation'),
        ('browser', bp, r'Widget::Init|DesktopWindowTreeHostPlatform::Init|X11Window'), ('browser', bp, r'WebContentsImpl::Init|WebContentsImpl::CreateWithOpener'),
        ('gpu', gp, r'GpuMain|GpuInit|gpu::InitializeGLOneOffPlatform|InitializeGLOneOff'), ('gpu', gp, r'GpuServiceImpl::InitializeWithHost|GpuChildThread'),
        ('gpu', gp, r'Display::DrawAndSwap|DirectRenderer::DrawFrame'), ('gpu', gp, r'SwapBuffers|PostSubBuffer|SoftwareOutputDevice|PresentFrame'),
        ('renderer', rp, r'RendererMain|RenderThreadImpl::Init|RenderThreadImpl::RenderThreadImpl'), ('renderer', rp, r'RenderFrameImpl::Initialize|RenderFrameImpl::RenderFrameImpl|RenderFrameImpl::CreateFrame'),
        ('renderer', rp, r'CommitNavigation'), ('renderer', rp, r'DidClearWindowObject'), ('renderer', rp, r'ParseHTML|HTMLDocumentParser'), ('renderer', rp, r'UpdateLayoutTree|Layout'),
        ('renderer', rp, r'^Paint$|PaintContents|LocalFrameView::RunPaintLifecyclePhase'), ('renderer', rp, r'ProxyMain::BeginMainFrame|BeginMainFrame'), ('renderer', rp, r'ProxyImpl::ScheduledActionCommit|Commit'),
        ('renderer', rp, r'firstPaint'), ('renderer', rp, r'firstContentfulPaint'), ('renderer', rp, r'SubmitCompositorFrame|LayerTreeHostImpl::DrawLayers|DrawFrame'),
    ]
    print('\n== milestone first-occurrence (ms from first trace event) ==')
    rows = []
    for label, pid, pat in milestones:
        if pid is None: continue
        e = T.first(pat, pid=pid)
        if e: rows.append((T.rel(e['ts']), label, e['name'], (e.get('dur', 0)/1000.0), T.tname.get((e['pid'], e['tid']))))
    for r in sorted(rows): print('  +%8.1f ms  %-8s %-55s dur=%7.1f  thread=%s' % r)
    until = a.until
    for label, pid, tn in [('browser main', bp, 'CrBrowserMain'), ('renderer main', rp, 'CrRendererMain'), ('gpu main', gp, 'CrGpuMain'), ('viz compositor (gpu)', gp, 'VizCompositorThread'), ('renderer compositor', rp, 'Compositor')]:
        if pid is None: continue
        tid = T.tid_of(pid, tn)
        if tid is None: print('\n(no thread %s in pid %s)' % (tn, pid)); continue
        print('\n== top slices on %s (pid %s tid %s)%s ==' % (label, pid, tid, ' until +%.0fms' % until if until else ''))
        for st, d, name, cat, tid_, args in T.top_slices(pid, tid, t_to=until, n=a.top):
            print('  +%8.1f  %7.1f ms  %-60s [%s]' % (st, d, name[:60], (cat or '')[:30]))
        selft, total = T.self_time(pid, tid, t_to=until)
        print('  -- self time by name (top 25):')
        for name, v in selft.most_common(25): print('     %7.1f ms self  (%7.1f total)  %s' % (v, total[name], name[:80]))
    # other browser threads with significant work
    if bp:
        print('\n== browser process: busiest threads (sum of top-level X durations%s) ==' % (' until +%.0fms' % until if until else ''))
        per = collections.Counter()
        for e in T.X:
            if e['pid'] != bp: continue
            if until and T.rel(e['ts']) > until: continue
        # approximate: self-time sum per thread
        for (p, t), n in T.tname.items():
            if p != bp: continue
            s, _ = T.self_time(bp, t, t_to=until); per[n or str(t)] = sum(s.values())
        for n, v in per.most_common(12): print('  %8.1f ms  %s' % (v, n))

if __name__ == '__main__': main()
