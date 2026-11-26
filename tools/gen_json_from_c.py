"""Regenerate an EEZ/SquareLine-style screen JSON from a compiled `ui_<Screen>.c`.

The firmware's `ui_*.c` files are the source of truth (they are what runs on the
device), while `DefaultScreens/*.json` is what the front end loads and what the
JSON renderer draws.  This tool derives the JSON from the C file so the two can
never drift again.

Usage:
    python tools/gen_json_from_c.py StartUpScreen            # print to stdout
    python tools/gen_json_from_c.py StartUpScreen --write    # update DefaultScreens
    python tools/gen_json_from_c.py --all --check            # report drift
"""
import argparse
import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UI = os.path.join(ROOT, "main", "TemcoScreen")
OUT = os.path.join(ROOT, "components", "temco_dynamic_display", "DefaultScreens")

SCREEN_JSON = {
    "StartUpScreen": "start_up_screen",
    "HomeScreen": "home_screen",
    "MainMenu": "main_menu",
    "WifiConfig": "wifi_config",
    "NetworkConfig": "network_config",
    "Protocols": "protocols",
    "Parameters": "parameters",
    "Time": "time",
    "ScheduleScreen": "schedule_screen",
    "ScheduleEditScreen": "schedule_edit_screen",
    "HolidayCalenderScreen": "holiday_calender_screen",
    "WireGuardScreen": "wireguard_screen",
    "DdnsScreen": "ddns_screen",
}

# Screens with no static `ui_<Name> = lv_..._create()` statements: they are built
# procedurally with helper functions and local variables, so there is nothing to
# derive.  Their JSON stays hand-maintained.
PROCEDURAL = {
    "WireGuardScreen": "ui_RemoteAccess.c builds it with local variables",
    "DdnsScreen": "ui_RemoteAccess.c builds it with local variables",
}

SUBTYPE = {
    "obj": "panel",
    "label": "label",
    "button": "button",
    "image": "image",
    "img": "image",
    "imagebutton": "imagebutton",
    "textarea": "textarea",
    "switch": "switch",
    "checkbox": "checkbox",
    "dropdown": "dropdown",
    "roller": "roller",
    "arc": "arc",
    "calendar": "calendar",
    "keyboard": "keyboard",
}

ALIGN = {
    "LV_ALIGN_DEFAULT": "top_left",
    "LV_ALIGN_TOP_LEFT": "top_left",
    "LV_ALIGN_TOP_MID": "top_mid",
    "LV_ALIGN_TOP_RIGHT": "top_right",
    "LV_ALIGN_LEFT_MID": "left_mid",
    "LV_ALIGN_CENTER": "center",
    "LV_ALIGN_RIGHT_MID": "right_mid",
    "LV_ALIGN_BOTTOM_LEFT": "bottom_left",
    "LV_ALIGN_BOTTOM_MID": "bottom_mid",
    "LV_ALIGN_BOTTOM_RIGHT": "bottom_right",
    "LV_ALIGN_OUT_TOP_LEFT": "out_top_left",
    "LV_ALIGN_OUT_TOP_MID": "out_top_mid",
    "LV_ALIGN_OUT_TOP_RIGHT": "out_top_right",
    "LV_ALIGN_OUT_BOTTOM_LEFT": "out_bottom_left",
    "LV_ALIGN_OUT_BOTTOM_MID": "out_bottom_mid",
    "LV_ALIGN_OUT_BOTTOM_RIGHT": "out_bottom_right",
    "LV_ALIGN_OUT_LEFT_TOP": "out_left_top",
    "LV_ALIGN_OUT_LEFT_MID": "out_left_mid",
    "LV_ALIGN_OUT_LEFT_BOTTOM": "out_left_bottom",
    "LV_ALIGN_OUT_RIGHT_TOP": "out_right_top",
    "LV_ALIGN_OUT_RIGHT_MID": "out_right_mid",
    "LV_ALIGN_OUT_RIGHT_BOTTOM": "out_right_bottom",
}

PART = {
    "LV_PART_MAIN": "MAIN",
    "LV_PART_SCROLLBAR": "SCROLLBAR",
    "LV_PART_INDICATOR": "INDICATOR",
    "LV_PART_KNOB": "KNOB",
    "LV_PART_SELECTED": "SELECTED",
    "LV_PART_ITEMS": "ITEMS",
    "LV_PART_CURSOR": "CURSOR",
}

