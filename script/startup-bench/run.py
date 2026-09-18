#!/usr/bin/env python3
"""Electron cold-start-to-first-paint benchmark orchestrator.

Spawns Electron with the bench app, watches the Xvfb framebuffer pixel at the window
centre, and records (epoch-ms) when the native window appears (white) and when the
page content (red) is visible. Merges with the milestones the app prints.
"""
import argparse, collections, json, os, signal, statistics, subprocess, sys, threading, time
from Xlib import X, display

def epoch_ms():
    return time.clock_gettime_ns(time.CLOCK_REALTIME) / 1e6

def classify(px):
    b, g, r = px[0], px[1], px[2]
    if r > 200 and g < 70 and b < 70: return 'red'
    if r > 200 and g > 200 and b > 200: return 'white'
    if r < 40 and g < 40 and b < 40: return 'black'
    return 'other(%d,%d,%d)' % (r, g, b)

def evict_dir(d):
    n = 0
    for root, dirs, files in os.walk(d):
        for f in files:
            try:
                fd = os.open(os.path.join(root, f), os.O_RDONLY)
                os.posix_fadvise(fd, 0, 0, os.POSIX_FADV_DONTNEED); os.close(fd); n += 1
            except OSError: pass
    return n

def run_once(args, run_idx, extra_env):
    if args.evict:
        for d in args.evict: evict_dir(d)
        time.sleep(0.2)
    d = display.Display()
    root = d.screen().root
    root.change_attributes(event_mask=X.SubstructureNotifyMask)
    d.sync()
    px, py = args.probe_x, args.probe_y
    def sample():
        img = root.get_image(px, py, 1, 1, X.ZPixmap, 0xffffffff)
        return classify(img.data)
    initial = sample()
    env = dict(os.environ)
    env.update(extra_env)
    env['BENCH_USER_DATA'] = args.user_data
    env['ELECTRON_BENCH_STAMPS'] = '1'
    cmd = [args.electron] + args.electron_args + ([args.app] if args.app else [])
    lines = []
    done_evt = threading.Event()
    t_before_spawn = epoch_ms()
    proc = subprocess.Popen(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL if not args.stderr else None, close_fds=True)
    t_after_spawn = epoch_ms()
    def reader():
        for raw in proc.stdout:
            line = raw.decode('utf-8', 'replace').rstrip('\n')
            lines.append(line)
            if line.startswith('BENCH_DONE:'): done_evt.set()
    th = threading.Thread(target=reader, daemon=True); th.start()
    transitions = [(initial, t_before_spawn)]
    xevents = []
    t_red = t_white = t_map = None
    deadline = t_before_spawn + args.timeout * 1000
    nsamples = 0
    while True:
        now = epoch_ms()
        if now > deadline: break
        # drain X events (MapNotify of toplevel)
        while d.pending_events():
            ev = d.next_event()
            if ev.type == X.MapNotify and t_map is None:
                t_map = epoch_ms(); xevents.append(('map', t_map))
            elif ev.type == X.ConfigureNotify and not any(e[0]=='configure' for e in xevents):
                xevents.append(('configure', epoch_ms(), ev.width, ev.height))
        c = sample(); nsamples += 1
        t = epoch_ms()
        if c != transitions[-1][0]:
            transitions.append((c, t))
            if c == 'white' and t_white is None: t_white = t
            if c == 'red' and t_red is None: t_red = t
        if t_red is not None and (done_evt.is_set() or proc.poll() is not None):
            break
        if t_red is not None and now - t_red > 5000: break
        if proc.poll() is not None and t_red is None and now - t_after_spawn > 2000:
            break
    # let the app finish its renderer probe
    done_evt.wait(timeout=5)
    if args.linger: time.sleep(args.linger)
    try: proc.send_signal(signal.SIGTERM)
    except Exception: pass
    try: proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        proc.kill(); proc.wait()
    th.join(timeout=2)
    d.close()
    res = {'run': run_idx, 't0_spawn': t_before_spawn, 'popen_ms': t_after_spawn - t_before_spawn,
           'x_initial': initial, 'x_white': t_white, 'x_red': t_red, 'x_map': t_map,
           'x_transitions': transitions, 'x_events': xevents, 'nsamples': nsamples, 'exit': proc.returncode}
    for line in lines:
        if line.startswith('BENCH_DONE:'):
            res['app'] = json.loads(line[len('BENCH_DONE:'):])
        elif line.startswith('BENCH:'):
            res.setdefault('app_msgs', []).append(json.loads(line[len('BENCH:'):]))
        elif line.startswith('BENCHTS:'):
            parts = line[len('BENCHTS:'):].split(':')
            if len(parts) >= 5:
                res.setdefault('stamps', []).append({'name': parts[0], 'tag': parts[1], 'pid': int(parts[2]), 'rt': float(parts[3]), 'mono': float(parts[4])})
    return res

