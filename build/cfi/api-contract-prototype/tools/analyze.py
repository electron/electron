"""API-rooted CFI callback-boundary analysis for libuv (prototype).

usage: python3 -I analyze.py --root <libuv> --contract <contract.txt> --out <dir> [--discover]
"""
import argparse
import collections
import json
import multiprocessing
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import extract  # noqa: E402

COMMON = """src/fs-poll.c src/idna.c src/inet.c src/random.c src/strscpy.c src/strtok.c
src/thread-common.c src/threadpool.c src/timer.c src/uv-data-getter-setters.c src/uv-common.c
src/version.c""".split()
UNIX = """async core dl fs getaddrinfo getnameinfo loop loop-watcher pipe poll process
random-devurandom signal stream tcp thread tty udp proctitle linux procfs-exepath random-getrandom
random-sysctl-linux""".split()
SOURCES = COMMON + ["src/unix/%s.c" % s for s in UNIX]
CFLAGS = ["-D_GNU_SOURCE", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Iinclude", "-Isrc",
          "-std=gnu11"]
PUBLIC = ["include/uv.h", "include/uv/unix.h", "include/uv/linux.h", "include/uv/threadpool.h",
          "include/uv/errno.h", "include/uv/version.h", "include/uv/tree.h"]

EXEMPT_ROLES = ("addon", "external")


