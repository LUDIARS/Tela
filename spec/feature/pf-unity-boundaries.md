# Pf and Unity presentation boundaries

2026-09-17 neco requested the remaining work, with Pf-driven Unity overlay and
Editor UI/instructions handled separately from Tela rendering.

- Tela owns host-independent rendering, fitting, input ownership, and the external
  overlay lifetime (UX-TL-W1/W2/W3/W4/W5). It does not own Pf credentials or mutations.
- `Tela/unity/com.ludiars.tela` translates Scene viewport/selection/input to the native
  bridge. Its public connection API belongs to host-bridge; it does not interpret Pf
  specifications or arbitrary commands.
- `Praeforma/Packages/jp.ludiars.praeforma` owns the Unity Editor window, chosen Pf
  project, specification/scene selection, instruction drafts and API calls. Pf remains
  the authoritative store. Editor display remains usable without the Tela package.
- Optional overlay integration uses an explicit Tela bridge adapter. Missing package,
  endpoint or overlay process must produce a visible error. It must not silently open
  another application or execute instruction text as code.
- September is an integration consumer, not the owner of shared Editor UI. Existing
  scene/project changes and the running desktop graph demo are preserved.

Acceptance is separate: native contracts; Editor UI and Pf API; actual Scene overlay.
Mac/mixed-DPI/physical-input checks require their corresponding hardware and must not
be closed based on a build or a review alone.
