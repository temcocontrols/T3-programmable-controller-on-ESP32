"""Wait for the ESP32-S3 bootloader, then flash temco_app.bin and boot it.

The board has no DTR/RTS wiring, so it only answers in download mode (J4+J6
shorted while powering on, J4 removed afterwards).  多 The window is short, so
poll instead of asking a human to time it.

    python tools/flash_and_run.py [--port COM5] [--timeout 180]
"""
import argparse
import subprocess
import sys
import time

APP_OFFSET = "0x20000"


def esptool(args, timeout=120):
    cmd = [sys.executable, "-m", "esptool", "--chip", "esp32s3"] + args
    return subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="COM5")
    ap.add_argument("--timeout", type=int, default=180)
    ap.add_argument("--bin", default="build/temco_app.bin")
    a = ap.parse_args()

    base = ["-p", a.port, "-b", "460800", "--before", "no_reset", "--after", "no_reset"]

    print("waiting for the bootloader on %s ..." % a.port, flush=True)
    t0 = time.time()
    ok = False
    # Do NOT probe with chip-id first: the ROM answers the sync only once per
    # power-on, and a probe leaves the flasher stub resident so the next
    # invocation cannot talk to it.  write_flash syncs by itself, so retry that.
    while time.time() - t0 < a.timeout:
        r = esptool(base + ["write_flash", APP_OFFSET, a.bin], timeout=300)
        out = r.stdout + r.stderr
        if "Hash of data verified" in out:
            ok = True
            print("flash: verified", flush=True)
            break
        print("  attempt: %s" % [l for l in out.splitlines() if "ERROR" in l][-1:], flush=True)
        time.sleep(2)
    if not ok:
        print("flash failed - power-cycle the board (J4+J6) and retry")
        return 1

    print("booting the application ...", flush=True)
    r = esptool(base + ["run"], timeout=60)
    print("run:", "ok" if r.returncode == 0 or "Resetting" in r.stdout + r.stderr else "failed")
    print((r.stdout + r.stderr).strip().splitlines()[-1][:120])
    return 0


if __name__ == "__main__":
    sys.exit(main())
