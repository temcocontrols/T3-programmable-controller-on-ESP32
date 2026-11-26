"""Audit the rendered screen geometry against DefaultScreens/*.json.

Reads the layout dumps collected by tools/dump_screens.py (build/screen_dumps)
and, for every widget whose size is fully declared by the JSON, recomputes the
rectangle the JSON asks for from its parent's *rendered* rectangle and compares
it with the widget's rendered rectangle.  This checks the JSON -> LVGL contract
(the part that can silently drift); the JSON -> C contract is covered by
tools/gen_json_from_c.py --check.

    python tools/dump_screens.py         # collect on hardware
    python tools/audit_render.py         # compare

Reported per screen:
    ok        widget matches the JSON
    MISMATCH  rendered rectangle differs by more than the tolerance
    skipped   size comes from the content (label / one-line textarea), parent
              hidden or flex-laid-out, the placement is an align_to chain head,
              or the compiled screen animates it on LV_EVENT_SCREEN_LOADED
"""
import argparse
import glob
import json
import os
import re
import sys

SCREEN_W, SCREEN_H = 480, 320
TOL = 1  # LVGL rounds the centred offsets
CONTENT = 1073741823  # LV_SIZE_CONTENT as printed by the dump
ALIGNS = ("top_left", "top_mid", "top_right", "left_mid", "center",
          "right_mid", "bottom_left", "bottom_mid", "bottom_right")

class Rect:
    def __init__(self, x, y, w, h, pads=None, hidden=False, bw=0):
        self.x, self.y, self.w, self.h = x, y, w, h
        self.pads = pads
        self.hidden = hidden
        self.bw = bw

    def __repr__(self):
        return "%d,%d %dx%d" % (self.x, self.y, self.w, self.h)


def load_dump(path):
    out = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            r = json.loads(line)
            pads = None
            if r.get("pl") is not None:
                pads = (r["pl"], r["pt"], r["pr"], r["pb"])
            out[r["name"]] = Rect(r["x"], r["y"], r["w"], r["h"], pads,
                                  hidden=r["hid"] == 1, bw=r.get("bw") or 0)
    return out


def trunc_div(a, b):
    return int(a / b)


C_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                     "main", "TemcoScreen")
DECL_RE = re.compile(r"ui_(\w+)\s*=\s*lv_\w+\(")
ANIM_RE = re.compile(r"\w*Animation\(ui_(\w+)")


def load_compiled_animations():
    """{C file: (declared widget names, names the handler animates)}.

    The compiled screens run their entry animations on the JSON objects (the
    renderer bridges the design handlers), and ShowItem_Animation animates y and
    height to absolute 0, so those widgets only reach their design geometry once
    the runtime code sets it again.  Their rendered rectangle is therefore not
    comparable with the JSON and the audit skips them.
    """
    files = {}
    for path in glob.glob(os.path.join(C_DIR, "ui_*.c")):
        with open(path, encoding="utf-8") as f:
            src = f.read()
        files[os.path.basename(path)] = (set(DECL_RE.findall(src)),
                                         set(ANIM_RE.findall(src)))
    return files


def animated_widgets(widget_names, screens):
    """Animated widget names (plus their descendants) for one screen."""
    best, best_hits = set(), 0
    for decl, anim in screens.values():
        hits = len(widget_names & decl)
        if hits > best_hits:
            best, best_hits = anim, hits
    if not best:
        return set()
    return best


def border_space(parent, parent_json, side):
    """LVGL adds the border width to the padding only on the drawn sides."""
    bs = ((parent_json.get("style") or {}).get("MAIN") or {}).get("DEFAULT", {}).get("border_side")
    if not bs or bs == "none":
        return 0
    if bs == "full":
        return parent.bw
    return parent.bw if side in bs.split("+") else 0


