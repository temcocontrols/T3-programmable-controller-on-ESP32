# Device REST API for Display Rendering (EEZ Studio / T3000 WebView)

This document specifies the device-side REST API and JSON formats required for full compatibility with the T3000 WebView / EEZ Studio frontend. Implementing these endpoints on your hardware/software will let the editor and web UI treat your device as a drop-in replacement for the built-in mock (`/api/eez-device/*`).

Place this file on your device development team docs and use the sample payloads and schemas to implement and test the service.

---

## Table of contents

- Purpose & compatibility
- Base path & routing
- Endpoint list (detailed): requests, responses, error codes
- Screen JSON format (fields, widgets, fonts, bitmaps) — canonical schema and examples
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

## Screen JSON format (canonical)

The editor expects a consistent JSON structure for each screen. The repo's mock returns a `json` object for each screen which typically contains keys like `fonts`, `bitmaps`, and `widgets`. The exact widget schema is driven by the EEZ/LVGL flow runtime; below is a general-purpose, implementable shape that is compatible with the editor.

Canonical screen JSON structure:

```json
{
  "fonts": [ { "name": "Roboto", "size": 16 }, ... ],
  "bitmaps": [ "logo", "icon_wifi", ... ],
  "widgets": {
    "widgetId1": {
      "type": "label",
      "x": 10,
      "y": 20,
      "w": 200,
      "h": 40,
      "text": "Hello",
      "style": { "color": "#ffffff", "font": "Roboto" }
    },
    "widgetId2": {
      "type": "image",
      "x": 220,
      "y": 20,
      "w": 48,
      "h": 48,
      "src": "logo"
    },
    "widgetId3": {
      "type": "button",
      "x": 10,
      "y": 70,
      "w": 100,
      "h": 30,
      "label": "Click",
      "onClick": { "action": "navigate", "target": "menu_screen" }
    }
  },
  "bg_color": "#000000",
  "meta": { "panelId": 0, "created": "2026-08-05T12:00:00Z" }
}
```

Notes:
- `widgets` can be an object keyed by widget id (string) — editor code in the repo expects this mapping.
- Widget `type` may be `label`, `image`, `button`, `container`, `slider`, `gauge`, `chart`, etc. Provide properties relevant to LVGL widgets.
- `bitmaps` list names that the screen references; images must be uploaded via `/images/push` or embedded in screen JSON as base64 (embedding not recommended for large assets).
- `fonts` list names and sizes used by screen.

### Recommended widget properties (common)
- `label`: `text`, `font`, `color`, `align`
- `image`: `src` (name), `scale`, `opacity`
- `button`: `label`, `onClick` (action object)
- `container`: `children` (list of widget ids or keys)

The editor will treat screen JSON generically; ensure keys are predictable.

---

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


---

Generated by the T3000 WebView codebase analysis; meant to be used as a developer integration guide for device-side REST API implementation supporting EEZ Studio and the web editor.
