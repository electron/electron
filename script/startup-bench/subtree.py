import sys, re, collections
sys.path.insert(0, __import__('os').path.dirname(__file__))
from perf_an import parse_script, parse_stamps, short, dso_short
script, stdout, root_rx = sys.argv[1], sys.argv[2], sys.argv[3]
depth = int(sys.argv[4]) if len(sys.argv) > 4 else 4
phase_from = sys.argv[5] if len(sys.argv) > 5 else None; phase_to = sys.argv[6] if len(sys.argv) > 6 else None
hz = float(sys.argv[7]) if len(sys.argv) > 7 else 9000.0
S = parse_script(script); st = parse_stamps(stdout)
bpid = next(pid for name, tag, pid, t in st if name == 'main.enter' and tag == 'browser')
tmap = {name: t for name, tag, pid, t in st if pid == bpid}
t_from = tmap.get(phase_from, 0) if phase_from else 0; t_to = tmap.get(phase_to, 1e18) if phase_to else 1e18
pid_filter = bpid if not root_rx.startswith('ANYPID:') else None
if pid_filter is None: root_rx = root_rx[7:]
rx = re.compile(root_rx)
tree = collections.Counter(); total = 0
for comm, pid, tid, t, fr in S:
    if pid_filter and pid != pid_filter: continue
    if not (t_from <= t < t_to): continue
    names = [short(sym) if sym != '[unknown]' else '[%s]' % dso_short(d) for sym, d in fr]  # leaf first
    # find root match scanning from root side
    idx = None
    for i in range(len(names) - 1, -1, -1):
        if rx.search(names[i]): idx = i; break
    if idx is None: continue
    total += 1
    path = []
    j = idx - 1
    while j >= 0 and len(path) < depth:
        n = names[j]
        # collapse noise frames
        if re.match(r'^(Builtins_|v8::internal::(Execution|Invoke)|base::internal::Invoker|base::(Once|Repeating)Callback|std::__Cr::__invoke|gin_helper::(Dispatcher|Invoker|internal::Invoke)|non-virtual thunk)', n):
            j -= 1; continue
        if path and path[-1] == n: j -= 1; continue
        path.append(n); j -= 1
    for k in range(1, len(path) + 1):
        tree[tuple(path[:k])] += 1
    if not path: tree[('<self>',)] += 1
print('%s: %d samples = %.1f ms' % (root_rx, total, total / hz * 1000))
def show(prefix, indent):
    kids = [(k, v) for k, v in tree.items() if len(k) == len(prefix) + 1 and k[:len(prefix)] == prefix]
    kids.sort(key=lambda kv: -kv[1])
    for k, v in kids:
        if v / hz * 1000 < 0.25: continue
        print('%s%5.1f  %s' % ('  ' * indent, v / hz * 1000, k[-1][:120]))
        show(k, indent + 1)
show((), 0)