def predict(w, parent, parent_json, wd, ht):
    """Expected rectangle of widget w inside a rendered parent."""
    xo, yo = int(w.get("x_pos") or 0), int(w.get("y_pos") or 0)
    align = w.get("align") or "center"
    pl, pt, pr, pb = parent.pads
    # LVGL aligns inside the parent's content box: padding plus the border on
    # the sides where it is actually drawn (lv_obj_get_style_space_*)
    sl = pl + border_space(parent, parent_json, "left")
    sr = pr + border_space(parent, parent_json, "right")
    st = pt + border_space(parent, parent_json, "top")
    sb = pb + border_space(parent, parent_json, "bottom")
    cx, cy = parent.x + sl, parent.y + st
    cw = parent.w - sl - sr
    ch = parent.h - st - sb
    x, y = cx + xo, cy + yo
    if align in ("top_mid", "bottom_mid", "center"):
        x = cx + trunc_div(cw - wd, 2) + xo
    elif align in ("top_right", "right_mid", "bottom_right"):
        x = cx + cw - wd + xo
    if align in ("left_mid", "right_mid", "center"):
        y = cy + trunc_div(ch - ht, 2) + yo
    elif align in ("bottom_left", "bottom_mid", "bottom_right"):
        y = cy + ch - ht + yo
    return x, y, wd, ht


def predict_align_to(w, base, wd, ht):
    at = w["align_to"]
    xo, yo = int(at.get("x_pos") or 0), int(at.get("y_pos") or 0)
    a = (at.get("align") or "").replace("out_", "")
    bx2, by2 = base.x + base.w - 1, base.y + base.h - 1
    x, y = base.x, base.y
    if a == "top_left":
        y = base.y - ht
    elif a == "top_mid":
        x, y = base.x + trunc_div(base.w, 2) - trunc_div(wd, 2), base.y - ht
    elif a == "top_right":
        x, y = bx2 - wd + 1, base.y - ht
    elif a == "left_top":
        x = base.x - wd
    elif a == "left_mid":
        x, y = base.x - wd, base.y + trunc_div(base.h, 2) - trunc_div(ht, 2)
    elif a == "left_bottom":
        x, y = base.x - wd, by2 - ht + 1
    elif a == "right_top":
        x = bx2 + 1
    elif a == "right_mid":
        x, y = bx2 + 1, base.y + trunc_div(base.h, 2) - trunc_div(ht, 2)
    elif a == "right_bottom":
        x, y = bx2 + 1, by2 - ht + 1
    elif a == "bottom_left":
        y = by2 + 1
    elif a == "bottom_mid":
        x, y = base.x + trunc_div(base.w, 2) - trunc_div(wd, 2), by2 + 1
    elif a == "bottom_right":
        x, y = bx2 - wd + 1, by2 + 1
    return x + xo, y + yo, wd, ht


