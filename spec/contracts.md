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
