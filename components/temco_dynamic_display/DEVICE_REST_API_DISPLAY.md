# Device REST API for Display Rendering (EEZ Studio / T3000 WebView)

This document specifies the device-side REST API and JSON formats required for full compatibility with the T3000 WebView / EEZ Studio frontend. Implementing these endpoints on your hardware/software will let the editor and web UI treat your device as a drop-in replacement for the built-in mock (`/api/eez-device/*`).

Place this file on your device development team docs and use the sample payloads and schemas to implement and test the service.

---

## Table of contents

- Purpose & compatibility
- Base path & routing
- Endpoint list (detailed): requests, responses, error codes
- Screen JSON format (fields, widgets, fonts, bitmaps) — canonical schema and examples
- Keeping the shipped defaults in step with the compiled UI, and verifying they render the same
- Delta patch semantics (dot-path patching)
- Image upload/download format
- BACnet-style RPC compatibility
- Headers, CORS, limits & behavior
- Error handling & status codes
- Test commands (curl examples)
- Implementation checklist

---

## Purpose & compatibility

This API lets the EEZ Studio web frontend (and other tooling in the T3000 WebView repo) load, edit, patch, and deploy display screens (LVGL/EEZ projects), images, and metadata from a physical device. The frontend expects the following behavior and JSON shapes used in the repository's mock implementation at `api/src/eez_studio/bacnet_api_mock.rs`.

Implementations must match endpoints, HTTP methods, and JSON structures below for full compatibility.

---

## What you must implement on the device side

### 1. Device-side server (must be implemented on the device)

Your device should expose a small HTTP server that acts as the backend for the display editor. The device/server side is responsible for:

- Listening on a local or network port.
- Implementing the REST endpoints under `/api/eez-device`.
- Parsing and validating incoming JSON payloads.
- Storing screens and images locally (RAM, flash, or filesystem).
- Rendering or refreshing the display when a screen is deployed or patched.
- Returning structured success/error responses.

In practice, this means implementing:

- `GET /device/info`
- `GET /screens` and `GET /screens/:name`
- `PUT /screens` and `PUT /screens/:name`
- `PATCH /screens/:name`
- `POST /images/push/:panelId` and `GET /images/pull/:panelId/:name`
- Optional: `DELETE /images/:panelId/:name`

### 2. Client-side app (implemented in the editor/web host app)

The client is not the device; it is the web UI, desktop app, or host application that talks to the device. The client is responsible for:

- Building or editing the screen JSON.
- Sending screens to the device with `PUT` requests.
- Uploading image assets.
- Sending patch updates for live changes.
- Showing device info and errors returned by the device.

Typical client flow:

1. Read device info from the device.
2. Load current screens from the device.
3. Edit screen JSON in the UI.
4. Deploy the edited screen or patch specific widget fields.
5. Upload any referenced images.

### 3. How to wire them together

A simple architecture is:

- Device server receives HTTP requests.
- Device server stores the screen JSON and image binaries.
- Device server converts the JSON into native drawing commands for the display.
- Device server sends back success/error JSON so the client can update the UI.

Recommended implementation order:

1. Implement the device info endpoint.
2. Implement screen list and single-screen fetch.
3. Implement screen deploy endpoints.
4. Implement patching for live updates.
5. Implement image upload/download.
6. Add rendering logic that redraws the screen after each deploy or patch.

### 4. Minimal first version

If this is your first implementation, keep it small:

- One HTTP server on the device.
- One storage folder for screen JSON files.
- One storage folder for image assets.
- A simple renderer that supports text, image, and button widgets.
- Basic validation and error responses.

That is enough to support a working device-side REST UI pipeline without overcomplicating the first release.

---

## Base path & routing

- Base path: `/api/eez-device`
- Content type: `application/json` for JSON endpoints.
- Accept `serial_number` either as a URL query parameter or inside request JSON bodies where noted.