STATE = {
    "LV_STATE_DEFAULT": "DEFAULT",
    "LV_STATE_PRESSED": "PRESSED",
    "LV_STATE_CHECKED": "CHECKED",
    "LV_STATE_FOCUSED": "FOCUSED",
    "LV_STATE_EDITED": "EDITED",
    "LV_STATE_DISABLED": "DISABLED",
    "LV_STATE_HOVERED": "HOVERED",
    "LV_STATE_USER_1": "USER_1",
    "LV_STATE_USER_2": "USER_2",
    "LV_STATE_USER_3": "USER_3",
    "LV_STATE_USER_4": "USER_4",
    "LV_STATE_SCROLLED": "SCROLLED",
}

# LVGL value -> JSON value, per style property
VALUE_MAP = {
    "border_side": {
        "LV_BORDER_SIDE_NONE": "none",
        "LV_BORDER_SIDE_TOP": "top",
        "LV_BORDER_SIDE_BOTTOM": "bottom",
        "LV_BORDER_SIDE_LEFT": "left",
        "LV_BORDER_SIDE_RIGHT": "right",
        "LV_BORDER_SIDE_FULL": "full",
        "LV_BORDER_SIDE_INTERNAL": "internal",
    },
    "bg_grad_dir": {"LV_GRAD_DIR_NONE": "none", "LV_GRAD_DIR_HOR": "hor", "LV_GRAD_DIR_VER": "ver"},
    "text_align": {
        "LV_TEXT_ALIGN_AUTO": "auto",
        "LV_TEXT_ALIGN_LEFT": "left",
        "LV_TEXT_ALIGN_CENTER": "center",
        "LV_TEXT_ALIGN_RIGHT": "right",
    },
    "text_decor": {
        "LV_TEXT_DECOR_NONE": "none",
        "LV_TEXT_DECOR_UNDERLINE": "underline",
        "LV_TEXT_DECOR_STRIKETHROUGH": "strikethrough",
    },
    "flag_modify": {  # not a style, used for events
        "_UI_MODIFY_FLAG_ADD": "add",
        "_UI_MODIFY_FLAG_REMOVE": "remove",
        "_UI_MODIFY_FLAG_TOGGLE": "toggle",
    },
}

ANIM = {
    "LV_SCR_LOAD_ANIM_NONE": "NONE",
    "LV_SCR_LOAD_ANIM_OVER_LEFT": "OVER_LEFT",
    "LV_SCR_LOAD_ANIM_OVER_RIGHT": "OVER_RIGHT",
    "LV_SCR_LOAD_ANIM_OVER_TOP": "OVER_TOP",
    "LV_SCR_LOAD_ANIM_OVER_BOTTOM": "OVER_BOTTOM",
    "LV_SCR_LOAD_ANIM_MOVE_LEFT": "MOVE_LEFT",
    "LV_SCR_LOAD_ANIM_MOVE_RIGHT": "MOVE_RIGHT",
    "LV_SCR_LOAD_ANIM_MOVE_TOP": "MOVE_TOP",
    "LV_SCR_LOAD_ANIM_MOVE_BOTTOM": "MOVE_BOTTOM",
    "LV_SCR_LOAD_ANIM_FADE_IN": "FADE_IN",
    "LV_SCR_LOAD_ANIM_FADE_ON": "FADE_IN",
    "LV_SCR_LOAD_ANIM_FADE_OUT": "FADE_OUT",
    "LV_SCR_LOAD_ANIM_OUT_LEFT": "OUT_LEFT",
    "LV_SCR_LOAD_ANIM_OUT_RIGHT": "OUT_RIGHT",
    "LV_SCR_LOAD_ANIM_OUT_TOP": "OUT_TOP",
    "LV_SCR_LOAD_ANIM_OUT_BOTTOM": "OUT_BOTTOM",
}

EVENT = {
    "LV_EVENT_CLICKED": "CLICKED",
    "LV_EVENT_ALL": "ALL",
    "LV_EVENT_SCREEN_LOADED": "SCREEN_LOADED",
    "LV_EVENT_VALUE_CHANGED": "VALUE_CHANGED",
    "LV_EVENT_FOCUSED": "FOCUSED",
    "LV_EVENT_DEFOCUSED": "DEFOCUSED",
    "LV_EVENT_READY": "READY",
    "LV_EVENT_CANCEL": "CANCEL",
    "LV_EVENT_PRESSED": "PRESSED",
    "LV_EVENT_RELEASED": "RELEASED",
}

OPA = {"LV_OPA_TRANSP": 0, "LV_OPA_0": 0, "LV_OPA_COVER": 255, "LV_OPA_100": 255}


def num(v, default=0):
    v = v.strip()
    if v == "LV_SIZE_CONTENT":
        return 0
    m = re.match(r"^(-?\d+)", v)
    return int(m.group(1)) if m else default


