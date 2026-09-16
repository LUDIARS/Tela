---
x-anatomia:
  kind: specification
  id: tela-core-contract
---
# Tela core contracts

## Declaration [id: SPEC-TL-DECLARATION]

IDs are nonempty and unique across one document, independent of visible text.
Panel children reference their parent ID. Failed panel declarations roll back
their entire subtree and restore the parent scope. Static annotations default to
passthrough; actionable buttons default to exclusive input. Declaration does not
execute application actions or start rendering.

## Runtime [id: SPEC-TL-RUNTIME]

The runtime retains press/activation state by stable element ID and binds the
latest declaration's action. Removing an active action cancels its gesture.
Panel bounds clip both paint and input. Themes and geometry invalidate layout;
identical declarations may update callbacks without submitting another frame.
Logical layout pixels are converted to physical pixels once by the host adapter.

## Input ownership [id: SPEC-TL-INPUT]

Passthrough and shared regions never take physical input through native hit
windows. Exclusive input uses native capture. The owner remains fixed from down
through up/cancel; release outside the original button does not execute it.
Shared input arrives as a host observation and is not injected into the OS.
Duplicate sequence, foreign source and stale-revision events cannot activate an
action twice. Disconnect, target loss and focus loss cancel active gestures.

## Overlay lifecycle [id: SPEC-TL-OVERLAY]

The overlay follows the physical viewport and renders only when invalidated and
visible. A hidden, minimized, destroyed or unrelated-background target suppresses
presentation. Restore invalidates the stored surface. Native windows, capture,
GDI surfaces and IPC handles are released by their owners. The current backend
is explicit Pictor CPU bitmap composition, not transparent Vulkan presentation.

The optional MacOS adapter follows the coordinate conversion and native ownership
rules in [SPEC-TL-MACOS](macos-composition.md). macOS global points are scaled with
the target display's backing scale for each viewport revision; anchor producers
must use that same basis. Native Mac acceptance remains required.

## Unity bridge [id: SPEC-TL-BRIDGE]

The wire protocol carries protocol version, host/view identity, monotonic
sequence and viewport revision with every event. Screen coordinates use physical
desktop pixels; negative desktop origins are valid. Per-view selection uses stable
Unity object IDs. Drop stale geometry revisions and duplicate input sequences.
Scope streams to a connection generation so reconnects cannot revive old gestures.
Connection loss cancels captures and hides the overlay. Use authenticated local IPC,
bounded queues and coalesce moves without dropping down/up/cancel ordering.

The Unity sample defers its initial hello until the selected Scene's GUI callback
runs with that Scene focused and a Unity-owned foreground root window. Clicking
Connect in a floating bridge window is not sufficient evidence of Scene ownership.
A changed root requires explicit reconnect (new transport generation). The sample
conservatively hides when the selected Scene loses focus, including Inspector or
another Unity tab, and repaints once when focus returns. It does not periodically
force Scene repaints to infer visibility. A bounded one-second heartbeat remains
necessary for the native three-second transport timeout; suspension does not mean
all Editor callbacks or transport maintenance stop. The pipe worker owns its wait
event and disposes it in finally, including eventual exit after a bounded join.

The C++ structs are in-process value types, not a packed ABI or wire format.
Serialization, current-user authentication, bounds checks and version rejection
are implemented by the optional Windows pipe adapter; see `bridge-protocol.md`.

## Transition authoring [id: SPEC-TL-TRANSITIONS]

Transition IDs, source, destination and object IDs must be nonempty. Each field
is a bounded single-line UTF-8 string; condition may be empty (unconditional).
Persistence uses `TELA_TRANSITIONS 1` followed by five space-separated,
double-quoted fields (ID, source, destination, condition, object ID), with quote
and backslash escaped by backslash, LF lines and no BOM. It is single-writer
application storage, not a packed C++ ABI or a Pf database substitute.
Invalid loads leave the in-memory model unchanged. Save writes a sibling pending
file before replacing the destination. A leftover pending file is reported and
requires recovery; it is not silently overwritten. Conditions are stored as
specification text and are never evaluated as code.

## Scene overlay [id: SPEC-TL-SCENE-OVERLAY]

Pf exports one frame of a scene and the scenes layered over it as
`TELA_SCENE_OVERLAY 1`: LF lines, no BOM, then `frame "name" width height`,
`scene "id" "name" visible` (0 or 1, bottom to top) and
`element "scene id" "id" "kind" "label" x y width height` in the exported frame's
coordinates. Fields use the transition file's quote and backslash escaping. At most
32 scenes, 1024 elements and 4 MiB; sizes are positive and coordinates finite within
±100000. Unknown records, a missing or repeated frame, duplicate IDs, elements of
unknown scenes and trailing data reject the whole file. Tela only reads the file;
Pf remains the source of truth.

The frame is fitted into the logical viewport, keeping its aspect ratio and centered.
Visible scenes declare outlined elements and labels with passthrough input. Every
scene keeps an exclusive toggle button. A toggle changes session state only and is
never written back. Unchanged declaration inputs are not redeclared, and a hidden
host declares nothing.

## Spec view [id: SPEC-TL-SPEC-VIEW]