Mount examples (server side):
- `GET  /api/eez-device/device/info`
- `GET  /api/eez-device/screens`
- `GET  /api/eez-device/screens/:name`
- `PUT  /api/eez-device/screens` (deploy all)
- `PUT  /api/eez-device/screens/:name` (deploy single)
- `PATCH /api/eez-device/screens/:name` (delta apply)
- `PATCH /api/eez-device/screens/:name/widgets/:widgetId`
- `POST /api/eez-device/images/push/:panelId`
- `GET  /api/eez-device/images/pull/:panelId/:name`
- `DELETE /api/eez-device/images/:panelId/:name`
- BACnet-style: `POST /api/eez-device/screens/push/:panelId` and `POST /api/eez-device/screens/pull/:panelId`

---

## Endpoint details

All request and response examples show canonical shapes; implementors should return matching JSON keys and appropriate HTTP status codes.

### GET /api/eez-device/device/info

Purpose: quick summary for UI (screen list, counts, sizes).

Request:
- Query: optional `serial_number` (int)

Response (200):
```json
{
  "panel_name": "T3-ESP32",
  "serial_number": 12345,
  "screen_size": { "width": 480, "height": 320 },
  "screen_count": 12,
  "screens": ["start_up_screen","home_screen","menu_screen"],
  "image_count": 25,
  "font_count": 4,
  "firmware_version": "1.2.3",
  "lvgl_version": "9.1.0",
  "dark_theme": true,
  "color_format": "RGB"
}
```

Errors:
- 503 Service Unavailable if device cannot report info.
- 500 Internal Server Error on unexpected failures.

---

### GET /api/eez-device/screens

Purpose: return all screens (heavy JSON). This is used by the editor to load projects.

Request:
- Query: optional `serial_number` (int)

Response (200):
```json
{
  "screens": [
    { "name": "home_screen", "json": { /* screen JSON object */ } },
    { "name": "menu_screen", "json": { /* ... */ } }
  ],
  "meta": { /* optional: device meta (panel_name, serial_number) */ }
}
```

Behavior:
- Return screens in the desired UI order.
- For very large responses, streaming or compressed transfer is OK as long as `application/json` is returned.

Errors:
- 200 with `screens: []` if no screens or parse fails but service still available.
- 500 if service cannot produce screens.

---

### GET /api/eez-device/screens/:name

Purpose: fetch a single screen by name.

Response (200):
```json
{ "name": "home_screen", "json": { /* screen JSON object */ } }
```

Errors:
- 404 Not Found if screen not present.
- 500 on parse error.

---

### PUT /api/eez-device/screens

Purpose: deploy/replace all screens on device (editor "deploy" action).

Request body:
```json
{
  "screens": [ { "name": "screen1", "json": { /*...*/ } }, ... ],
  "serial_number": 12345
}
```

Response (200):
```json
{
  "deployed": 10,
  "failed": 0,
  "status": "ok",
  "errors": null
}
```

Errors:
- 400 Bad Request when required fields missing.
- 200 with `status: "partial"` and `errors` array if some screens failed validation.
- 500 on internal failure.

---

### PUT /api/eez-device/screens/:name

Purpose: deploy/replace a single screen by name.

Request body:
```json
{ "json": { /* screen JSON */ }, "serial_number": 12345 }
```

Response (200):
```json
{ "name": "home_screen", "status": "ok", "error": null }
```

Errors: 400/500 as above.

---

### PATCH /api/eez-device/screens/:name

Purpose: apply delta (dot-path) changes to a screen's JSON.

Request body:
```json
{
  "changes": [
    { "path": "widgets.myLabel.text", "value": "New text" },
    { "path": "widgets.tempSensor.value", "value": 22.5 }
  ],
  "serial_number": 12345
}
```

Response (200):
```json
{
  "applied": 2,
  "rejected": 0,
  "status": "ok",
  "errors": null
}
```

Behavior:
- For each change, set the dot-path to provided value. If path missing, mark rejection but continue.
- Apply operations in order.

