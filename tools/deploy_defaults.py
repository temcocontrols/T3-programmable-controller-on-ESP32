"""Push the committed default screens/images to a running device.

The firmware embeds DefaultScreens/*.json and seeds them into SPIFFS on the first
boot only, so an already-flashed unit keeps rendering whatever it was seeded
with.  This script stages the current repository copies through the deploy API
and commits them, which is the same path the T3000 front end uses.

    python tools/deploy_defaults.py --host 192.168.1.46
    python tools/deploy_defaults.py --screens wireguard_screen,ddns_screen

A commit publishes only the files staged in that transaction (begin wipes the
target slot), so a filtered run leaves a slot holding just those files: use it
to test one screen, then deploy everything again or POST /reset-defaults.
"""
import argparse
import hashlib
import json
import os
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

ROOT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                    "components", "temco_dynamic_display")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="192.168.1.46")
    ap.add_argument("--start-screen", default="start_up_screen")
    ap.add_argument("--schema", type=int, default=1)
    ap.add_argument("--screens", default="",
                    help="comma separated screen file stems, default all")
    ap.add_argument("--images", default="",
                    help="comma separated image file stems, default all")
    a = ap.parse_args()
    base = "http://" + a.host

    def call(method, path, body=None, timeout=180):
        r = urllib.request.Request(base + path, data=body, method=method)
        if body is not None:
            r.add_header("Content-Type", "application/json")
        r.add_header("Connection", "close")
        try:
            with urllib.request.urlopen(r, timeout=timeout) as x:
                return x.status, x.read().decode("utf-8", "replace")
        except urllib.error.HTTPError as e:
            return e.code, e.read().decode("utf-8", "replace")

    only = {}
    for area, spec in (("screens", a.screens), ("images", a.images)):
        if spec:
            only[area] = {s.strip() for s in spec.split(",") if s.strip()}
    # A commit only publishes the files staged in this transaction (the target
    # slot is wiped by begin), so naming one area means "only that area".
    if only:
        for area in ("screens", "images"):
            only.setdefault(area, set())

    def wanted(area, fn):
        return area not in only or fn[: -len(".json")] in only[area]

    files = [("screens", fn, os.path.join(ROOT, "DefaultScreens"), ".json")
             for fn in sorted(os.listdir(os.path.join(ROOT, "DefaultScreens")))
             if wanted("screens", fn)]
    files += [("images", fn, os.path.join(ROOT, "DefaultImages"), ".json")
              for fn in sorted(os.listdir(os.path.join(ROOT, "DefaultImages")))
              if wanted("images", fn)]

    print("status before:", *call("GET", "/api/eez-device/device/status"))
    st, txt = call("POST", "/api/eez-device/deploy/begin",
                   json.dumps({"schema": a.schema, "start_screen": a.start_screen}).encode())
    if st != 200:
        print("begin failed:", st, txt)
        return 1
    did = json.loads(txt)["deploy_id"]

    ok = bad = 0
    t0 = time.time()
    for area, fn, folder, ext in files:
        name = fn[: -len(ext)]
        payload = open(os.path.join(folder, fn), "rb").read()
        q = urllib.parse.urlencode({"path": area + "/" + name})
        st, txt = call("PUT", "/api/eez-device/deploy/%s/file?%s" % (did, q), payload)
        if st == 200 and json.loads(txt)["sha256"] == hashlib.sha256(payload).hexdigest():
            ok += 1
        else:
            bad += 1
            print("FAILED", area, name, st, txt)
    print("staged %d ok / %d failed in %.1fs" % (ok, bad, time.time() - t0))

    print("commit:", *call("POST", "/api/eez-device/deploy/%s/commit" % did))
    print("status after:", *call("GET", "/api/eez-device/device/status"))
    return 2 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
