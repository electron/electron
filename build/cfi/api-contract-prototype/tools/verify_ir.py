"""Step 4: verify the emitted cfi-icall checks against the AST classification.

usage: python3 -I verify_ir.py <sites.json> <ignorelist> <ll files...>
Checks, per source function (frontend IR, -O0, so no inlining):
  * exempt (fun:) functions emit no llvm.type.test
  * every function with a non-exempt indirect call emits >= that many type tests
  * each internal target's !type ids contain the type id tested at its site
    (a mismatch traps at runtime even though provenance is internal)
"""
import collections
import json
import re
import sys

DEF_RE = re.compile(r"^define [^@]*@\"?([\w.$]+)\"?\(.*?\)(.*)\{\s*$")
DECL_RE = re.compile(r"^declare ((?:!type !\d+ )*)[^@]*@\"?([\w.$]+)\"?\(")
TYPEMD_RE = re.compile(r"^!(\d+) = !\{i64 0, !\"([^\"]+)\"\}")
TEST_RE = re.compile(r"llvm\.type\.test\(ptr %[\w.]+, metadata !\"([^\"]+)\"\)")


def parse(files):
    tests = collections.defaultdict(list)   # fn -> [type ids tested]
    fn_types = collections.defaultdict(set)  # fn -> {type ids}
    for path in files:
        md = {}
        defs = []
        decls = []
        cur = None
        for line in open(path):
            m = TYPEMD_RE.match(line)
            if m:
                md[m.group(1)] = m.group(2)
                continue
            m = DEF_RE.match(line)
            if m:
                cur = m.group(1)
                defs.append((cur, re.findall(r"!type !(\d+)", m.group(2))))
                continue
            m = DECL_RE.match(line)
            if m:
                decls.append((m.group(2), re.findall(r"!(\d+)", m.group(1))))
                continue
            if line.startswith("}"):
                cur = None
            elif cur:
                for t in TEST_RE.findall(line):
                    tests[cur].append(t)
        for name, ids in defs + decls:
            for i in ids:
                if i in md:
                    fn_types[name].add(md[i])
    return tests, fn_types


def main():
    sites = json.load(open(sys.argv[1]))
    exempt = {l.strip()[4:] for l in open(sys.argv[2]) if l.startswith("fun:")}
    tests, fn_types = parse(sys.argv[3:])
    errors = []
    by_fn = collections.defaultdict(list)
    for s in sites:
        by_fn[s["fn"].split("@")[0]].append(s)
    rows = []
    for fn, ss in sorted(by_fn.items()):
        n_tests = len(tests.get(fn, []))
        n_sites = len(ss)
        if fn in exempt:
            ok = n_tests == 0
            if not ok:
                errors.append("%s is exempt but still emits %d type tests" % (fn, n_tests))
        else:
            ok = n_tests >= n_sites
            if not ok:
                errors.append("%s has %d protected sites but only %d type tests" % (fn, n_sites, n_tests))
            tested = set(tests.get(fn, []))
            for s in ss:
                for lab in s["labels"]:
                    if lab[0] not in ("internal", "internal-ext"):
                        continue
                    tgt = lab[1].split("@")[0]
                    have = fn_types.get(tgt, set())
                    if have and not (have & tested):
                        errors.append("TYPE MISMATCH at %s (%s:%s): calls %s as %s but %s has type ids %s; "
                                      "this protected call traps" % (fn, s["file"], s["line"], tgt,
                                                                     "/".join(sorted(tested)), tgt, sorted(have)))
                        ok = False
                    elif not have:
                        errors.append("no !type for internal target %s of %s (not address-taken in this unit?)" % (tgt, fn))
        rows.append((fn, "exempt" if fn in exempt else "checked", n_sites, n_tests, "ok" if ok else "FAIL"))
    print("%-30s %-8s %5s %6s" % ("function", "policy", "sites", "tests"))
    for r in rows:
        print("%-30s %-8s %5d %6d  %s" % r)
    print("\nERRORS (%d)" % len(errors))
    for e in errors:
        print("  " + e)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
