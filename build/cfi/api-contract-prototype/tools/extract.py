"""Per-TU fact extraction from clang's JSON AST (C sources).

Facts emitted (all keys are strings):
  flows:     (src_term, dst_node, site)   src_term = ('fn',name)|('node',key)|('ret',fkey)|('unk',reason)
  icalls:    dict(fn, file, line, callee_text, terms)
  funcs:     {fkey: {file, line, static, params:[...]}}
  extcalls:  (callee, idx, src_term, site)  args passed to functions with no body here
  opaque:    (src_term, reason, site)       stores to locations we cannot name
  public:    candidate callback-bearing decls in public headers
"""
import json
import os
import re
import subprocess
import sys

FNPTR_RE = re.compile(r"\(\*[^)]*\)\s*\(")


def is_fnptr_type(t):
    if not t:
        return False
    q = t.get("desugaredQualType") or t.get("qualType", "")
    return bool(FNPTR_RE.search(q)) or bool(FNPTR_RE.search(t.get("qualType", "")))


class LocResolver:
    """clang's JSON dumper elides file/line when unchanged; replay in emission order."""

    def __init__(self):
        self.file = None
        self.line = None

    def bare(self, d):
        if "file" in d:
            self.file = d["file"]
        if "line" in d:
            self.line = d["line"]
        d["_f"] = self.file
        d["_l"] = self.line

    def loc(self, d):
        if not d:
            return
        if "spellingLoc" in d:
            self.bare(d["spellingLoc"])
            self.bare(d["expansionLoc"])
        else:
            self.bare(d)

    def walk(self, node):
        stack = [node]
        while stack:
            n = stack.pop()
            if not isinstance(n, dict):
                continue
            self.loc(n.get("loc"))
            r = n.get("range")
            if r:
                self.loc(r.get("begin"))
                self.loc(r.get("end"))
            inner = n.get("inner")
            if inner:
                stack.extend(reversed(inner))


def exp_loc(d):
    """file/line where a location appears in source (expansion location)."""
    if not d:
        return (None, None)
    if "expansionLoc" in d:
        d = d["expansionLoc"]
    return (d.get("_f"), d.get("_l"))


def spell_loc(d):
    if not d:
        return (None, None)
    if "spellingLoc" in d:
        d = d["spellingLoc"]
    return (d.get("_f"), d.get("_l"))


_macro_cache = {}


def macro_name_at(path, line):
    """Name of the #define whose body contains `line` (fields declared via macros)."""
    if path not in _macro_cache:
        try:
            with open(path) as f:
                _macro_cache[path] = f.read().split("\n")
        except OSError:
            _macro_cache[path] = []
    lines = _macro_cache[path]
    i = line - 1
    while i >= 0 and i < len(lines):
        m = re.match(r"\s*#\s*define\s+(\w+)", lines[i])
        if m:
            return m.group(1)
        if i < line - 1 and not lines[i].rstrip().endswith("\\"):
            return None
        i -= 1
    return None