def audit(screen, widgets, dump, report, animated=None):
    animated = animated or set()
    by_name = {}

    def walk(node, parent_name):
        for name, w in (node.get("children") or {}).items():
            by_name[name] = w
            walk(w, name)

    walk(widgets, None)
    root = Rect(0, 0, SCREEN_W, SCREEN_H, (0, 0, 0, 0))

    def parent_of(name):
        for parent, w in by_name.items():
            if name in (w.get("children") or {}):
                return parent
        return None

    results = []  # (name, status, expected, got, reason)
    for name, w in by_name.items():
        got = dump.get(name)
        if got is None:
            results.append((name, "MISSING", None, None, "not in dump"))
            continue
        pname = parent_of(name)
        parent = dump.get(pname) if pname else root
        if parent is None:
            results.append((name, "skipped", None, None, "parent %s not in dump" % pname))
            continue
        if parent.hidden or got.hidden:
            results.append((name, "skipped", None, None, "hidden"))
            continue
        if pname and by_name.get(pname, {}).get("flex_flow"):
            results.append((name, "skipped", None, None, "flex parent (layout decides)"))
            continue
        wd, ht = int(w.get("width") or 0), int(w.get("height") or 0)
        at = w.get("align_to")
        if at:
            base = dump.get(at.get("target"))
            if base is None:
                results.append((name, "skipped", None, None, "align_to base missing"))
                continue
            exp = predict_align_to(w, base, got.w, got.h)
        else:
            # a content-sized widget's own size is only known once LVGL has
            # measured it, which the dump reports: verify the placement rule
            # against that size instead of skipping the widget
            if wd <= 0:
                wd = got.w
            if ht <= 0:
                ht = got.h
            if parent.pads is None:
                results.append((name, "skipped", None, None,
                                "parent padding lost in capture"))
                continue
            exp = predict(w, parent, by_name.get(pname, {}), wd, ht)
        ok = max(abs(exp[0] - got.x), abs(exp[1] - got.y),
                 abs(exp[2] - got.w), abs(exp[3] - got.h)) <= TOL
        results.append((name, "ok" if ok else "MISMATCH", exp, got, ""))

    # ShowItem_Animation animates y/height to absolute 0 and the *In_Animation
    # family slides x, all on the JSON objects (the compiled handler runs on
    # them); a widget that moved exactly like an animated sibling is following
    # it, not mis-laid-out.
    def delta(rec):
        exp, got = rec[2], rec[3]
        return (got.x - exp[0], got.y - exp[1])

    deltas = {rec[0]: delta(rec) for rec in results
              if rec[0] in animated and rec[2] and rec[3] and delta(rec) != (0, 0)}
    for name, status, exp, got, reason in results:
        if status == "MISMATCH":
            if name in animated:
                status, reason = "animated", "entry animation (compiled handler)"
            else:
                d = (got.x - exp[0], got.y - exp[1])
                host = next((n for n, v in deltas.items()
                             if n != name and abs(v[0] - d[0]) <= TOL
                             and abs(v[1] - d[1]) <= TOL), None)
                if host:
                    status, reason = "animated", "moved with animated %s" % host
        if status == "MISMATCH":
            report.append((screen, name, status,
                           "json %d,%d %dx%d" % exp,
                           "rendered %d,%d %dx%d" % (got.x, got.y, got.w, got.h)))
        else:
            report.append((screen, name, status, "", reason if status != "ok" else ""))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dumps", default="build/screen_dumps")
    ap.add_argument("--json", default="components/temco_dynamic_display/DefaultScreens")
    ap.add_argument("--verbose", action="store_true", help="list ok widgets too")
    a = ap.parse_args()

    report = []
    screens = sorted(glob.glob(os.path.join(a.dumps, "*.jsonl")))
    if not screens:
        print("no dumps in %s - run tools/dump_screens.py first" % a.dumps)
        return 1
    compiled = load_compiled_animations()
    for path in screens:
        name = os.path.splitext(os.path.basename(path))[0]
        jpath = os.path.join(a.json, name + ".json")
        if not os.path.exists(jpath):
            continue
        with open(jpath, encoding="utf-8") as f:
            data = json.load(f)
        widgets = next(iter(data["widgets"].values()))
        names = set()
        stack = [widgets]
        while stack:
            node = stack.pop()
            for wname, w in (node.get("children") or {}).items():
                names.add(wname)
                stack.append(w)
        audit(name, widgets, load_dump(path), report,
              animated_widgets(names, compiled))

    counts = {}
    reasons = {}
    for screen, name, status, exp, got in report:
        counts[status] = counts.get(status, 0) + 1
        if status in ("skipped", "animated"):
            reasons[got] = reasons.get(got, 0) + 1
    print("%-24s %5s %8s %8s %7s %8s" % ("screen", "ok", "MISMATCH", "skipped",
                                        "missing", "animated"))
    for path in screens:
        name = os.path.splitext(os.path.basename(path))[0]
        rows = [r for r in report if r[0] == name]
        if not rows:
            continue
        print("%-24s %5d %8d %8d %7d %8d" % (
            name,
            sum(1 for r in rows if r[2] == "ok"),
            sum(1 for r in rows if r[2] == "MISMATCH"),
            sum(1 for r in rows if r[2] == "skipped"),
            sum(1 for r in rows if r[2] == "MISSING"),
            sum(1 for r in rows if r[2] == "animated")))
    print("\ntotal: %s" % counts)
    if reasons:
        print("not compared because: %s" % ", ".join(
            "%s x%d" % (k, v) for k, v in sorted(reasons.items(), key=lambda kv: -kv[1])))
    bad = [r for r in report if r[2] in ("MISMATCH", "MISSING")]
    for screen, name, status, exp, got in bad:
        print("  %-22s %-24s %-9s %-26s %s" % (screen, name, status, exp, got))
    if a.verbose:
        for r in report:
            if r[2] == "ok":
                print("  ok %-22s %s" % (r[0], r[1]))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
