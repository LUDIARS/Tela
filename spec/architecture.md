# Tela architecture

Tela owns the public C++ UI API. Applications declare components and supply data
and actions. They do not access Vulkan/DirectX handles. Pictor remains a lower
rendering library and must not depend on Tela or Unity.

Product purpose is [LUDIARSの柔軟なツール描画](ux/product.md).
DDD vocabulary, context candidates and layer policy are in [DDD](architecture/ddd.md).

## Modules

- Core: stable element identity, declarations, input ownership, UI state.
- Pictor adapter: layout output to premultiplied bitmap composition. Existing
  Pictor UIRenderer skips text and exposes Vulkan details, so a complete adapter
  must explicitly bridge text and clip semantics; it is not a cast of enum values.
- Desktop host: window lifecycle, transparent composition, hit regions,
  focus, gesture capture, DPI, target visibility and position tracking.
- Unity bridge: Scene view bounds, camera-projected anchors, observed
  input and selection notifications. UI rendering remains in native Tela.
- Applications: transition specification data, validation and editing actions.

Ergo's existing retained UI is a reference and potential optional adapter, not a
mandatory dependency in the bootstrap. Ownership is not moved out of Ergo here.
Do not fork an entire widget toolkit or renderer into Tela.

## Input and scheduling

Passthrough routes physical input to Unity. Exclusive regions belong to Tela.
Shared regions route physical input to Unity and receive observations through the
bridge. Never replay these observations into the OS or Unity. The gesture owner
is fixed from down through up/cancel; target loss, disconnect and focus changes
cancel active gestures. A click notification cannot retroactively consume input.

Reevaluate on input, data/theme changes, geometry changes or animation deadlines.
Do not continuously submit identical GPU frames. Hide and suspend when the target
Scene view is hidden, closed, minimized or obscured by unrelated foreground apps.
Measure idle CPU/GPU, interaction latency and resource lifetime with the native
acceptance procedure; successful compilation makes no performance claim.

## Delivery sequence

1. Core document and host contracts.
2. Pictor text/graphics adapter and Windows composition feasibility prototype.
3. Layout, stable-ID reconciliation, actions and bounded event scheduling.
4. Authenticated local Unity bridge and Scene view tracking.
5. Shared input, gesture cancellation and transition-specification application.

Windows composition with Pictor's Vulkan backend must be verified before promising
transparent GPU presentation. The general DX12 backend is not implemented merely
by creating Tela. The selected Windows path is explicitly CPU bitmap composition.
The host process exposes a current-user named pipe only when explicitly launched;
there is no Unity injection or network listener.
