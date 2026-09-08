# Tela DDD policy

2026-09-08 neco instructed DDD and supplied the product UX:
**「LUDIARSの柔軟なツール描画」**. That direction is decided; the boundaries below
are an implementation proposal, not a record of human boundary approval.

## Contexts and ubiquitous language

| Context | Classification | Owned meaning and invariants |
|---|---|---|
| tool-composition | core candidate | Tool document, stable element identity, declaration transaction, retained state, theme and layout |
| host-overlay | core candidate | Host/view attachment, physical viewport, target visibility, suspension and presentation lifetime |
| input-ownership | core candidate | Passthrough/exclusive/shared, gesture owner fixed from down to up/cancel, observation without replay |
| host-bridge | supporting | Anti-corruption layer translating Unity/other host identity, anchors, selection and input into versioned Tela values |
| pictor-composition | supporting adapter | TrueType text, clipping and premultiplied pixel composition; no application actions |
| transition-authoring | sample application | Transition identity, source/destination/condition, stable object association, validated persistence |

Windows are display hosts, not automatically a separate business domain. UI parts
belong to tool composition unless their own vocabulary and independent invariants
justify a split. Overlay tracking is a meaningful domain because it owns rules
about which target a tool belongs to and when it must stop being visible.
Ordinary windows and overlays may later be two host strategies for the same tool.

## Model and boundaries

`Document` is the declaration aggregate: unique IDs and subtree rollback form one
consistency boundary. `Runtime` is the tool-session coordinator: it reconciles the
document's state, layout and active gesture before exposing a frame. Its input
rules are isolated in the input domain implementation, not mixed into Win32 event
handling. `Viewport`, `Rect`, `Theme` and wire events are values, not packed ABIs.
`BridgeSession` is the connection boundary, rejecting old generations/revisions
before they can affect the runtime. `Transitions` is separate application data;
it is not part of the reusable tool document model.

The dependency direction is presentation/composition root → application → domain.
Infrastructure adapters call the domain/application contracts; domain code has no
Win32, Unity, Pictor, pipe or HTTP dependency. Pictor does not depend on Tela.
The transition persistence implementation is outside the tool runtime.
There is no shared mutable object spanning the Unity process and the native host.

Anatomia membership is in `spec/domains/`; program layer declarations are in
`.anatomia/layers.json`. Pf links the same repository and UX IDs. UX prose and
invariants stay in this repository; registration is not a second editable source
of architectural truth. Business-domain approvals remain separate from generated
program domains and from the fact that Cc's DDD setting is enabled.

## Acceptance

Contract tests cover declaration rollback, stable state, clipping, gesture
cancellation, stale/duplicate messages and save/load consistency. Consumer builds
cover library packaging. Real Windows/Unity behavior, CPU/GPU and resource lifetime
require the scenarios in `spec/windows-composition.md`; build success cannot
substitute for those checks.
