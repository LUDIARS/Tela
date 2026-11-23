# Tela implementation status (2026-09-08)

Product UX is recorded verbatim in ux/product.md. DDD direction is instructed by neco;
context boundaries in architecture/ddd.md remain proposals.

Implemented foundations: declarative runtime, isolated gesture ownership,
Pictor CPU layered-window adapter, authenticated bounded named-pipe bridge,
Unity Editor package and transition-authoring sample. Pf and Anatomia project
registrations are present; Anatomia indexes the main project checkout.

Verification completed in the task checkout:
- MSVC Release native build and four CTest contract tests passed.
- Installed Tela package linked from the separate CMake consumer.
- Editor package compiled against installed Unity 6000.3.10f1 assemblies.
- Anatomia diff verification passed rule_conformance, duplication and coupling_delta.
  It still reports spec_linkage for C++ header definitions and the consumer entry
  point despite file-level contract annotations, and convention_drift for three
  snake_case native sample helpers. These warnings are unresolved; this is not
  an Anatomia pass. No gate has been disabled.

Outstanding acceptance:
- Physical transparency, passthrough and exclusive/shared interaction over a
  separate process have not been exercised. Run through Excubitor from Tela main
  after review/merge, with a Concordia testing claim.
- Real Unity attachment is untested. On 2026-09-08 neco selected September
  (E:/Document/Ars/September) as the test project. Its ProjectVersion.txt specifies
  Unity 6000.0.41f1, absent from the inspected D:/Unity/Hub installations.
  neco approved installed Unity 6000.0.59f2, including required reimport/update.
  No September Excubitor catalog entry was found. Launch registration remains
  required. September is on perf/first-wave-settings with existing changes and
  has no local main branch; its integration checkout base needs to be resolved.
  Existing project changes have not been modified.
  In particular, floating Scene views and mixed-DPI host-window identity need
  verification; the current adapter selects the foreground Unity HWND on connect.
- Idle CPU/GPU, native handle/GDI cleanup and reconnect behavior need actual runs.
  Native reports provide CPU/frame/handle counters, not GPU measurements.

Build success does not close the seven requested runtime acceptance tasks.

## Review 1512 follow-up

The native executable was separated into option parsing, probe presentation,
resource diagnostics and session orchestration. MSVC Release build and all four
CTest contract regressions passed; the Concordia testing claim was released.
Ephemeral Anatomia review measured average cyclomatic complexity 1.735, maximum 6,
score 84 (previously 1.793, 8, 83). The bootstrap baseline score is 100; the
relative score gate still blocks this implementation. No threshold was changed.
The orphan advisory counts C++ qualified methods despite file-level spec links;
69 are currently reported. This remains unresolved rather than a passing check.

September integration targets develop by neco instruction and is registered as
Project-September/September in Revisor (local PR 1513). Runtime acceptance remains
blocked on reviewed deployment to the project body.

## Studio controls and web rendering (2026-11-23)

Stages 1 and 2 of [the web renderer plan](plan/2026-11-23-web-renderer-and-studio.md)
(Actio `actio:3065429c-591a-4016-b247-38ff0f5231cf`):

- [Input controls](feature/input-controls.md) (SPEC-TL-CONTROLS): text field (secret), text area,
  slider, list item, separator, status, scroll, key/text events, runtime focus and wheel scrolling;
  a backend-neutral look (`control_visual`) drawn by the CPU raster; Windows view keys, IME
  committed text and wheel.
- [Web rendering](feature/web-rendering.md) (SPEC-TL-WEB): `Tela::Web` `WebSession` (bounded
  input JSON, changed-only frame JSON, secrets as bullets, images once), `web/tela.js`,
  video element, `tela_web_stdio` check host.

Verification: MSVC Release build of every target with `TELA_BUILD_WINDOWS=ON` and Pictor had
no warnings or errors. CTest was not run in the implementing session (permission to execute
was not granted there); new contract tests are `tela_controls_contract`,
`tela_text_edit_contract` and `tela_web_contract`. Native window input, browser drawing, IME
and MediaStream overlay have not been exercised. GPU drawing is a later stage.

## GPU rendering (2026-11-23)

The GPU stage of [the web renderer plan](plan/2026-11-23-web-renderer-and-studio.md)
(Actio `actio:255cd33d-cf29-4291-b752-deb3ff255533`), [GPU rendering](feature/gpu-rendering.md)
(SPEC-TL-GPU): `Tela::GpuFrame` plans the placed result as quads, scissors and CPU-rasterized
layers in exact painting order (no GPU needed); `Tela::Gpu` `GpuRenderer` draws it through
Pictor's Vulkan `UIRenderer` into a host-owned render pass, with Pictor TrueType text and the
CPU shape raster in the layers, sRGB-aware colors, DPI and changed-only uploads. The check host
`tela_gpu_controls` shows the control gallery in a GLFW + Vulkan window with mouse, wheel, keys
and committed text.

Verification: MSVC Release build of every target with `TELA_BUILD_GPU=ON`, `TELA_BUILD_WINDOWS=ON`,
Pictor `build/avatar-live-sdk` and Vulkan SDK 1.4.341.1 had no warnings or errors;
`tela_gpu_frame_tests` was built and run in the default configuration and passed. The full CTest
run and the `scripts/verify.ps1` timing were not done (CTest execution was refused by the
session's permissions). Drawing in a real window and its input have not been exercised.