Errors:
- 200 with `applied`/`rejected` counts and error details for rejected changes.
- 500 if processing fails catastrophically.

---

### PATCH /api/eez-device/screens/:name/widgets/:widgetId

Same as the single-screen PATCH but server should apply each change with a prefix of `widgets.<widgetId>.`.

Request:
```json
{ "changes": [ { "path": "text", "value": "Click" } ], "serial_number": 123 }
```

Server internally applies to `widgets.<widgetId>.text`.

---

### Images

#### POST /api/eez-device/images/push/:panelId

Purpose: upload/replace an image/bitmap on device.

Request JSON:
```json
{ "name": "logo", "data_base64": "<base64-encoded PNG or LVGL binary>" }
```

Response:
```json
{ "name": "logo", "status": "ok" }
```

Behavior:
- Validate `name` non-empty.
- Store image bytes; return 200.
- Reject and return 400 if `data_base64` invalid or too large.

#### GET /api/eez-device/images/pull/:panelId/:name

Response (200):
```json
{ "name": "logo", "data_base64": "<base64>" }
```

Errors: 404 if not found.

#### DELETE /api/eez-device/images/:panelId/:name

Purpose: remove stored image. Return 200 OK.

---

### BACnet-style compatibility (optional but recommended)

Some legacy clients expect RPC POST handlers (used in the repo's mock):

- POST /api/eez-device/screens/push/:panelId
  - Body: { "serial_number": int, "screens": [ {name,json}, ... ] }
  - Response: same structure as PUT /screens (DeployAllResponse)

- POST /api/eez-device/screens/pull/:panelId
  - Body: { "serial_number": int }
  - Response: { "screens": [ ... ], "meta": ... }

Implement these endpoints to ease integration with BACnet fallback clients.

---

## Screen JSON format

The normative definition — envelope, every widget kind, the style part/state model, `align`
semantics, events, image references and limits — is maintained once, in the T3000 Web View
documentation: `docs/t3000/bacnet-api/screen-json.md`.

What this firmware stores and serves, in the smallest correct form:

```json
{ "bg_color": "#000000",
  "bitmaps": ["fan_small"],
  "fonts": [["lv_font_montserrat_18", 18]],
  "widgets": { "HomeScreen": { "type": "Widget", "sub_type": "screen", "children": {} } } }
```

Every widget node carries `type`, `sub_type`, `x_pos`, `y_pos`, `width`, `height`, `obj_text` and
`text_type`, plus optionally `align`, `style`, `children`, `events` and the `flex_*` set. A
`width` or `height` of `0` means size-to-content. When `align` is present, `x_pos`/`y_pos` are
offsets from the anchor, not coordinates.

`align_to` (optional) mirrors SquareLine's `lv_obj_align_to()`: the widget is placed relative to a
sibling instead of its parent, and is resolved after layout just like content-sized widgets.

```json
"Co2Label": { "align_to": { "target": "OccTimerLabel", "align": "out_top_left",
                            "x_pos": 0, "y_pos": -2 } }
```

Earlier revisions of this page showed a different shape — `type: "label"`, `x`/`y`/`w`/`h`,
`text`, and `screens` as an array. That is not what the firmware emits or accepts: `GET
/device/info` returns `screens` as an object keyed `screen1..N`, and widgets use the fields above.

### Keeping the shipped defaults in step with the compiled UI

`DefaultScreens/*.json` is what the front end loads and what the JSON renderer draws, and it is
derived from the compiled SquareLine sources, never hand-written (`DefaultImages/*.json` come from
the exported image assets and are not generated):

```bash
python tools/gen_json_from_c.py --all --check --diff   # report drift against ui_*.c
python tools/gen_json_from_c.py --all --write         # regenerate the defaults
python tools/gen_ui_json_bind.py                      # refresh the name -> ui_*/uic_* table
python tools/gen_ui_json_events.py                     # refresh handler + input-rule tables
```

Run those three generators whenever `main/TemcoScreen/ui_*.c` changes. `ddns_screen` and
`wireguard_screen` are listed as *procedural*: `ui_RemoteAccess.c` builds them with local
variables, so there is no static design to derive and their JSON stays hand-maintained.

JSON that deliberately differs from the C (for example the home screen's WiFi symbol, which must
open the WiFi configuration screen in JSON mode although the compiled UI ignores taps on it) is
declared in `tools/json_overrides.json` and re-applied on every regeneration.

The regenerated defaults only reach a device that was already flashed through a factory reset:
seeding fills in *missing* files, it never overwrites existing ones. `POST
/api/eez-device/reset-defaults` (the front end's "reset" action) formats `screen_data`, rewrites
all defaults from the firmware image and returns `{"screens_restored": 13}`.

### Verifying that the JSON still renders like the compiled screens

`main/TemcoScreen/ui_json_render.c` draws the screens from `DefaultScreens/*.json`. To prove the two
designs agree, the renderer can dump every built widget's final rectangle over the serial console and
`tools/audit_render.py` recomputes the same rectangle from the JSON:

```bash
python tools/dump_screens.py  --port COM5 --host 192.168.1.46   # navigate all 13 screens, capture
python tools/audit_render.py                                    # ok / MISMATCH per widget

python tools/deploy_defaults.py --host 192.168.1.46             # push the repo defaults to a live unit
python tools/deploy_defaults.py --screens wireguard_screen       # one screen (test only)
```

The dumper is compiled in but switched off (a dump blocks the LVGL task for about a second on the
115200 console). `dump_screens.py` turns it on first:

```bash
curl -X POST -H "Content-Type: application/json" \
     -d '{"layout":true}' http://192.168.1.46/api/eez-device/ui-debug
curl http://192.168.1.46/api/eez-device/ui-debug     # {"ok":true,"layout_dump":true}
```

`deploy_defaults.py` publishes only the files staged in that transaction — `deploy/begin` wipes the
inactive slot — so a filtered run leaves the slot holding just those files. Use it to test one
screen, then deploy everything again (or `reset-defaults`) before shipping.

LVGL rules the audit encodes, which are easy to get wrong in a JSON screen:

| Behaviour | Consequence for the JSON |
| --- | --- |
| `lv_obj_align()` works inside the parent's *content* box: padding plus the border width on the sides where the border is drawn (`border_side`) | a widget inside a bordered panel sits `border_width` further in than one inside a borderless one |
| `lv_obj_set_align()` + `lv_obj_set_pos()` keep the alignment *live*; LVGL re-runs it whenever the parent resizes | never convert an alignment into absolute coordinates: the entry animations resize their targets |
| `lv_textarea_set_one_line(o, true)` forces `LV_STYLE_HEIGHT = LV_SIZE_CONTENT` | a one-line textarea must declare `"height": 0`, whatever height the design asked for |
| `LV_SIZE_CONTENT` is written as `0` | a content-sized widget is placed from its measured size, not from the SquareLine editor hint |
| `ShowItem_Animation` animates `y` and `height` to absolute 0, `*In_Animation` slides `x` | widgets the compiled handler animates are reported as `animated`, not compared |

## Renderer control endpoints

| Method | Path | Body | Effect |
| --- | --- | --- | --- |
| GET | `/api/eez-device/ui-mode` | — | `{"ui_mode":"json"｜"compiled","forced_compiled":bool}` |
| POST | `/api/eez-device/ui-mode` | `{"mode":"json"｜"compiled"}` | switch renderer; returns the resulting mode |
| POST | `/api/eez-device/ui-screen` | `{"name":"time"}` | navigate to a design screen |
| GET | `/api/eez-device/ui-debug` | — | `{"ok":true,"layout_dump":bool}` (report only) |
| POST | `/api/eez-device/ui-debug` | `{"layout":true｜false}` | enable/disable the serial geometry dump |
| GET | `/api/eez-device/device/status` | — | slot, revision, current screen, build state, storage |
| POST | `/api/eez-device/deploy/begin` | `{"schema":1,"start_screen":"..."}` | open a deploy transaction, returns `deploy_id` |
| PUT | `/api/eez-device/deploy/{id}/file?path=screens/time` | raw JSON | stage one file, returns its `sha256` |
| POST | `/api/eez-device/deploy/{id}/commit` | — | swap the slot; returns revision + slot + counts |
| POST | `/api/eez-device/reset-defaults` | — | wipe both slots and reseed from the firmware image |

## Delta patch semantics (dot-path)

Patch format: `path` is a dot-separated path into the JSON object (no array index syntax unless you implement it). Example paths:
- `widgets.myLabel.text`
- `widgets.myButton.style.color`
- `meta.created`

Patch application rules:
- For each change: resolve path left-to-right. If the intermediate key is missing, consider the change rejected (unless you choose to create intermediate objects—document that behavior).
- Replace the value at the final path with the provided `value` (deep clone semantics).
- Apply changes sequentially and return counts of applied and rejected changes.
- Return per-change error messages for rejections.

Example PATCH request:
```json
{
  "changes": [
    { "path": "widgets.w1.text", "value": "New title" },
    { "path": "widgets.w2.src", "value": "new_icon" }
  ]
}
```

Response example:
```json
{ "applied": 2, "rejected": 0, "status": "ok" }
```

---

## Headers, CORS, content-type & limits

- `Content-Type: application/json` for JSON.
- Add `Access-Control-Allow-Origin: *` or device-specific origin if frontend served from another origin.
- Respect large request bodies for screen catalogs and images. The server-side mock in this project sets `DefaultBodyLimit::max(50 * 1024 * 1024)`. Implement a sensible maximum (e.g., 10–50 MB) and return 413 Payload Too Large if exceeded.
- Consider `Accept: application/json` and respond accordingly.

---

## Error handling & status codes

- 200 OK — normal successful response (even for partial failures; inspect `status` in JSON)
- 400 Bad Request — malformed JSON; missing required fields
- 404 Not Found — missing resource (screen/image)
- 413 Payload Too Large — client sent too-large body
- 500 Internal Server Error — unhandled error
- 503 Service Unavailable — device currently cannot serve the request

Include structured `errors` arrays in partial successes.

---

## Test commands (curl examples)

Fetch device info:

```bash
curl -s http://<device>:<port>/api/eez-device/device/info | jq
```

Fetch all screens:

```bash
curl -s http://<device>:<port>/api/eez-device/screens | jq
```

Fetch single screen:

```bash
curl -s http://<device>:<port>/api/eez-device/screens/home_screen | jq
```

Deploy screens (PUT):

```bash
curl -X PUT -H "Content-Type: application/json" -d @payload.json http://<device>:<port>/api/eez-device/screens | jq
```

Patch a screen (PATCH):

```bash
curl -X PATCH -H "Content-Type: application/json" -d '{"changes":[{"path":"widgets.w1.text","value":"Hello"}]}' http://<device>:<port>/api/eez-device/screens/home_screen | jq
```

Push image:

```bash
curl -X POST -H "Content-Type: application/json" -d '{"name":"logo","data_base64":"<base64>"}' http://<device>:<port>/api/eez-device/images/push/0 | jq
```

Pull image:

```bash
curl -s http://<device>:<port>/api/eez-device/images/pull/0/logo | jq
```

---

## Implementation checklist (developer tasks)

1. Implement `GET /api/eez-device/device/info` (lightweight summary).
2. Implement `GET /api/eez-device/screens` and `GET /api/eez-device/screens/:name`.
3. Implement `PUT /api/eez-device/screens` and `PUT /api/eez-device/screens/:name`.
4. Implement `PATCH /api/eez-device/screens/:name` and widget-level patch.
5. Implement image push/pull/delete endpoints.
6. Return exact JSON keys and response shapes described above.
7. Add logging for requests and errors.
8. Add CORS headers if required by your deployment.
9. Add tests: run the `curl` commands above against your device.
10. (Optional) Add BACnet-style POST endpoints for legacy client compatibility.

---

## Implementation tips & examples

- Storage: store screens as JSON files on disk in a `screens/` folder keyed by `name`. Keep an index file listing order.
- Atomicity: when writing multiple screens, write to temp files then atomically rename into place.
- Concurrency: guard in-memory stores with mutexes to avoid race conditions.
- Validation: before accepting a deployed screen, validate the top-level structure contains `widgets` and at least basic meta keys; return `errors` for invalid screens.
- Memory: for devices with limited RAM, stream larger responses and avoid building huge in-memory JSON blobs.

---

## Example screen JSON (full small example)

```json
{
  "fonts": [{ "name": "Roboto", "size": 16 }],
  "bitmaps": ["logo"],
  "widgets": {
    "w1": { "type": "label", "x": 12, "y": 8, "w": 200, "h": 24, "text": "Welcome", "style": { "color": "#ffffff" } },
    "w2": { "type": "image", "x": 12, "y": 40, "w": 64, "h": 64, "src": "logo" },
    "w3": { "type": "button", "x": 80, "y": 40, "w": 120, "h": 40, "label": "Menu", "onClick": { "action": "navigate", "target": "menu_screen" } }
  },
  "bg_color": "#000000",
  "meta": { "panelId": 0, "created": "2026-08-05T12:00:00Z" }
}
```

Use this example as test payloads during development.

---

## JSON request/response formats from the mock implementation

This section lists the exact request and response JSON shapes defined in the repository mock handler `api/src/eez_studio/bacnet_api_mock.rs`.

### Device info response
```json
{
  "panel_name": "T3-ESP32-Firmware",
  "serial_number": 0,
  "screen_size": { "width": 480, "height": 320 },
  "screen_count": 12,
  "screens": ["start_up_screen", "home_screen", "menu_screen"],
  "image_count": 30,
  "font_count": 5,
  "firmware_version": "5.1.0",
  "lvgl_version": "9.1.0",
  "dark_theme": true,
  "color_format": "RGB"
}
```

### Screen list response
```json
{
  "screens": [
    { "name": "home_screen", "json": { /* full screen JSON */ } },
    { "name": "menu_screen", "json": { /* full screen JSON */ } }
  ],
  "meta": {
    "panel_name": "T3-ESP32-Firmware",
    "serial_number": 0
  }
}
```

### Single screen response
```json
{ "name": "home_screen", "json": { /* full screen JSON */ } }
```

### Deploy all screens request body
```json
{
  "screens": [
    { "name": "home_screen", "json": { /* full screen JSON */ } },
    { "name": "menu_screen", "json": { /* full screen JSON */ } }
  ],
  "serial_number": 12345
}
```

### Deploy single screen request body
```json
{
  "json": { /* full screen JSON */ },
  "serial_number": 12345
}
```

### Patch request body
```json
{
  "changes": [
    { "path": "widgets.button1.text", "value": "OK" },
    { "path": "widgets.status.value", "value": 42 }
  ],
  "serial_number": 12345
}
```

### Deploy all screens response
```json
{
  "deployed": 10,
  "failed": 0,
  "status": "ok",
  "errors": null
}
```

### Deploy single screen response
```json
{ "name": "home_screen", "status": "ok", "error": null }
```

### Patch response
```json
{
  "applied": 2,
  "rejected": 0,
  "status": "ok",
  "errors": null
}
```

### BACnet push request body
```json
{
  "serial_number": 12345,
  "screens": [
    { "name": "home_screen", "json": { /* full screen JSON */ } }
  ]
}
```

### BACnet pull request body
```json
{
  "serial_number": 12345
}
```

### Image upload request body
```json
{
  "name": "logo",
  "data_base64": "<base64-encoded image>"
}
```

### Image upload response
```json
{
  "name": "logo",
  "status": "ok"
}
```

### Image download response
```json
{
  "name": "logo",
  "data_base64": "<base64-encoded image>"
}
```

### Patch error example
```json
{
  "applied": 1,
  "rejected": 1,
  "status": "partial",
  "errors": [
    { "path": "widgets.missing.value", "message": "key 'missing' not found" }
  ]
}
```

---

## Where this maps in the T3000 WebView repo

- The server-side mock implementing these endpoints is: `api/src/eez_studio/bacnet_api_mock.rs`.
- The bridge router that mounts `eez-device` endpoints is: `api/src/eez_studio/mod.rs`.
- The frontend client uses `src/t3-eez-studio/bridge/eez-studio-api.ts` to call these endpoints.

Use those files as reference for exact behavior and JSON keys.

---

## Final notes

- Keep the JSON shapes and endpoints stable. If you need to change shapes, version the API (e.g., `/v1/` prefix) and update the editor counterpart accordingly.
- Start with the minimal set (device/info, screens GET, screen GET) then implement PUT/PATCH and images.
- If you want, I can generate a small skeleton server in your device language (Rust/Node/Python/C#) that implements all above endpoints and stores screens on disk — tell me which language and I'll scaffold it.

## Test commnads

IP=192.168.31.164   # replace with your device IP

# GET device info
curl -v http://192.168.31.164/api/eez-device/device/info

# GET all screens
curl -v http://192.168.31.164/api/eez-device/screens

# PUT all screens

curl -s http://192.168.31.164/api/eez-device/screens -o current_screens.json

curl -v -X PUT http://192.168.31.164/api/eez-device/screens -H "Content-Type: application/json" --data-binary "@current_screens.json"

# GET one screen
curl -v http://192.168.31.164/api/eez-device/screens/start_up_screen
curl -v http://192.168.31.164/api/eez-device/screens/home_screen
curl -v http://192.168.31.164/api/eez-device/screens/main_menu
curl -v http://192.168.31.164/api/eez-device/screens/network_config
curl -v http://192.168.31.164/api/eez-device/screens/parameters
curl -v http://192.168.31.164/api/eez-device/screens/protocols
curl -v http://192.168.31.164/api/eez-device/screens/schedule_edit_screen
curl -v http://192.168.31.164/api/eez-device/screens/schedule_screen
curl -v http://192.168.31.164/api/eez-device/screens/time
curl -v http://192.168.31.164/api/eez-device/screens/wifi_config
curl -v http://192.168.31.164/api/eez-device/screens/holiday_calender_screen
curl -v http://192.168.31.164/api/eez-device/screens/ddns_screen
curl -v http://192.168.31.164/api/eez-device/screens/wireguard_screen

# PUT one screen
curl -v -X PUT http://192.168.31.164/api/eez-device/screens/start_up_screen \
  -H "Content-Type: application/json" -d '{"name":"start_up_screen"}'

# PATCH one screen
curl -v -X PATCH http://192.168.31.164/api/eez-device/screens/start_up_screen \
  -H "Content-Type: application/json" -d '{"name":"updated"}'

# Image pull
curl -v http://192.168.31.164/api/eez-device/images/pull/output_40

# Reset/defaults endpoints
curl -v -X POST http://192.168.31.164/api/eez-device/reset-defaults
curl -v -X POST http://192.168.31.164/api/eez-device/set-default-screens
curl -v -X POST http://192.168.31.164/api/eez-device/load-default-screens

# OPTIONS (CORS preflight)
curl -v -X OPTIONS http://192.168.31.164/api/eez-device/screens

---

Generated by the T3000 WebView codebase analysis; meant to be used as a developer integration guide for device-side REST API implementation supporting EEZ Studio and the web editor.