class TU:
    def __init__(self, path, root, public_headers):
        self.path = path
        self.root = root
        self.public_headers = public_headers
        self.field_key = {}  # FieldDecl id -> key
        self.field_info = {}  # key -> dict
        self.anon_typedef = {}  # anonymous record id -> typedef name
        self.static_fns = {}  # name -> fkey
        self.flows = []
        self.icalls = []
        self.funcs = {}
        self.extcalls = []
        self.opaque = []
        self.args = []
        self.public = []
        self.globals = {}  # VarDecl id -> key
        self.defined = set()

    # ---------------------------------------------------------------- helpers
    def rel(self, f):
        if f is None:
            return None
        f = os.path.normpath(f if os.path.isabs(f) else os.path.join(self.root, f))
        if f.startswith(self.root + "/"):
            return f[len(self.root) + 1:]
        return f

    def in_project(self, f):
        r = self.rel(f)
        return r is not None and not r.startswith("/")

    def fkey(self, name):
        return self.static_fns.get(name, name)

    # ---------------------------------------------------------------- records
    def collect_records(self, tu):
        for n in tu.get("inner", []):
            if n.get("kind") == "TypedefDecl":
                for c in n.get("inner", []):
                    rid = self._typedef_record(c)
                    if rid:
                        self.anon_typedef.setdefault(rid, n["name"])
        for n in tu.get("inner", []):
            if n.get("kind") == "RecordDecl":
                self._record(n, None)
            elif n.get("kind") == "FunctionDecl" and n.get("inner"):
                stack = list(n["inner"])
                while stack:  # records declared inside function bodies
                    c = stack.pop()
                    if not isinstance(c, dict):
                        continue
                    if c.get("kind") == "RecordDecl":
                        self._record(c, n["name"])
                    elif c.get("inner"):
                        stack.extend(c["inner"])

    def _typedef_record(self, t):
        while t:
            if t.get("kind") == "RecordType":
                return t.get("decl", {}).get("id")
            inner = t.get("inner")
            t = inner[0] if inner else None
        return None

    def _record(self, rec, parent_name):
        name = rec.get("name") or self.anon_typedef.get(rec["id"])
        if not name:
            name = (parent_name or "?") + "::anon"
        is_union = rec.get("tagUsed") == "union"
        for c in rec.get("inner", []):
            k = c.get("kind")
            if k == "RecordDecl":
                self._record(c, name)
            elif k == "FieldDecl":
                fname = c.get("name") or "<anon>"
                sf, sl = spell_loc(c.get("loc"))
                ef, el = exp_loc(c.get("loc"))
                macro = None
                if sf and (sf, sl) != (ef, el):
                    macro = macro_name_at(self._abs(sf), sl)
                key = (macro or name) + "." + fname
                if is_union:
                    # union members alias: type punning must not erase provenance
                    key = (macro or name) + ".<union>"
                self.field_key[c["id"]] = key
                if key not in self.field_info:
                    self.field_info[key] = {
                        "record": name,
                        "macro": macro,
                        "type": c.get("type", {}).get("qualType"),
                        "fnptr": is_fnptr_type(c.get("type")),
                        "file": self.rel(sf or ef),
                        "line": sl or el,
                    }

    def _abs(self, f):
        return f if os.path.isabs(f) else os.path.join(self.root, f)

    # ---------------------------------------------------------------- terms
    def strip(self, e):
        while e and e.get("kind") in (
            "ImplicitCastExpr", "CStyleCastExpr", "ParenExpr", "ConstantExpr",
            "BuiltinBitCastExpr",
        ):
            e = (e.get("inner") or [None])[0]
        return e

    def text(self, e):
        e = self.strip(e)
        if not e:
            return "?"
        k = e["kind"]
        if k == "DeclRefExpr":
            return e["referencedDecl"].get("name", "?")
        if k == "MemberExpr":
            return self.text(e["inner"][0]) + ("->" if e.get("isArrow") else ".") + e["name"]
        if k == "ArraySubscriptExpr":
            return self.text(e["inner"][0]) + "[...]"
        if k == "UnaryOperator":
            return e["opcode"] + self.text(e["inner"][0])
        if k == "CallExpr":
            return self.text(e["inner"][0]) + "(...)"
        return "<" + k + ">"

    def node_of_decl(self, ref, ctx):
        kind = ref.get("kind")
        did = ref.get("id")
        if kind == "ParmVarDecl" and ctx and did in ctx["params"]:
            return "P:%s#%d" % (ctx["fkey"], ctx["params"][did])
        if kind in ("VarDecl", "ParmVarDecl"):
            if did in self.globals:
                return self.globals[did]
            if ctx:
                return "L:%s:%s#%s" % (ctx["fkey"], ref.get("name"), did)
            return "G:" + ref.get("name", "?")
        return None

    def terms(self, e, ctx):
        """Abstract sources of the value of expression e."""
        if e is None:
            return []
        k = e.get("kind")
        if k in ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr", "ConstantExpr",
                 "BuiltinBitCastExpr"):
            # Casts never erase provenance.
            return self.terms((e.get("inner") or [None])[0], ctx)
        if k == "DeclRefExpr":
            ref = e["referencedDecl"]
            if ref.get("kind") == "FunctionDecl":
                return [("fn", self.fkey(ref["name"]))]
            n = self.node_of_decl(ref, ctx)
            return [("node", n)] if n else []
        if k == "MemberExpr":
            key = self.field_key.get(e.get("referencedMemberDecl"))
            if key:
                return [("node", "F:" + key)]
            return [("unk", "member " + e.get("name", "?"))]
        if k in ("ConditionalOperator", "BinaryConditionalOperator"):
            inner = e.get("inner", [])
            out = []
            for c in inner[1:]:
                out += self.terms(c, ctx)
            return out
        if k == "BinaryOperator":
            op = e.get("opcode")
            if op == ",":
                return self.terms(e["inner"][1], ctx)
            if op == "=":
                return self.terms(e["inner"][1], ctx)
            if is_fnptr_type(e.get("type")):
                return [("unk", "binop " + op)]
            return []
        if k == "ArraySubscriptExpr":
            return self.terms(e["inner"][0], ctx)
        if k == "UnaryOperator":
            op = e.get("opcode")
            if op == "&":
                return self.terms(e["inner"][0], ctx)
            if op == "*" and is_fnptr_type(e.get("type")):
                return [("unk", "load through pointer " + self.text(e))]
            return []
        if k == "CallExpr":
            callee = self.strip(e["inner"][0])
            if callee and callee.get("kind") == "DeclRefExpr" and \
                    callee["referencedDecl"].get("kind") == "FunctionDecl":
                return [("ret", self.fkey(callee["referencedDecl"]["name"]))]
            if is_fnptr_type(e.get("type")):
                return [("unk", "result of indirect call")]
            return []
        if k in ("IntegerLiteral", "StringLiteral", "CharacterLiteral",
                 "FloatingLiteral", "UnaryExprOrTypeTraitExpr", "OffsetOfExpr"):
            return []
        if k == "AtomicExpr":
            # __c11_atomic_load(ptr): value comes from *ptr.
            if is_fnptr_type(e.get("type")) or "uintptr" in e.get("type", {}).get("qualType", "") \
                    or "void *" in e.get("type", {}).get("qualType", ""):
                return [("unk", "atomic load " + self.text(e["inner"][0]))]
            return []
        if is_fnptr_type(e.get("type")):
            return [("unk", "expr " + k)]
        return []

    def dest(self, e, ctx):
        e = self.strip(e)
        if not e:
            return None
        k = e["kind"]
        if k == "DeclRefExpr":
            return self.node_of_decl(e["referencedDecl"], ctx)
        if k == "MemberExpr":
            key = self.field_key.get(e.get("referencedMemberDecl"))
            return "F:" + key if key else None
        if k == "ArraySubscriptExpr":
            return self.dest(e["inner"][0], ctx)
        return None

    # ---------------------------------------------------------------- bodies
    def site(self, e, ctx):
        r = e.get("range", {}).get("begin")
        f, l = exp_loc(r)
        return {"fn": ctx["fkey"] if ctx else None, "file": self.rel(f), "line": l}

    def flow(self, terms, dst, site):
        for t in terms:
            self.flows.append((t, dst, site))

    def visit(self, n, ctx):
        stack = [n]
        while stack:
            e = stack.pop()
            if not isinstance(e, dict):
                continue
            k = e.get("kind")
            if k == "BinaryOperator" and e.get("opcode") == "=":
                lhs, rhs = e["inner"]
                d = self.dest(lhs, ctx)
                ts = self.terms(rhs, ctx)
                if d:
                    self.flow(ts, d, self.site(e, ctx))
                elif ts:
                    self.opaque.append((ts, "store to " + self.text(lhs), self.site(e, ctx)))
            elif k == "VarDecl" and e.get("init"):
                init = e["inner"][-1]
                d = self.node_of_decl(e, ctx)
                self.init(init, d, e.get("type"), ctx, self.site(e, ctx) if "range" in e else None)
            elif k == "CallExpr":
                callee = self.strip(e["inner"][0])
                args = e["inner"][1:]
                site = self.site(e, ctx)
                if callee and callee.get("kind") == "DeclRefExpr" and \
                        callee["referencedDecl"].get("kind") == "FunctionDecl":
                    cname = self.fkey(callee["referencedDecl"]["name"])
                    for i, a in enumerate(args):
                        ts = self.terms(a, ctx)
                        if ts:
                            self.args.append((cname, i, ts, site))
                else:
                    self.icalls.append(dict(site, callee_text=self.text(e["inner"][0]),
                                            terms=self.terms(e["inner"][0], ctx),
                                            callee_type=e["inner"][0].get("type", {}).get("qualType")))
            elif k == "ReturnStmt" and e.get("inner") and ctx:
                self.flow(self.terms(e["inner"][0], ctx), "R:" + ctx["fkey"], self.site(e, ctx))
            inner = e.get("inner")
            if inner:
                stack.extend(reversed(inner))

    def init(self, init, dst, typ, ctx, site):
        site = site or {"fn": ctx["fkey"] if ctx else None, "file": None, "line": None}
        e = self.strip(init)
        if e and e.get("kind") == "InitListExpr":
            # struct initializer: map positional inits to fields
            fields = self._fields_of_type(e.get("type"))
            inner = e.get("inner", [])
            if fields and len(fields) >= len(inner):
                for fk, sub in zip(fields, inner):
                    self.init(sub, "F:" + fk, None, ctx, site)
                return
            for sub in inner:  # arrays: every element flows into the array node
                self.init(sub, dst, None, ctx, site)
            return
        if dst:
            self.flow(self.terms(init, ctx), dst, site)

    def _fields_of_type(self, t):
        if not t:
            return None
        q = t.get("desugaredQualType") or t.get("qualType", "")
        m = re.match(r"(?:struct|union)\s+(\w+)$", q.strip())
        name = m.group(1) if m else q.strip()
        out = [k for k, v in self.field_info.items() if v["record"] == name]
        return out or None

    # ---------------------------------------------------------------- top level
    def run(self, tu):
        LocResolver().walk(tu)
        self.collect_records(tu)
        top = tu.get("inner", [])
        # static functions & globals
        for n in top:
            if n.get("kind") == "FunctionDecl" and n.get("storageClass") == "static":
                f, _ = spell_loc(n.get("loc"))
                self.static_fns[n["name"]] = "%s@%s" % (n["name"], os.path.basename(self.rel(f) or "?"))
        for n in top:
            if n.get("kind") == "VarDecl":
                f, _ = spell_loc(n.get("loc"))
                key = "G:" + n["name"]
                if n.get("storageClass") == "static":
                    key += "@" + os.path.basename(self.rel(f) or "?")
                self.globals[n["id"]] = key
        for n in top:
            k = n.get("kind")
            f, l = spell_loc(n.get("loc"))
            if f and f.startswith("<"):
                f, l = exp_loc(n.get("loc"))
            relf = self.rel(f)
            if k == "FunctionDecl":
                inner = n.get("inner", [])
                params = [p for p in inner if p.get("kind") == "ParmVarDecl"]
                body = [c for c in inner if c.get("kind") == "CompoundStmt"]
                fk = self.fkey(n["name"])
                if relf in self.public_headers:
                    for i, p in enumerate(params):
                        if is_fnptr_type(p.get("type")):
                            self.public.append({"kind": "param", "key": "%s.%s" % (n["name"], p.get("name", "#%d" % i)),
                                                "func": n["name"], "idx": i, "file": relf, "line": l,
                                                "type": p["type"].get("qualType")})
                if body and self.in_project(f):
                    ctx = {"fkey": fk, "params": {p["id"]: i for i, p in enumerate(params)}}
                    self.defined.add(fk)
                    self.funcs[fk] = {"file": relf, "line": l, "static": n.get("storageClass") == "static",
                                      "params": [p.get("name") for p in params]}
                    self.visit(body[0], ctx)
            elif k == "VarDecl" and n.get("init") and self.in_project(f):
                self.init(n["inner"][-1], self.globals[n["id"]], n.get("type"), None,
                          {"fn": None, "file": relf, "line": l})
            elif k == "TypedefDecl" and relf in self.public_headers and is_fnptr_type(n.get("type")):
                self.public.append({"kind": "typedef", "key": n["name"], "file": relf, "line": l,
                                    "type": n["type"].get("qualType")})
        for key, info in self.field_info.items():
            if info["fnptr"] and info["file"] in self.public_headers:
                self.public.append({"kind": "field", "key": key, "file": info["file"],
                                    "line": info["line"], "type": info["type"]})
        return {
            "tu": self.rel(self.path),
            "flows": self.flows,
            "args": self.args,
            "icalls": self.icalls,
            "funcs": self.funcs,
            "opaque": self.opaque,
            "public": self.public,
            "fields": {k: v for k, v in self.field_info.items() if v["fnptr"]},
        }


def extract(args):
    path, root, cflags, public_headers = args
    out = subprocess.run(
        ["clang", "-fsyntax-only", "-Xclang", "-ast-dump=json"] + cflags + [path],
        cwd=root, capture_output=True, text=True)
    if out.returncode != 0:
        raise RuntimeError("clang failed on %s:\n%s" % (path, out.stderr[-4000:]))
    tu = json.loads(out.stdout)
    return TU(path, root, set(public_headers)).run(tu)


if __name__ == "__main__":
    r = extract((sys.argv[1], sys.argv[2], sys.argv[3:], []))
    print(json.dumps(r, indent=1)[:5000])
