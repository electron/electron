#!/usr/bin/env python3
"""Bucket perf samples by process/thread and by stamp-delimited phase; print top frames."""
import sys, re, collections, argparse

def parse_script(path):
    samples = []  # (comm, pid, tid, t_sec, [frames (sym, dso)] leaf-first)
    with open(path, errors='replace') as f:
        cur = None
        for line in f:
            if not line.strip():
                if cur: samples.append(cur); cur = None
                continue
            if not line.startswith('\t') and not line.startswith(' ' * 8):
                m = re.match(r'\s*(\S.*?)\s+(\d+)/(\d+)\s+([\d.]+):', line)
                if m:
                    if cur: samples.append(cur)
                    cur = [m.group(1), int(m.group(2)), int(m.group(3)), float(m.group(4)), []]
                continue
            if cur is not None:
                m = re.match(r'\s+([0-9a-f]+)\s+(.*?)\s+\((.*)\)\s*$', line)
                if m: cur[4].append((m.group(2), m.group(3)))
        if cur: samples.append(cur)
    return samples

def parse_stamps(path):
    st = []
    for line in open(path, errors='replace'):
        if line.startswith('BENCHTS:'):
            p = line.strip().split(':')
            st.append((p[1], p[2], int(p[3]), float(p[5]) / 1000.0))
    return st

def short(sym):
    sym = re.sub(r'\+0x[0-9a-f]+$', '', sym)
    sym = re.sub(r'\(.*', '', sym) if len(sym) > 90 else sym
    return sym[:110]

def dso_short(d):
    return d.rsplit('/', 1)[-1]

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('script'); ap.add_argument('stdout'); ap.add_argument('--top', type=int, default=18)
    a = ap.parse_args()
    S = parse_script(a.script); stamps = parse_stamps(a.stdout)
    bpid = next(pid for name, tag, pid, t in stamps if name == 'main.enter' and tag == 'browser')
    t_main = next(t for name, tag, pid, t in stamps if name == 'main.enter' and tag == 'browser')
    t0 = min(s[3] for s in S if s[1] == bpid)  # first sample of browser process ~ exec
    print('browser pid %d; first sample %.1f ms before main()' % (bpid, (t_main - t0) * 1000))
    hz = 9000.0
    # per process/thread sample counts
    print('\n== samples by pid/tid (≈ on-CPU ms at %d Hz) ==' % hz)
    c = collections.Counter((s[1], s[2], s[0]) for s in S)
    tag_of_pid = {}
    for name, tag, pid, t in stamps:
        if name == 'main.enter': tag_of_pid[pid] = tag or 'browser'
        if name == 'delegate.zygote_forked': tag_of_pid[pid] = tag
        if name.startswith('delegate.create_') and pid not in tag_of_pid: tag_of_pid[pid] = tag
    for (pid, tid, comm), n in c.most_common(25):
        print('  %6.1f ms  pid %d (%s) tid %d %s' % (n / hz * 1000, pid, tag_of_pid.get(pid, '?'), tid, comm))
    # phases for browser main thread
    bst = [(name, t) for name, tag, pid, t in stamps if pid == bpid]
    bst.sort(key=lambda x: x[1])
    bounds = [('exec', t0)] + bst
    main_samples = [s for s in S if s[1] == bpid and s[2] == bpid]
    print('\n== browser main thread phases ==')
    for i, (name, t) in enumerate(bounds):
        t_end = bounds[i + 1][1] if i + 1 < len(bounds) else t + 10
        ss = [s for s in main_samples if t <= s[3] < t_end]
        if len(ss) < 4: continue
        print('\n-- %s → %s : %.1f ms wall, %.1f ms on-CPU (%d samples)' % (name, bounds[i+1][0] if i+1 < len(bounds) else 'end', (t_end - t) * 1000, len(ss) / hz * 1000, len(ss)))
        leaf = collections.Counter(); dso = collections.Counter(); incl = collections.Counter()
        for s in ss:
            fr = s[4]
            if not fr: continue
            # skip unknown kernel leaf
            lf = next(((sym, d) for sym, d in fr if sym != '[unknown]' or 'kernel' in d or d == '[unknown]'), fr[0])
            leaf[(short(fr[0][0]), dso_short(fr[0][1]))] += 1
            dso[dso_short(fr[0][1])] += 1
            seen = set()
            for sym, d in fr:
                k = short(sym)
                if k in seen or k == '[unknown]': continue
                seen.add(k); incl[k] += 1
        print('   leaf DSOs: ' + ', '.join('%s %.1f' % (k, v / hz * 1000) for k, v in dso.most_common(6)))
        print('   top leaf frames:')
        for (k, d), v in leaf.most_common(a.top // 2): print('     %5.1f ms  %s (%s)' % (v / hz * 1000, k, d))
        print('   top inclusive frames:')
        for k, v in incl.most_common(a.top): print('     %5.1f ms  %s' % (v / hz * 1000, k))

    # other processes: gpu main thread, renderer main thread, zygote
    for pid, tag in tag_of_pid.items():
        if pid == bpid: continue
        ss = [s for s in S if s[1] == pid and s[2] == pid]
        if len(ss) < 20: continue
        tt0 = min(s[3] for s in ss)
        print('\n== pid %d (%s) main thread: %.1f ms on-CPU, first sample at +%.1f ms from browser exec ==' % (pid, tag, len(ss) / hz * 1000, (tt0 - t0) * 1000))
        incl = collections.Counter(); dso = collections.Counter()
        for s in ss:
            fr = s[4]
            if not fr: continue
            dso[dso_short(fr[0][1])] += 1
            seen = set()
            for sym, d in fr:
                k = short(sym)
                if k in seen or k == '[unknown]': continue
                seen.add(k); incl[k] += 1
        print('   leaf DSOs: ' + ', '.join('%s %.1f' % (k, v / hz * 1000) for k, v in dso.most_common(6)))
        for k, v in incl.most_common(a.top): print('     %5.1f ms  %s' % (v / hz * 1000, k))

if __name__ == '__main__': main()