def load_contract(path):
    c = {}
    with open(path) as f:
        for ln, line in enumerate(f, 1):
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            parts = line.split(None, 2)
            if len(parts) < 3:
                raise SystemExit("%s:%d: need '<kind:key> <role> <reason>'" % (path, ln))
            c[parts[0]] = {"role": parts[1], "reason": parts[2], "line": ln}
    return c


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True)
    ap.add_argument("--contract", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--discover", action="store_true")
    ap.add_argument("--jobs", type=int, default=os.cpu_count())
    a = ap.parse_args()
    root = os.path.abspath(a.root)
    os.makedirs(a.out, exist_ok=True)
    t0 = time.time()
    srcs = [s for s in SOURCES if os.path.exists(os.path.join(root, s))]
    with multiprocessing.Pool(a.jobs) as pool:
        tus = pool.map(extract.extract, [(os.path.join(root, s), root, CFLAGS, PUBLIC) for s in srcs])
    t_extract = time.time() - t0

    errors = []
    warnings = []

    # ------------------------------------------------------------ public surface
    public = {}
    for tu in tus:
        for p in tu["public"]:
            public.setdefault("%s:%s" % (p["kind"], p["key"]), p)
    fields = {}
    for tu in tus:
        fields.update(tu["fields"])
    if a.discover:
        for k in sorted(public):
            p = public[k]
            print("%-60s %s:%s  %s" % (k, p["file"], p["line"], p["type"]))
        return 0

    contract = load_contract(a.contract)
    for k, p in sorted(public.items()):
        if k not in contract:
            errors.append("UNCONTRACTED public callback decl %s (%s:%s, %s): add it to the contract "
                          "with role addon|runtime|n/a" % (k, p["file"], p["line"], p["type"]))
    for k, v in contract.items():
        if k.split(":", 1)[0] in ("param", "field", "typedef") and k not in public:
            errors.append("STALE contract entry %s (contract line %d): no such public decl; "
                          "was it renamed/removed?" % (k, v["line"]))

    # ------------------------------------------------------------ graph
    funcs = {}
    for tu in tus:
        for fk, info in tu["funcs"].items():
            funcs.setdefault(fk, info)
    edges = collections.defaultdict(set)   # src node -> {dst}
    seeds = collections.defaultdict(set)   # node -> {label}
    why = {}                               # (node,label) -> (prev node or None, site)
    opaque = []
    escapes = []

    def add_term(t, dst, site):
        kind, val = t[0], t[1]
        if kind == "fn":
            lab = ("internal", val) if val in funcs or "@" in val else ("internal-ext", val)
            if lab not in seeds[dst]:
                seeds[dst].add(lab)
                why[(dst, lab)] = (None, site)
        elif kind == "node":
            edges[val].add((dst, json.dumps(site)))
        elif kind == "ret":
            if val in funcs:
                edges["R:" + val].add((dst, json.dumps(site)))
            else:
                lab = ("unknown", "return of external %s()" % val)
                seeds[dst].add(lab)
                why[(dst, lab)] = (None, site)
        elif kind == "unk":
            lab = ("unknown", val)
            seeds[dst].add(lab)
            why[(dst, lab)] = (None, site)

    for tu in tus:
        for t, dst, site in tu["flows"]:
            add_term(t, dst, site)
        for callee, i, ts, site in tu["args"]:
            if callee in funcs:
                for t in ts:
                    add_term(t, "P:%s#%d" % (callee, i), site)
            else:
                tmp = "X:%s#%d@%s:%s" % (callee, i, site["file"], site["line"])
                for t in ts:
                    add_term(t, tmp, site)
                escapes.append((callee, i, tmp, site))
        for ts, what, site in tu["opaque"]:
            tmp = "O:%s@%s:%s" % (what, site["file"], site["line"])
            for t in ts:
                add_term(t, tmp, site)
            opaque.append((what, tmp, site))

    # contract seeds
    for k, v in contract.items():
        kind, key = k.split(":", 1)
        role = v["role"]
        if kind == "param" and k in public:
            p = public[k]
            node = "P:%s#%d" % (p["func"], p["idx"])
            if p["func"] not in funcs:
                warnings.append("contract %s: %s has no body in this configuration" % (k, p["func"]))
            lab = ("addon", k) if role == "addon" else ("runtime", k)
            if role in ("addon", "runtime"):
                seeds[node].add(lab)
                why[(node, lab)] = (None, {"fn": p["func"], "file": p["file"], "line": p["line"]})
        elif kind == "field" and role == "addon-input":
            node = "F:" + key
            lab = ("addon", k)
            seeds[node].add(lab)
            why[(node, lab)] = (None, {"fn": None, "file": public.get(k, {}).get("file"),
                                       "line": public.get(k, {}).get("line")})

    # ------------------------------------------------------------ propagate
    labels = collections.defaultdict(set)
    work = collections.deque()
    for n, ls in seeds.items():
        for lab in ls:
            labels[n].add(lab)
            work.append((n, lab))
    while work:
        n, lab = work.popleft()
        for dst, site in edges.get(n, ()):
            if lab not in labels[dst]:
                labels[dst].add(lab)
                why[(dst, lab)] = (n, json.loads(site))
                work.append((dst, lab))

    def path(node, lab, limit=12):
        out = []
        cur = node
        while cur is not None and len(out) < limit:
            prev, site = why.get((cur, lab), (None, None))
            out.append("%s [%s:%s]" % (cur, (site or {}).get("file"), (site or {}).get("line")))
            cur = prev
        return list(reversed(out))

    # ------------------------------------------------------------ classify sites
    sites = []
    seen_sites = set()
    for tu in tus:
        for ic in tu["icalls"]:
            sk = (ic["fn"], ic["file"], ic["line"], ic["callee_text"])
            if sk in seen_sites:
                continue  # static inline header functions appear in several TUs
            seen_sites.add(sk)
            labs = set()
            srcs = []
            for t in ic["terms"]:
                if t[0] == "node":
                    labs |= labels.get(t[1], set())
                    srcs.append(t[1])
                elif t[0] == "fn":
                    labs.add(("internal", t[1]))
                elif t[0] == "ret":
                    labs |= labels.get("R:" + t[1], set()) if t[1] in funcs else \
                        {("unknown", "return of external %s()" % t[1])}
                    srcs.append("R:" + t[1])
                else:
                    labs.add(("unknown", t[1]))
            kinds = {lab[0] for lab in labs}
            site_key = "site:%s:%s" % (ic["fn"], ic["callee_text"])
            reviewed = contract.get(site_key)
            if reviewed and reviewed["role"] == "external":
                cls = "external (reviewed)"
            elif "addon" in kinds:
                cls = "addon"
            elif "unknown" in kinds or not labs:
                if reviewed and reviewed["role"] in ("internal", "external"):
                    cls = reviewed["role"] + " (reviewed)"
                else:
                    cls = "UNRESOLVED"
            elif "internal-ext" in kinds:
                cls = "internal"  # external symbols referenced by name in libuv source
            else:
                cls = "internal"
            prov = []
            for lab in sorted(labs):
                for s in srcs:
                    if lab in labels.get(s, set()):
                        prov.append({"label": list(lab), "path": path(s, lab)})
                        break
                else:
                    prov.append({"label": list(lab), "path": []})
            sites.append(dict(ic, cls=cls, labels=sorted(map(list, labs)), provenance=prov,
                              site_key=site_key))
    sites.sort(key=lambda s: (s["file"] or "", s["line"] or 0))

    for s in sites:
        if s["cls"] == "UNRESOLVED":
            errors.append("UNRESOLVED indirect call %s in %s (%s:%s) labels=%s; add a reviewed "
                          "'%s internal|external <reason>' contract entry or a model" %
                          (s["callee_text"], s["fn"], s["file"], s["line"], s["labels"], s["site_key"]))
    used_sites = {s["site_key"] for s in sites}
    for k, v in contract.items():
        if k.startswith("site:") and k not in used_sites:
            errors.append("STALE reviewed site %s (contract line %d): no such indirect call" % (k, v["line"]))

    # field expectations
    for k, v in contract.items():
        kind, key = k.split(":", 1)
        if kind != "field" or k not in public:
            continue
        has_addon = any(l[0] == "addon" for l in labels.get("F:" + key, ()))
        if v["role"] == "runtime" and has_addon:
            lab = next(l for l in labels["F:" + key] if l[0] == "addon")
            errors.append("CONTRACT VIOLATION %s is 'runtime' but may hold an addon callback: %s" %
                          (k, " -> ".join(path("F:" + key, lab))))
        if v["role"] in ("addon", "addon-input") and not has_addon:
            warnings.append("contract %s says addon, but no addon value reaches it" % k)

    # escapes and opaque stores of addon values
    for callee, i, tmp, site in escapes:
        labs = labels.get(tmp, set())
        if any(l[0] == "addon" for l in labs):
            warnings.append("addon callback escapes to external %s() arg %d at %s:%s (%s): invoked "
                            "outside the instrumented unit" % (callee, i, site["file"], site["line"], site["fn"]))
    for what, tmp, site in opaque:
        if any(l[0] == "addon" for l in labels.get(tmp, set())):
            errors.append("OPAQUE store of addon callback: %s at %s:%s (%s)" % (what, site["file"], site["line"], site["fn"]))

    # ------------------------------------------------------------ policy
    by_fn = collections.defaultdict(list)
    for s in sites:
        by_fn[s["fn"]].append(s)
    exempt_fns = sorted({s["fn"] for s in sites if s["cls"] == "addon" or s["cls"].startswith("external")})
    mixed = []
    for fn in exempt_fns:
        others = [s for s in by_fn[fn] if not (s["cls"] == "addon" or s["cls"].startswith("external"))]
        if others:
            mixed.append((fn, others))
            errors.append("MIXED dispatcher %s: exempting it also unprotects %s" %
                          (fn, ", ".join("%s (%s:%s, %s)" % (o["callee_text"], o["file"], o["line"], o["cls"]) for o in others)))

    def sym(fk):
        return fk.split("@", 1)[0]

    with open(os.path.join(a.out, "ignorelist.generated.txt"), "w") as f:
        f.write("#!special-case-list-v2\n# GENERATED by analyze.py from the libuv public API contract. Do not edit.\n"
                "# Pass only to the libuv target's cflags.\n[cfi-icall]\n")
        for fn in exempt_fns:
            why_ = sorted({s["cls"] + ":" + s["callee_text"] for s in by_fn[fn] if s["cls"] == "addon" or s["cls"].startswith("external")})
            # special-case lists have no trailing comments: comment on its own line
            f.write("# %s: %s\nfun:%s\n" % (funcs.get(fn, {}).get("file"), "; ".join(why_), sym(fn)))
    with open(os.path.join(a.out, "sites.json"), "w") as f:
        json.dump(sites, f, indent=1)

    t_total = time.time() - t0
    with open(os.path.join(a.out, "report.txt"), "w") as f:
        cnt = collections.Counter(s["cls"] for s in sites)
        f.write("libuv API-rooted CFI analysis\nTUs: %d  extract: %.2fs  total: %.2fs\n" % (len(tus), t_extract, t_total))
        f.write("public callback decls: %d  contract entries: %d\n" % (len(public), len(contract)))
        f.write("indirect call sites: %d  %s\n" % (len(sites), dict(cnt)))
        f.write("exempt functions: %d  mixed dispatchers: %d\n\n" % (len(exempt_fns), len(mixed)))
        f.write("== ERRORS (%d)\n" % len(errors))
        for e in errors:
            f.write("  " + e + "\n")
        f.write("\n== WARNINGS (%d)\n" % len(warnings))
        for w in sorted(set(warnings)):
            f.write("  " + w + "\n")
        f.write("\n== MIXED-PROVENANCE exempt sites (internal targets also lose the check here)\n")
        for s in sites:
            if (s["cls"] == "addon" or s["cls"].startswith("external")) and \
                    any(l[0].startswith("internal") for l in s["labels"]):
                f.write("  %s %s: %s\n" % (s["fn"], s["callee_text"], ", ".join(
                    l[1] for l in s["labels"] if l[0].startswith("internal"))))
        f.write("\n== SITES\n")
        for s in sites:
            f.write("%-22s %-34s %-28s %s:%s\n" % (s["cls"], s["fn"], s["callee_text"], s["file"], s["line"]))
            for p in s["provenance"]:
                if p["label"][0] in ("addon", "unknown") or s["cls"] != "internal":
                    f.write("      %s  via %s\n" % ("/".join(p["label"]), " -> ".join(p["path"][-6:])))
    print(open(os.path.join(a.out, "report.txt")).read().split("== SITES")[0])
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
