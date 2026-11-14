#!/usr/bin/env python3
"""The game's UI walk budget for a hud.element record, as Swofty measures it (SwoftyLauncher
packages/core/src/hud/walk.rs, launcher 1.9.49+).

The game flattens a screen's nodes into 16-bit-indexed slots: one per node, plus (n + 4) / 8 for a node with four or
more children; a tree holds at most 65,536. The HUD customisation screen holds the host scene six times, so a record
whose live root and editor copy take S slots costs about 6 * (S + 8) + 816 there; past 65,536 the game closes when
that screen opens (0.7.2 did, at 66,306). The node mapping is the composer's (scene.rs): a group, stack or centre is
one null (two with a motion holder), a sprite one quad, a text a holder with the text under it, a unit one node.
Checked against Swofty's own numbers for 0.7.2: 9,827 nodes and 10,806 slots, 403 nodes a row.

    python tools/walkcost.py <registry/tower.json>
"""
import json
import sys

SLOT_LIMIT = 65536
STACK_LIMIT = 256
EDITOR_INSTANCES, EDITOR_FIXED, HOST_EXTRA = 6, 816, 8
BUDGET = 60000          # our own ceiling for the editor tree: a margin under the game's limit


def own_slots(n):
    return 1 if n < 4 else 1 + (n + 4) // 8


def of(kids):
    n = len(kids)
    nodes = 1 + sum(k[0] for k in kids)
    slots = own_slots(n) + sum(k[1] for k in kids)
    stack = n
    for i, k in enumerate(kids):
        if k[2] > 0:
            stack = max(stack, n - i + k[2])
    return (nodes, slots, stack)


def expand(children):
    out = []
    for c in children:
        rep = c.get("repeat")
        out += [c] * (rep["count"] if rep else 1)
    return out


def lit(b, state):
    if not b or "view_model" not in b:
        return True
    v = bool(state.get(b["view_model"], False))
    return not v if b.get("not") else v


def cost(e, state=None):
    """state: the editor's preview leaves (the editor copy keeps only the elements lit by them, no repeats)."""
    if state is not None and (e.get("repeat") or not lit(e.get("visible"), state)):
        return None
    k = e.get("kind")
    if k in ("group", "stack", "centre"):
        kids = [c for c in (cost(x, state) for x in (e.get("children", []) if state is not None
                                                      else expand(e.get("children", [])))) if c]
        g = of(kids)
        return of([g]) if e.get("motion") and state is None else g
    if k == "sprite":
        return (1, 1, 0)
    if k == "text":
        return of([(1, 1, 0)])
    if k == "unit":
        return (1, 1, 0)
    raise SystemExit(f"walkcost: unknown element kind {k}")


def measure(path):
    rec = json.load(open(path, encoding="utf-8"))["hud.element"]["tower"]
    live = of([cost(e) for e in expand(rec["elements"])])
    state = (rec.get("editor") or {}).get("preview", {})
    pv = of([c for c in (cost(e, state) for e in rec["elements"]) if c])
    slots = live[1] + pv[1]
    editor = EDITOR_INSTANCES * (slots + HOST_EXTRA) + EDITOR_FIXED
    return {"nodes": live[0] + pv[0], "slots": slots, "stack": max(live[2], pv[2]), "editor": editor}


def main():
    m = measure(sys.argv[1])
    print(f"walk: {m['nodes']} nodes, {m['slots']} slots, stack {m['stack']} of {STACK_LIMIT}; "
          f"HUD customisation screen ~{m['editor']} of {SLOT_LIMIT} slots (our ceiling {BUDGET})")
    if m["editor"] > BUDGET or m["stack"] > STACK_LIMIT:
        sys.exit("walk: over budget: the game would close (or come close to closing) on the HUD customisation screen")


if __name__ == "__main__":
    main()
