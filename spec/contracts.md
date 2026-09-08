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

## Unity bridge (planned)

The future wire protocol carries protocol version, host/view identity, monotonic
sequence and viewport revision with every event. Screen coordinates use physical
desktop pixels; negative desktop origins are valid. Per-view selection uses stable
Unity object IDs. Drop stale geometry revisions and duplicate input sequences.
Scope streams to a connection generation so reconnects cannot revive old gestures.
Connection loss cancels captures and hides the overlay. Use authenticated local IPC,
bounded queues and coalesce moves without dropping down/up/cancel ordering.

The C++ structs are in-process value types, not a packed ABI or wire format.
Serialization, connection authentication, bounds checks and version negotiation
must be implemented before accepting input from another process.