Pf exports its specification visualization as `TELA_SPEC_VIEW 1`: LF lines, no BOM, then
`view "project" "version" width height`, `group "id" "name" visible` (0 or 1, in the axis'
fixed order) and `card "group id" "code" "title" "status" version x y width height` in the
exported view's coordinates. Fields use the transition file's quote and backslash escaping.
At most 32 groups, 256 cards and 4 MiB; sizes are positive, coordinates finite within
±100000 and the card version a count within 0..1000000. Group IDs and card codes are
nonempty and at most 256 bytes because they become element ID segments; other fields are
at most 4096 bytes. Unknown records, a missing or repeated view header, an empty group
list, duplicate IDs, cards of unknown groups and trailing data reject the whole file.
Tela only reads the file; Pf remains the source of truth.

The view is fitted into the logical viewport, keeping its aspect ratio and centered.
Visible groups declare outlined cards with a code line (`code status vN`) and a title line,
both with passthrough input. The card fill pre-blends Pf's dark canvas with the group color
at the same ratio Pf uses, so the near-white text reads over a light host as well as a dark
one; the outline keeps the group color. Every group keeps an exclusive toggle button. A toggle
changes session state only and is never written back. Unchanged declaration inputs are not
redeclared, and a hidden host declares nothing. The group palette matches Pf's
`SPEC_VIEW_PALETTE`, so the Pf screen and the Tela overlay draw the same picture.

## Overlay placement [id: SPEC-TL-PLACEMENT]

An attached session chooses where its viewport sits with `--place`
(`inside`, `left`, `right`, `above`, `below`; `inside` is the default). `inside` uses the
target's client area, as before. The other placements build the viewport from the content's
own exported size and put it beside the target window, so a view larger than the host is
read at 1:1 instead of being shrunk. A placement with no room in the monitor work area flips
to the opposite side; when neither side fits the viewport is clamped into the work area, and
a view larger than the work area is reduced to it. A degenerate size or work area falls back
to the host rect. Placements other than `inside` require `--attach probe-target`, because
through the Unity bridge the host decides the geometry. Target visibility, minimization and
loss are handled exactly as for `inside`.

`--font-size <8..96>` sets the theme's text size for the run and scales the line height by
the theme's existing ratio; the typeface stays the host's `--font` file. The size is a
per-run choice because one exported file is read on hosts of very different sizes.

## View host [id: SPEC-TL-VIEW-HOST]

`tela_view --font <ttf> --spec-view <file>` draws a declaration in Tela's own top-level
window instead of overlaying a host, for the case where the target application cannot be
started. Without `--width` / `--height` the window takes the content's exported size, so it
is read at 1:1. `--fullscreen` starts borderless on the window's monitor, F11 toggles it and
Esc closes. `--font-size 8..96` sets the theme text size and scales the line height by the
theme's ratio. A run without a font or without a content source fails instead of showing an
empty window.

The view owns its whole surface and all of its input, so the overlay's passthrough and
exclusive regions do not apply and declared buttons are clickable directly. The viewport is
the client area with the window's DPI, and is only rebuilt when size, DPI, visibility or
focus changed. Unchanged declarations present no frame, a hidden or minimized window draws
nothing, and closing releases presentation and input ownership exactly as a lost host does.

`Layout::lines` is the number of text rows a box reserves (1 by default, at most 64). The
renderer wraps inside the box width, so a single row silently drops everything past the
first break; a caller that wants wrapped text declares the rows it needs.

## Graph view [id: SPEC-TL-GRAPH]

Pf exports its domain relation diagram as `TELA_GRAPH 1`: LF lines, no BOM, then
`graph "project" "title" width height`, `group "id" "name" visible r g b` (0 or 1 and three
0..255 channels, in the diagram's column order), `node "group id" "id" "label" x y width height`
and, for each relation, `edge "id" "from node" "to node" dashed` followed by its
`point "edge id" x y` records in route order. Fields use the transition file's quote and
backslash escaping. At most 8 groups, 128 nodes, 256 edges, 4096 route points and 4 MiB;
sizes are positive and coordinates finite within ±100000. IDs are nonempty and at most 256
bytes because they become element ID segments. Unknown records, a missing or repeated
header, an empty group or node list, duplicate IDs, a node in an unknown group, an edge with
an unknown end, a point before or outside its edge, an edge with fewer than two points and
trailing data reject the whole file. Tela only reads the file; Pf remains the source of truth.

Tela performs no layout: node positions and edge routes come from Pf and are drawn as given,
never re-curved, so the Pf screen and the Tela drawing keep the same shape. The graph is
fitted into the logical viewport, keeping its aspect ratio and centered. Visible groups draw
their nodes outlined in the group colour, filled with `tint` of that colour, and
labelled over the rows the box allows. An edge is drawn only while both of its ends are in
visible groups, solid for membership and dashed for a parent link. Every group keeps an
exclusive toggle; a toggle changes session state only and is never written back. Unchanged
declaration inputs are not redeclared, and a hidden host declares nothing.

`tint(accent)` is the shared blend of Pf's dark canvas with an accent colour, used
wherever Tela draws Pf content without a canvas of its own.
