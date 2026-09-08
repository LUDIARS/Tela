# Overlay diagnosis and Scene lifecycle

Implementation sequence requested by neco on 2026-09-08:

1. Expose why a synchronization did not present. A versioned report counts actual
   observations and retains the last reason/process pair across connections.
   Diagnostic collection does not bypass visibility/input gates. No observation
   is distinct from target loss. Zero frames are not visual acceptance.
2. Defer Unity hello until the selected Scene owns Editor focus and a Unity-owned
   foreground root. A floating bridge window cannot supply the Scene's identity.
   Moving the Scene to another root requires explicit reconnect with a new generation.
3. Hide once when that Scene loses focus, restore with one repaint, retain only
   bounded transport heartbeats. This deliberately hides on Inspector focus too:
   focus provides reliable evidence without private Unity docking APIs or repeated
   GUI polling. Visible unfocused Scene annotations are not promised in this sample.
   The normal native loop wakes on pipe/messages and otherwise waits one second;
   the diagnostic probe retains 100 ms geometry sampling.
4. Check presentation suppression, bridge generations, canceled gestures, binding
   eligibility, activity transitions, and transport worker/event cleanup. Event
   lifetime belongs to the worker, even if the caller's bounded join expires.

## Validation

Scene connection/focus coordination and scene snapshot publishing have separate
owners. `SceneFramePublisher` owns geometry/selection/anchor change detection and
observed input serialization; `SceneBridge` owns connection setup and activity.
Windows presentation eligibility is evaluated before bitmap composition. This
keeps the diagnostic and host-binding additions out of the rendering/GUI callbacks.

Windows Release build and Unity 6000.0.59f2 reference compilation are required.
CTest covers six suites including presentation and 64 repeated reconnect cycles.
The pure C# runner checks 16 canceled connects and an unaided connect timeout;
it does not start Unity or a native overlay. These checks do not establish
physical input, mixed DPI, native GDI/capture teardown, or GPU usage. Those remain
main-folder Excubitor acceptance checks with Concordia testing claims.

## References

- `spec/contracts.md`: SPEC-TL-OVERLAY, SPEC-TL-BRIDGE, SPEC-TL-INPUT
- `spec/windows-composition.md`: report format and physical acceptance
- `spec/plan/problem_logs/2026-09-08-zero-frame-probe.md`: initial evidence