def line_blocks(src):
    """Yield (var, kind, parent) for every ui_X = lv_kind_create(ui_Parent)."""
    for m in re.finditer(r"ui_([A-Za-z0-9_]+)\s*=\s*lv_([a-z_]+)_create\(\s*([^)]*)\)", src):
        yield m.group(1), m.group(2), m.group(3).strip()


def split_args(s):
    """Split an argument list on top-level commas."""
    out, cur, depth, q = [], [], 0, None
    for c in s:
        if q:
            cur.append(c)
            if c == q:
                q = None
            continue
        if c in "\"'":
            q = c
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif c == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
            continue
        cur.append(c)
    if cur or out:
        out.append("".join(cur).strip())
    return out


def scan_calls(region):
    """Yield (func, [args]) for every lv_* call, honouring nested parentheses."""
    for m in re.finditer(r"\blv_([a-z0-9_]+)\s*\(", region):
        i = m.end()
        depth = 1
        while i < len(region) and depth:
            if region[i] == "(":
                depth += 1
            elif region[i] == ")":
                depth -= 1
            i += 1
        yield m.group(1), split_args(region[m.end():i - 1])


def screen_region(src, screen):
    """The body of ui_<screen>_screen_init(), so event handlers never leak in."""
    head = "void ui_%s_screen_init(void)" % screen
    start = src.find(head)
    if start < 0:
        return src
    nxt = src.find("\nvoid ", start + len(head))
    return src[start:nxt if nxt > 0 else len(src)]


LOCAL_WIDGET = re.compile(
    r"lv_obj_t\s*\*\s*(\w+)\s*=\s*lv_([a-z_]+)_create\s*\(\s*ui_[A-Za-z0-9_]+\s*\)\s*;")


def alias_local_widgets(region):
    """Give widgets the design keeps in a local variable a `ui_` name.

    The compiled screens sometimes create a widget into a local, e.g.
    `lv_obj_t *label6 = lv_label_create(ui_Button6); lv_label_set_text(label6,
    "WireGuard");`.  Those are real design objects, so alias them as
    `ui_label6` and let the normal (name based) extraction pick up their text and
    style; otherwise the JSON silently loses them and the button renders blank.
    """
    for m in LOCAL_WIDGET.finditer(region):
        local = m.group(1)
        if local.startswith("ui_"):
            continue
        region = re.sub(r"\b%s\b" % re.escape(local), "ui_" + local, region)
    return region


_ASSET_DIMS = {}


def _load_asset_dims():
    """symbol -> (w, h) for every generated ui_img_* asset (several share one .c)."""
    if _ASSET_DIMS:
        return _ASSET_DIMS
    for path in glob.glob(os.path.join(UI, "ui_img_*.c")):
        c = open(path, encoding="utf-8", errors="replace").read()
        for m in re.finditer(r"lv_image_dsc_t\s+(ui_img_\w+)\s*=\s*\{(.*?)\n\};", c, re.S):
            body = m.group(2)
            mw = re.search(r"\.header\.w\s*=\s*(\d+)", body)
            mh = re.search(r"\.header\.h\s*=\s*(\d+)", body)
            if mw and mh:
                _ASSET_DIMS[m.group(1)] = (int(mw.group(1)), int(mh.group(1)))
    return _ASSET_DIMS


def img_dim(assets, sym):
    """Intrinsic w/h of a ui_img_* asset: generated C first, DefaultImages JSON second."""
    sym = sym.strip().lstrip("&")
    if sym not in assets:
        dims = _load_asset_dims().get(sym)
        if not dims:
            name = img_name(sym)
            path = os.path.join(os.path.dirname(OUT), "DefaultImages", name + ".json")
            if os.path.exists(path):
                d = json.load(open(path, encoding="utf-8"))
                dims = (int(d.get("width") or 0), int(d.get("height") or 0))
        assets[sym] = dims or (0, 0)
    return assets[sym]


def img_name(sym):
    """ui_img_fan_small_png -> fan_small"""
    s = sym.strip().lstrip("&")
    s = s.replace("ui_img_", "")
    s = re.sub(r"_png$", "", s)
    return s


def style_value(prop, raw):
    raw = raw.strip()
    if prop in VALUE_MAP:
        for k, v in VALUE_MAP[prop].items():
            if k in raw:
                return v
    if prop.endswith("_font"):
        f = raw.lstrip("&").strip()
        f = f.replace("ui_font_", "")
        return f
    if prop.endswith("_color"):
        m = re.search(r"0x([0-9A-Fa-f]{6})", raw)
        return "#" + m.group(1).upper() if m else raw
    if prop in ("opa",) or prop.endswith("_opa"):
        for k, v in OPA.items():
            if k in raw:
                return v
        return num(raw)
    if raw.startswith("LV_") or raw.startswith("&"):
        return raw
    return num(raw)


