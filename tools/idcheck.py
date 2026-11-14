#!/usr/bin/env python3
"""Element id clashes in a hud.element record, as Swofty's composer computes them (SwoftyLauncher
packages/core/src/hud: model.rs expand, scene.rs element, ui/idf.rs recompute_ids): every scene node's elementId is
djb2 of its dotted path, and a scene with two equal ids is refused ("an element id is used twice"). A 32-bit hash of
~10,000 similar names can clash by chance; this finds the clashing pair before the composer does.

    python tools/idcheck.py <registry/tower.json>
"""
import json
import sys

HOST = "boundless_customisation"   # the stock node the composer hangs a record from (dials.rs CUSTOMISATION)


def djb2(s):
    h = 5381
    for c in s.encode():
        h = (h * 33 + c) & 0xFFFFFFFF
    return h


def lit(b, state):
    if not b or "view_model" not in b:
        return True
    v = bool(state.get(b["view_model"], False))
    return not v if b.get("not") else v


def expand(e, prefix, rows, depth=0):
    """model.rs expand_into: copy i of repeat group g is gNN, its subtree's ids start with gNN_"""
    r = e.get("repeat")
    if not r:
        c = dict(e, id=prefix + e["id"])
        c["children"] = [x for ch in e.get("children", []) for x in expand(ch, prefix, rows, depth)]
        return [c]
    count = r["count"] if not (rows and depth == 0) else min(rows, r["count"])
    out = []
    for k in range(count):
        i = r.get("from", 0) + k
        cid = f"{prefix}{e['id']}{i:02d}"
        c = dict(e, id=cid, repeat=None)
        c["children"] = [x for ch in e.get("children", []) for x in expand(ch, cid + "_", rows, depth + 1)]
        out.append(c)
    return out


def nodes(e, names, path, out, state, preview):
    """scene.rs element: (name, path) of every node an element makes"""
    if preview and not lit(e.get("visible"), state):
        return
    name = names + e["id"]
    k = e["kind"]
    p = f"{path}.{name}"
    if k in ("group", "stack", "centre"):
        out.append(p)
        inner = p
        if e.get("motion") and not preview:
            inner = f"{p}.zh_{name}_{e['motion']['id']}"
            out.append(inner)
        for c in e.get("children", []):
            nodes(c, names, inner, out, state, preview)
    elif k == "text":
        out.append(f"{p}_n")
        out.append(f"{p}_n.{name}")
    else:  # sprite, unit
        out.append(p)


def paths(rec):
    pfx = rec["prefix"]
    state = (rec.get("editor") or {}).get("preview", {})
    rows = (rec.get("editor") or {}).get("rows")
    out = []
    for root, preview in ((pfx, False), (f"{pfx}_pv", True)):
        base = f"{HOST}.{root}"
        out.append(base)
        for e in rec["elements"]:
            for x in expand(e, "", rows if preview else None):
                nodes(x, root + "_", base, out, state, preview)
    return out


def clashes(rec):
    seen, bad = {}, []
    for p in paths(rec):
        h = djb2(p)
        if h in seen and seen[h] != p:
            bad.append((seen[h], p))
        seen.setdefault(h, p)
    return bad


def main():
    rec = json.load(open(sys.argv[1], encoding="utf-8"))["hud.element"]["tower"]
    bad = clashes(rec)
    for a, b in bad:
        print(f"idcheck: {a} and {b} hash to the same element id")
    print(f"idcheck: {len(paths(rec))} scene nodes, {len(bad)} id clashes")
    if bad:
        sys.exit("idcheck: rename one of each pair (the composer refuses a scene with an id used twice)")


if __name__ == "__main__":
    main()
