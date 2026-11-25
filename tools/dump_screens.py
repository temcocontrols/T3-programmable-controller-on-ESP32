"""Collect the renderer's layout dump for every default screen.

The JSON renderer (main/TemcoScreen/ui_json_render.c) prints one "layout dump"
block per built screen, 1200 ms after the screen is built and again at 3000 ms
(final geometry, after the design's own ShowItem_Animation).  The dumper is
compiled in but disabled, so this script enables it first through
POST /api/eez-device/ui-debug {"layout":true}.  It then forces a full UI reload,
navigates to every screen over the REST API while capturing the console
continuously, and splits the capture into one file per screen so the rendered
geometry can be diffed against DefaultScreens/*.json.

    python tools/dump_screens.py --port COM5 --host 192.168.1.46
"""
import argparse
import json
import os
import re
import sys
import threading
import time
import urllib.request

import serial  # pyserial, part of the ESP-IDF python env

SCREENS = [
    "start_up_screen", "home_screen", "main_menu", "network_config",
    "parameters", "protocols", "schedule_edit_screen", "schedule_screen",
    "time", "wifi_config", "holiday_calender_screen", "wireguard_screen",
    "ddns_screen",
]

HEAD = re.compile(r"---- (\w+): layout dump \((\d+) objects\) ----")
# The console is shared with other tasks, which occasionally interleaves a log
# line into the middle of a dump line, so only the geometry prefix is required
# and the padding is picked up separately when it survived.
OBJ = re.compile(
    r"(\S+)\s+(-?\d+),(-?\d+)\s+(\d+)x(\d+)\s+rel=(-?\d+),(-?\d+)"
    r"\s+sty=(-?\d+)x(-?\d+)"
)
PADS = re.compile(r"pad=(\d+),(\d+),(\d+),(\d+)")
BW = re.compile(r"bw=(-?\d+)")
ALIGN = re.compile(r"al=(-?\d+)")
HID = re.compile(r"hid=(\d+)")


def split_blocks(lines):
    """Group the console capture into {screen: [records]}.

    The renderer prints two dumps per build (1200 ms and 3000 ms after the
    build), and a later navigation prints them again.  The last value seen for a
    widget wins: the 3000 ms dump is the one taken once the design's own entry
    animations have finished.
    """
    blocks, cur = {}, None
    for line in lines:
        m = HEAD.search(line)
        if m:
            cur = m.group(1)
            blocks.setdefault(cur, {})
            continue
        if not cur:
            continue
        m = OBJ.search(line)
        if not m:
            continue
        name = m.group(1)
        p = PADS.search(line)
        b = BW.search(line)
        al = ALIGN.search(line)
        hd = HID.search(line)
        rec = {
            "name": name,
            "x": int(m.group(2)), "y": int(m.group(3)),
            "w": int(m.group(4)), "h": int(m.group(5)),
            "rx": int(m.group(6)), "ry": int(m.group(7)),
            "sw": int(m.group(8)), "sh": int(m.group(9)),
            "al": int(al.group(1)) if al else None,
            "hid": int(hd.group(1)) if hd else 0,
            "bw": int(b.group(1)) if b else None,
            "pl": int(p.group(1)) if p else None,
            "pt": int(p.group(2)) if p else None,
            "pr": int(p.group(3)) if p else None,
            "pb": int(p.group(4)) if p else None,
        }
        blocks[cur][name] = rec
    return {k: list(v.values()) for k, v in blocks.items() if v}


def post(host, path, payload, timeout=8):
    req = urllib.request.Request(
        "http://%s%s" % (host, path),
        data=json.dumps(payload).encode(),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return json.loads(r.read() or b"{}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="COM5")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--host", default="192.168.1.46")
    ap.add_argument("--out", default="build/screen_dumps")
    ap.add_argument("--settle", type=float, default=6.0,
                    help="seconds to wait after forcing a UI reload")
    ap.add_argument("--screens", default="", help="comma separated, default all")
    ap.add_argument("--resplit", default="",
                    help="rebuild the per-screen files from an existing all.log, "
                         "without touching the device")
    a = ap.parse_args()

    names = [s for s in a.screens.split(",") if s] or SCREENS
    os.makedirs(a.out, exist_ok=True)

    def write_blocks(lines):
        blocks = split_blocks(lines)
        for name, recs in sorted(blocks.items()):
            with open(os.path.join(a.out, name + ".jsonl"), "w", encoding="utf-8") as f:
                for r in recs:
                    f.write(json.dumps(r) + "\n")
            print("%-26s %3d objects" % (name, len(recs)), flush=True)
        missing = [s for s in names if s not in blocks]
        if missing:
            print("no dump for: %s" % ", ".join(missing))

    if a.resplit:
        with open(a.resplit, encoding="utf-8") as f:
            write_blocks(f.read().splitlines())
        return 0

    sp = serial.Serial(a.port, a.baud, timeout=0.2)
    sp.dtr = False
    sp.rts = False
    all_lines = []
    state = {"stop": False}

    def reader():
        while not state["stop"]:
            try:
                raw = sp.readline()
            except Exception:  # noqa: BLE001
                break
            if raw:
                all_lines.append(raw.decode("utf-8", "replace").rstrip())

    th = threading.Thread(target=reader, daemon=True)
    th.start()

    try:
        try:
            print("enabling the layout dump ...", flush=True)
            post(a.host, "/api/eez-device/ui-debug", {"layout": True})
        except Exception as e:  # noqa: BLE001
            print("could not enable the dump (%s); it must already be on" % e, flush=True)

        print("forcing a full UI reload ...", flush=True)
        post(a.host, "/api/eez-device/ui-mode", {"mode": "compiled"})
        time.sleep(1.5)
        post(a.host, "/api/eez-device/ui-mode", {"mode": "json"})
        time.sleep(a.settle)

        def dumped(name, mark):
            """Both dump blocks seen: the 2nd one is the final geometry."""
            return sum(1 for l in all_lines[mark:]
                       if (HEAD.search(l) or [None, None])[1] == name) >= 2

        for name in names:
            # wait for the previous screen to finish building: navigating while
            # a build is still running overlaps the two and can fault
            mark = len(all_lines)
            try:
                post(a.host, "/api/eez-device/ui-screen", {"name": name})
            except Exception as e:  # noqa: BLE001
                print("nav %-26s FAILED %s" % (name, e), flush=True)
                continue
            t0 = time.time()
            while time.time() - t0 < 25 and not dumped(name, mark):
                time.sleep(0.25)
            print("nav %-26s %s (%.1fs)" % (name, "ok" if dumped(name, mark) else "NO DUMP",
                                            time.time() - t0), flush=True)
    finally:
        state["stop"] = True
        time.sleep(0.4)
        sp.close()

    with open(os.path.join(a.out, "all.log"), "w", encoding="utf-8") as f:
        f.write("\n".join(all_lines))

    write_blocks(all_lines)
    return 0


if __name__ == "__main__":
    sys.exit(main())