def handler_ifs(body):
    """Yield (condition, statement) for every `if (...)` in an event handler.

    The condition is matched by balancing parentheses, and the statement is
    either the braced block or the single statement up to its `;`, because
    SquareLine emits both forms (`if (...) { ... }` and
    `if (...) _ui_screen_change(...);` — see ui_event_Button6).
    """
    for bm in re.finditer(r"\bif\s*\(", body):
        i = bm.end() - 1
        depth, j = 0, i
        while j < len(body):
            if body[j] == "(":
                depth += 1
            elif body[j] == ")":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        cond = body[i + 1:j]
        k = j + 1
        while k < len(body) and body[k] in " \t\r\n":
            k += 1
        if k < len(body) and body[k] == "{":
            d, e2 = 0, k
            while e2 < len(body):
                if body[e2] == "{":
                    d += 1
                elif body[e2] == "}":
                    d -= 1
                    if d == 0:
                        break
                e2 += 1
            inner = body[k:e2 + 1]
        else:
            semi = body.find(";", k)
            inner = body[k:semi + 1 if semi >= 0 else len(body)]
        yield cond, inner


def parse(screen):
    path = os.path.join(UI, "ui_" + screen + ".c")
    src = open(path, encoding="utf-8", errors="replace").read()
    region = screen_region(src, screen)
    region = alias_local_widgets(region)

    # creation order + parents, and every call grouped by the object it targets
    objs = []
    calls = {}
    for fn, args in scan_calls(region):
        if args[0].startswith("ui_"):
            calls.setdefault(args[0][3:], []).append((fn, args))
    # Design objects are exactly the `ui_X = lv_kind_create(parent)` statements:
    # taking the name and the parent from the same statement keeps them paired
    # (pairing them by "first unnamed widget of this kind" crossed the two when a
    # local label sat between them, which silently re-parented later labels).
    for m in re.finditer(
            r"ui_([A-Za-z0-9_]+)\s*=\s*lv_([a-z_]+)_create\s*\(\s*([^()]*)\)", region):
        pm = re.match(r"ui_([A-Za-z0-9_]+)", m.group(3).strip())
        objs.append({"name": m.group(1), "kind": m.group(2),
                     "parent": pm.group(1) if pm else None})
    # widgets LVGL creates for us (calendar header) are not design objects
    objs = [o for o in objs if not o["name"].endswith("_header")]

    by_name = {o["name"]: o for o in objs}
    assets = {}
    for o in objs:
        # replay this object's own calls so the extractors below can stay regex-simple
        body = "".join("lv_%s(%s);\n" % (fn, ",".join(a)) for fn, a in calls.get(o["name"], []))
        node = {}
        node["sub_type"] = SUBTYPE.get(o["kind"], o["kind"])
        node["type"] = "Widget"
        node["text_type"] = "literal"

        # geometry (walk the calls in source order: lv_obj_align() overrides x/y).
        # LV_SIZE_CONTENT is emitted as 0 ("size comes from the content"); the
        # SquareLine `/// 34` comment next to it is only the editor's estimate of
        # the measured text/asset size, so it must not become a fixed size here.
        node["width"] = 0
        node["height"] = 0
        node["x_pos"] = 0
        node["y_pos"] = 0
        node["align"] = "top_left"
        for fn, a in calls.get(o["name"], []):
            if fn == "obj_set_width" and len(a) > 1:
                node["width"] = num(a[1])
            elif fn == "obj_set_height" and len(a) > 1:
                node["height"] = num(a[1])
            elif fn == "obj_set_size" and len(a) > 2:
                node["width"] = num(a[1])
                node["height"] = num(a[2])
            elif fn == "obj_set_x" and len(a) > 1:
                node["x_pos"] = num(a[1])
            elif fn == "obj_set_y" and len(a) > 1:
                node["y_pos"] = num(a[1])
            elif fn in ("obj_set_align", "obj_align") and len(a) > 1:
                node["align"] = ALIGN.get(a[1], "top_left")
                if fn == "obj_align" and len(a) > 3:
                    node["x_pos"] = num(a[2])
                    node["y_pos"] = num(a[3])
            elif fn == "obj_center":
                node["align"] = "center"
                node["x_pos"] = 0
                node["y_pos"] = 0
            elif fn == "obj_align_to" and len(a) > 4:
                node["align_to"] = {
                    "target": a[1].replace("ui_", "", 1),
                    "align": ALIGN.get(a[2], "top_left"),
                    "x_pos": num(a[3]),
                    "y_pos": num(a[4]),
                }
        node["obj_text"] = ""

        # text / images
        t = re.search(r"lv_label_set_text\(\s*ui_%s\s*,\s*\"((?:[^\"\\]|\\.)*)\"\s*\)" % o["name"], body)
        if t:
            node["obj_text"] = t.group(1)
        t = re.search(r"lv_checkbox_set_text\(\s*ui_%s\s*,\s*\"((?:[^\"\\]|\\.)*)\"\s*\)" % o["name"], body)
        if t:
            node["obj_text"] = t.group(1)
        ph = re.search(r"lv_textarea_set_placeholder_text\(\s*ui_%s\s*,\s*\"((?:[^\"\\]|\\.)*)\"\s*\)" % o["name"], body)
        if ph:
            node["placeholder"] = ph.group(1)
        t = re.search(r"lv_textarea_set_text\(\s*ui_%s\s*,\s*\"((?:[^\"\\]|\\.)*)\"\s*\)" % o["name"], body)
        if t:
            node["obj_text"] = t.group(1)
        if re.search(r"lv_textarea_set_one_line\(\s*ui_%s\s*,\s*true\)" % o["name"], body):
            node["one_line"] = True
        m = re.search(r"lv_label_set_long_mode\(\s*ui_%s\s*,\s*LV_LABEL_LONG_([A-Z_]+)\)" % o["name"], body)
        if m:
            node["long_mode"] = m.group(1)

        # keyboard wiring (the design names the field it edits and its keypad type)
        m = re.search(r"lv_keyboard_set_textarea\(\s*ui_%s\s*,\s*ui_([A-Za-z0-9_]+)\)" % o["name"], body)
        if m:
            node["textarea"] = m.group(1)
        m = re.search(r"lv_keyboard_set_mode\(\s*ui_%s\s*,\s*LV_KEYBOARD_MODE_([A-Z_]+)\)" % o["name"], body)
        if m:
            node["keyboard_mode"] = m.group(1).lower()

        m = re.search(r"lv_dropdown_set_dir\(\s*ui_%s\s*,\s*LV_DIR_([A-Z_]+)\)" % o["name"], body)
        if m:
            node["dir"] = m.group(1).lower()
        m = re.search(r"lv_roller_set_selected\(\s*ui_%s\s*,\s*(\d+)" % o["name"], body)
        if m:
            node["selected"] = int(m.group(1))
        # accepted_chars / max_length are not part of the screen schema the front end
        # round-trips; they are emitted into ui_json_events.c by gen_ui_json_events.py.

        src_img = re.search(r"lv_(?:image|img)_set_src\(\s*ui_%s\s*,\s*&?(ui_img_[A-Za-z0-9_]+)\s*\)" % o["name"], body)
        if src_img:
            node["src"] = img_name(src_img.group(1))
            iw, ih = img_dim(assets, src_img.group(1))
            # an image sized with LV_SIZE_CONTENT takes the asset's own size
            if not node["width"]:
                node["width"] = iw
            if not node["height"]:
                node["height"] = ih
        rel = re.search(r"lv_imagebutton_set_src\(\s*ui_%s\s*,\s*LV_IMAGEBUTTON_STATE_RELEASED[^,]*,\s*[^,]+,\s*&?(ui_img_[A-Za-z0-9_]+)" % o["name"], body)
        if rel:
            node["img_released"] = img_name(rel.group(1))
        prs = re.search(r"lv_imagebutton_set_src\(\s*ui_%s\s*,\s*LV_IMAGEBUTTON_STATE_PRESSED[^,]*,\s*[^,]+,\s*&?(ui_img_[A-Za-z0-9_]+)" % o["name"], body)
        if prs:
            node["img_pressed"] = img_name(prs.group(1))

        # dropdown / roller options
        opts = re.search(r"lv_dropdown_set_options\(\s*ui_%s\s*,\s*\"((?:[^\"\\]|\\.)*)\"\s*\)" % o["name"], body)
        if opts:
            node["options"] = [x for x in opts.group(1).replace("\\n", "\n").split("\n")]
        opts = re.search(r"lv_roller_set_options\(\s*ui_%s\s*,\s*\"((?:[^\"\\]|\\.)*)\"\s*,\s*LV_ROLLER_MODE_[A-Z]+\)" % o["name"], body)
        if opts:
            node["options"] = [x for x in opts.group(1).replace("\\n", "\n").split("\n")]

        # arc
        for key, macro in (("min", r"lv_arc_set_range\(\s*ui_%s\s*,\s*(-?\d+)"),
                           ("max", r"lv_arc_set_range\(\s*ui_%s\s*,\s*-?\d+\s*,\s*(-?\d+)")):
            m = re.search(macro % o["name"], body)
            if m:
                node[key] = int(m.group(1))
        m = re.search(r"lv_arc_set_value\(\s*ui_%s\s*,\s*(-?\d+)\)" % o["name"], body)
        if m:
            node["value"] = int(m.group(1))
        m = re.search(r"lv_arc_set_bg_angles\(\s*ui_%s\s*,\s*(-?\d+)\s*,\s*(-?\d+)\)" % o["name"], body)
        if m:
            node["bg_start_angle"] = int(m.group(1))
            node["bg_end_angle"] = int(m.group(2))
        m = re.search(r"lv_arc_set_rotation\(\s*ui_%s\s*,\s*(-?\d+)\)" % o["name"], body)
        if m:
            node["rotation"] = int(m.group(1))

        # calendar
        m = re.search(r"lv_calendar_set_today_date\(\s*ui_%s\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\)" % o["name"], body)
        if m:
            node["today_year"], node["today_month"], node["today_day"] = (int(m.group(i)) for i in (1, 2, 3))
        m = re.search(r"lv_calendar_set_showed_date\(\s*ui_%s\s*,\s*(\d+)\s*,\s*(\d+)\)" % o["name"], body)
        if m:
            node["shown_year"], node["shown_month"] = int(m.group(1)), int(m.group(2))
        if re.search(r"lv_calendar_header_arrow_create\(\s*ui_%s\s*\)" % o["name"], body):
            node["header"] = "Arrow"
        elif re.search(r"lv_calendar_header_dropdown_create\(\s*ui_%s\s*\)" % o["name"], body):
            node["header"] = "Dropdown"

        # flex
        m = re.search(r"lv_obj_set_flex_flow\(\s*ui_%s\s*,\s*([A-Z_]+)\)" % o["name"], body)
        if m:
            node["flex_flow"] = {
                "LV_FLEX_FLOW_ROW": "row",
                "LV_FLEX_FLOW_COLUMN": "column",
                "LV_FLEX_FLOW_ROW_WRAP": "row_wrap",
                "LV_FLEX_FLOW_COLUMN_WRAP": "column_wrap",
            }.get(m.group(1), "row")
        m = re.search(r"lv_obj_set_flex_align\(\s*ui_%s\s*,\s*([A-Z_]+)\s*,\s*([A-Z_]+)\s*,\s*([A-Z_]+)\)" % o["name"], body)
        if m:
            fa = {"LV_FLEX_ALIGN_START": "start", "LV_FLEX_ALIGN_END": "end",
                  "LV_FLEX_ALIGN_CENTER": "center", "LV_FLEX_ALIGN_SPACE_EVENLY": "space_evenly",
                  "LV_FLEX_ALIGN_SPACE_AROUND": "space_around", "LV_FLEX_ALIGN_SPACE_BETWEEN": "space_between"}
            node["flex_main"] = fa.get(m.group(1), "start")
            node["flex_cross"] = fa.get(m.group(2), "start")
            node["flex_track"] = fa.get(m.group(3), "start")

        # flags
        for fn, a in calls.get(o["name"], []):
            if fn in ("obj_add_flag", "obj_add_state") and len(a) > 1 and "LV_OBJ_FLAG_HIDDEN" in a[1]:
                node["hidden"] = True
            elif fn == "obj_remove_flag" and len(a) > 1:
                if "LV_OBJ_FLAG_SCROLLABLE" in a[1]:
                    node["scrollable"] = False
            elif fn == "obj_set_scrollbar_mode" and len(a) > 1:
                node["scrollbar_mode"] = a[1].replace("LV_SCROLLBAR_MODE_", "").lower()
            elif fn == "obj_set_scroll_snap_y" and len(a) > 1:
                node["scroll_snap_y"] = a[1].replace("LV_SCROLL_SNAP_", "").lower()
        m = re.search(r"lv_obj_set_scroll_dir\(\s*ui_%s\s*,\s*([A-Z_]+)\)" % o["name"], body)
        if m:
            node["scroll_dir"] = m.group(1).replace("LV_DIR_", "")

        # styles
        styles = {}
        for m in re.finditer(
                r"lv_obj_set_style_([a-z_]+)\(\s*ui_%s\s*,\s*(.+?)\s*,\s*([A-Z0-9_ |]+)\s*\);" % o["name"],
                body, re.S):
            prop, raw, sel = m.group(1), m.group(2), m.group(3)
            part = "MAIN"
            state = "DEFAULT"
            for tok in [t.strip() for t in sel.split("|")]:
                if tok in PART:
                    part = PART[tok]
                elif tok in STATE:
                    state = STATE[tok]
            styles.setdefault(part, {}).setdefault(state, {})[prop] = style_value(prop, raw)
        if styles:
            node["style"] = styles

        if "no_default_style" not in node and re.search(
                r"lv_obj_remove_style_all\(\s*ui_%s\s*\)" % o["name"], body):
            node["no_default_style"] = True

        o["json"] = node

    # -------- events --------
    events = {}
    for m in re.finditer(r"void\s+ui_event_([A-Za-z0-9_]+)\s*\(\s*lv_event_t\s*\*\s*e\s*\)\s*\{", src):
        name = m.group(1)
        # body = balanced braces
        i = m.end() - 1
        depth = 0
        j = i
        while j < len(src):
            if src[j] == "{":
                depth += 1
            elif src[j] == "}":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        body = src[i:j + 1]

        acts = {}
        # SquareLine writes both `if (...) { ... }` and the brace-less
        # `if (...) _ui_screen_change(...);` form (see ui_event_Button6), so both
        # have to be read or the button silently navigates nowhere.
        for cond, inner in handler_ifs(body):
            evs = [EVENT[c] for c in re.findall(r"LV_EVENT_[A-Z_]+", cond) if c in EVENT]
            if not evs:
                continue
            for ev in evs:
                acts.setdefault(ev, [])
                for sc in re.finditer(
                        r"_ui_screen_change\(\s*&?ui_([A-Za-z0-9_]+)\s*,\s*(LV_SCR_LOAD_ANIM_[A-Z_]+)\s*,\s*"
                        r"(-?\d+)\s*,\s*(-?\d+)", inner):
                    acts[ev].append({
                        "action": "screen_change",
                        "anim": ANIM.get(sc.group(2), "NONE"),
                        "delay": int(sc.group(4)),
                        "screen": SCREEN_JSON.get(sc.group(1), sc.group(1).lower()),
                        "speed": int(sc.group(3)),
                    })
                for fm in re.finditer(
                        r"_ui_flag_modify\(\s*ui_([A-Za-z0-9_]+)\s*,\s*LV_OBJ_FLAG_([A-Z_]+)\s*,\s*"
                        r"(_UI_MODIFY_FLAG_[A-Z]+)\s*\)", inner):
                    if fm.group(2) != "HIDDEN":
                        continue
                    acts[ev].append({
                        "action": "flag_modify",
                        "flag": "hidden",
                        "mode": VALUE_MAP["flag_modify"].get(fm.group(3), "toggle"),
                        "target": fm.group(1),
                    })
        acts = {k: v for k, v in acts.items() if v}
        if acts:
            events[name] = {k: {"actions": v} for k, v in acts.items()}

    # attach events by registration site
    for m in re.finditer(r"lv_obj_add_event_cb\(\s*ui_([A-Za-z0-9_]+)\s*,\s*ui_event_([A-Za-z0-9_]+)\s*,", src):
        obj, fn = m.group(1), m.group(2)
        if obj in by_name and fn in events:
            cur = by_name[obj]["json"].setdefault("events", {})
            for ev, body in events[fn].items():
                cur.setdefault(ev, {"actions": []})["actions"].extend(body["actions"])

    # -------- tree --------
    root = None
    kids = {}
    for o in objs:
        if o["parent"] is None:
            root = o
            continue
        kids.setdefault(o["parent"], []).append(o)

    def build(o):
        node = o["json"]
        children = kids.get(o["name"], [])
        if children:
            node["children"] = {c["name"]: build(c) for c in children}
        return node

    screen_node = build(root)
    screen_node["sub_type"] = "screen"

    # --- normalise to the schema the front end already consumes -------------
    # The root carries no align/scrollable, and its background lives in the
    # document-level bg_color rather than in a MAIN/DEFAULT style block.
    screen_node.pop("align", None)
    screen_node.pop("scrollable", None)
    screen_node.pop("clickable", None)
    screen_bg = ((screen_node.get("style") or {}).get("MAIN") or {}).get("DEFAULT", {}).get("bg_color")
    main_style = ((screen_node.get("style") or {}).get("MAIN") or {}).get("DEFAULT")
    if main_style is not None:
        main_style.pop("bg_color", None)
        main_style.pop("bg_opa", None)
        if not main_style:
            screen_node["style"]["MAIN"].pop("DEFAULT")
        if not screen_node["style"]["MAIN"]:
            screen_node["style"].pop("MAIN")
        if not screen_node["style"]:
            screen_node.pop("style")

    fonts = []
    seen = set()
    for o in objs:
        for part in (o["json"].get("style") or {}).values():
            for st in part.values():
                f = st.get("text_font")
                if f and f not in seen:
                    seen.add(f)
                    size = int(re.sub(r"\D", "", f) or 0)
                    fonts.append([f, size])

    bitmaps = []
    for o in objs:
        for key in ("src", "img_released", "img_pressed"):
            b = o["json"].get(key)
            if b and b not in bitmaps:
                bitmaps.append(b)

    doc = {
        "bg_color": screen_bg or "#000000",
        "bitmaps": bitmaps,
        "fonts": sorted(fonts, key=lambda x: x[0]),
        "widgets": {root["name"]: screen_node},
    }
    return doc