def rel(res):
    """Milestones relative to t0_spawn in ms."""
    t0 = res['t0_spawn']; out = {}
    app = res.get('app', {})
    for k in ['process_creation_time','js_start','after_require_electron','will_finish_launching','ready','window_created','load_url_called',
              'did_start_loading','did_start_navigation','dom_ready','did_finish_load','did_stop_loading','ready_to_show','show_called','win_show_event','renderer_probe_done']:
        if k in app: out[k] = app[k] - t0
    r = app.get('renderer') or {}
    if r:
        to = r['timeOrigin']; out['r_time_origin'] = to - t0
        for name, st in r.get('paints', []): out['r_' + name.replace('-', '_')] = to + st - t0
        nav = r.get('nav') or {}
        for k in ['responseEnd','domInteractive','domContentLoadedEventEnd','domComplete','loadEventEnd']:
            if nav.get(k) is not None: out['r_nav_' + k] = to + nav[k] - t0
    for m in app.get('metrics', []):
        out['proc_%s_creation' % (m.get('name') or m['type']).replace(' ', '_')] = m['creationTime'] - t0
    for k in ['x_map','x_white','x_red']:
        if res.get(k): out[k] = res[k] - t0
    seen = collections.Counter()
    for st in res.get('stamps', []):
        tag = st['tag'] or 'browser'
        key = 's.%s%s' % ('' if tag == 'browser' else tag + '.', st['name'])
        seen[key] += 1
        if seen[key] > 1: key += '#%d' % seen[key]
        out[key] = st['rt'] - t0
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--electron', required=True)
    ap.add_argument('--app', default='')
    ap.add_argument('--runs', type=int, default=10)
    ap.add_argument('--warmup', type=int, default=1)
    ap.add_argument('--label', default='default')
    ap.add_argument('--out', required=True, help='jsonl output file (appended)')
    ap.add_argument('--user-data', required=True)
    ap.add_argument('--timeout', type=float, default=30)
    ap.add_argument('--settle', type=float, default=1.0)
    ap.add_argument('--probe-x', type=int, default=400)
    ap.add_argument('--probe-y', type=int, default=300)
    ap.add_argument('--env', action='append', default=[], help='KEY=VALUE for electron')
    ap.add_argument('--stderr', action='store_true')
    ap.add_argument('--linger', type=float, default=0, help='seconds to wait before SIGTERM (for traces)')
    ap.add_argument('--evict', action='append', default=[], help='directory whose files are dropped from page cache before each run')
    ap.add_argument('electron_args', nargs='*')
    args = ap.parse_args()
    extra_env = dict(e.split('=', 1) for e in args.env)
    os.makedirs(args.user_data, exist_ok=True)
    allrel = []
    for i in range(args.warmup + args.runs):
        res = run_once(args, i, extra_env)
        res['label'] = args.label; res['warmup'] = i < args.warmup
        r = rel(res); res['rel'] = r
        with open(args.out, 'a') as f: f.write(json.dumps(res) + '\n')
        tag = 'warmup' if res['warmup'] else 'run %d' % (i - args.warmup)
        print('[%s] %s: ready=%.1f window=%.1f x_map=%s x_white=%s did_finish_load=%s r_first_paint=%s x_red=%s (samples=%d)' % (
            args.label, tag, r.get('ready', -1), r.get('window_created', -1), fmt(r.get('x_map')), fmt(r.get('x_white')), fmt(r.get('did_finish_load')),
            fmt(r.get('r_first_paint')), fmt(r.get('x_red')), res['nsamples']), flush=True)
        if not res['warmup']: allrel.append(r)
        time.sleep(args.settle)
    keys = sorted({k for r in allrel for k in r}, key=lambda k: statistics.median([r[k] for r in allrel if k in r]))
    print('\n== %s: medians over %d runs (ms from spawn) ==' % (args.label, len(allrel)))
    for k in keys:
        vals = [r[k] for r in allrel if k in r]
        print('%-32s median %8.1f  min %8.1f  max %8.1f  sd %6.1f  n=%d' % (k, statistics.median(vals), min(vals), max(vals), statistics.pstdev(vals) if len(vals)>1 else 0, len(vals)))

def fmt(v): return '%.1f' % v if isinstance(v, (int, float)) else str(v)

if __name__ == '__main__':
    main()