def apply_overrides(doc, screen_json):
    """Re-apply deliberately hand-maintained JSON that has no counterpart in the C.

    Home screen for example: the WiFi icon is inert in the compiled UI (the firmware
    only repaints it), but in JSON mode it must open the WiFi config screen, verified
    on hardware.  Such entries live in tools/json_overrides.json keyed by
    "<screen>/<slash separated path>" so regeneration never silently drops them.
    """
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "json_overrides.json")
    if not os.path.exists(path):
        return doc
    all_over = json.load(open(path, encoding="utf-8"))
    for key, value in all_over.get(screen_json, {}).items():
        if key.startswith("_"):
            continue
        node = doc
        parts = key.split("/")
        for p in parts[:-1]:
            node = node[p]
        node[parts[-1]] = value
    return doc


def canonical(o):
    # children key order is LVGL z-order, so it has to take part in the check
    return json.dumps(o, indent=2)


def diff_nodes(old, new, path="", out=None, limit=400):
    """Collect human-readable differences between two JSON trees."""
    if out is None:
        out = []
    if len(out) >= limit:
        return out
    if type(old) is not type(new) and not (isinstance(old, (int, float)) and isinstance(new, (int, float))):
        out.append("%s: type %s -> %s (%r -> %r)" % (path or "/", type(old).__name__, type(new).__name__, old, new))
        return out
    if isinstance(new, dict):
        for k in sorted(set(old) | set(new)):
            p = "%s/%s" % (path, k)
            if k not in old:
                out.append("+ %s = %s" % (p, json.dumps(new[k], sort_keys=True)))
            elif k not in new:
                out.append("- %s (was %s)" % (p, json.dumps(old[k], sort_keys=True)))
            else:
                diff_nodes(old[k], new[k], p, out, limit)
    elif isinstance(new, list):
        if canonical(old) != canonical(new):
            out.append("~ %s: %s -> %s" % (path or "/", json.dumps(old, sort_keys=True),
                                           json.dumps(new, sort_keys=True)))
    elif old != new:
        out.append("~ %s: %r -> %r" % (path or "/", old, new))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("screen", nargs="?")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--diff", action="store_true", help="show per-key differences with --check")
    a = ap.parse_args()

    screens = sorted(SCREEN_JSON) if a.all else [a.screen]
    rc = 0
    stale = []
    for s in screens:
        if not s:
            ap.error("screen name required")
        if s in PROCEDURAL:
            if a.all:
                print("%-28s skipped (%s)" % (SCREEN_JSON[s], PROCEDURAL[s]))
            else:
                ap.error("%s cannot be generated: %s" % (s, PROCEDURAL[s]))
            continue
        doc = parse(s)
        target = os.path.join(OUT, SCREEN_JSON[s] + ".json")
        doc = apply_overrides(doc, SCREEN_JSON[s])
        if a.write:
            with open(target, "w", encoding="utf-8", newline="\n") as f:
                # children order is LVGL z-order: never sort the keys
                json.dump(doc, f, indent=2, sort_keys=False)
                f.write("\n")
            print("wrote %s" % target)
            continue
        if a.check:
            old = json.load(open(target, encoding="utf-8"))
            if canonical(old) == canonical(doc):
                print("%-28s in sync" % SCREEN_JSON[s])
            else:
                rc = 1
                stale.append(SCREEN_JSON[s])
                print("%-28s DRIFT" % SCREEN_JSON[s])
                if a.diff:
                    for line in diff_nodes(old, doc):
                        print("      " + line)
            continue
        print(json.dumps(doc, indent=2, sort_keys=True))
    if stale:
        print("\n%d stale: %s" % (len(stale), ", ".join(stale)))
    return rc


if __name__ == "__main__":
    sys.exit(main())
