# Windows composition implementation and verification

The selected backend is **Pictor CPU bitmap + Windows layered composition**.
This is an explicit backend choice, not a silent fallback when Vulkan fails.
Pictor's existing `TextImageRenderer` rasterizes TrueType outlines. Tela maps
layout rectangles and text to top-down premultiplied BGRA8, clips them, and calls
`UpdateLayeredWindow(ULW_ALPHA)`. Pictor supplies premultiplied RGBA, so conversion
swaps channels without multiplying alpha a second time. No renderer fork or
mandatory Ergo dependency is introduced.

The main visual HWND has `WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE`.
Exclusive regions use separate no-activate layered HWNDs with region masks.
Later shared regions cut through earlier exclusive masks, including overlaps.
The main visual bitmap excludes exclusive pixels so Windows blends them once.
The attached overlay and the desktop overlay share this composition (`LayeredComposition`) and
the native pointer translation (`NativePointer`); neither keeps a second copy.
`SetCapture` fixes native drag routing until up/cancel; hidden/destroyed targets
and capture loss release it. Text-entry editing uses a modeless Windows text
input surface; annotations and buttons stay in the Pictor surface.

Pictor's current Vulkan swapchain hard-codes `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR`.
This implementation does **not** claim transparent Vulkan swapchain support,
DirectComposition GPU interop, complex-script shaping or CFF/OTF outlines.
Use an explicitly supplied TrueType font containing the required characters.

Primary API references:

- [Windows layered-window input and lifetime](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features)
- [UpdateLayeredWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-updatelayeredwindow)
- [Unity pixelsPerPoint](https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorGUIUtility-pixelsPerPoint.html)

## Native acceptance procedure

Build with `TELA_BUILD_WINDOWS=ON` and explicit Pictor include/library locations.
Deploy build artifacts to the project's main `build-native` directory. Place a
licensed local TrueType font at `build-native/assets/default.ttf`. The catalog's
native examples are opt-in, not autostart services. Register a Cc testing claim,
then launch **from the main project** through Excubitor:

1. Start `tela-probe-target` (a separate process with an observable click count).
2. Start `tela-probe` and focus the target. Background grid must remain visible
   through the translucent panel and text edges must have no opaque rectangle.
3. Click the annotation: only the host count increases. Click the Tela button:
   only Tela's count increases. Drag out/up: no action. Repeat double clicks;
   each physical gesture has exactly one destination.
4. Move, resize, minimize, obscure and restore the host, including mixed-DPI and
   negative-origin monitors. Confirm alignment and hidden-state suspension.
5. Leave idle for the remainder of the probe's 120-second lifetime. Read
   `build-native/composition-report.json` for elapsed time, process CPU, frame
   submissions, handles and GDI objects before/after teardown. Measure DWM/GPU
   externally; process CPU is not a GPU measurement.
6. Stop the probe target through Excubitor and release the Cc claim.

For Unity, add the local `unity/com.ludiars.tela` package, start `tela` through
Excubitor, and use Tools/Tela/Scene Bridge. Select a Scene object, add/edit/save a
transition, move the Scene camera, change selection, hide the dock tab, disconnect,
reconnect and close Unity. Repeat idle and resource measurements during actual
Unity use. A successful library build is not evidence that these UI checks passed.

## Desktop overlay acceptance (SPEC-TL-DESKTOP-OVERLAY)

`WindowsDesktopOverlay` presents through the same layered composition and native pointer path as
the attached overlay, without a target window. Its look and its input cannot be checked
automatically; CTest covers only the placement arithmetic. A build or a passing CTest is not
evidence for any item below. Launch `tela-desktop-overlay-probe` through Excubitor from the main
project with a Cc testing claim (it exits by itself after 120 seconds), or run
`tela_desktop_overlay_probe --font <file.ttf> [--corner <corner>] [--seconds <n>]` directly.

1. The probe panel appears in the chosen corner of the primary monitor's work area, 16 logical
   pixels from both edges, above the taskbar. Its rounded backdrop is translucent: the windows and
   wallpaper below stay visible, and text edges show no opaque rectangle.
2. Nothing appears in the taskbar or in Alt+Tab. Starting the probe does not change the focused
   window, and typing continues to reach the application that had focus.
3. Click on the backdrop or its text: the window below receives the click (select text, press a
   button beneath). Click `Clicked N times`: only the count increases, the window below receives
   nothing and keeps its activation state. Drag out of the button before releasing: no count.
4. Hover the grip at the top right and then the button: the `Pointer:` line names each, and returns to
   `elsewhere` after the pointer leaves. Drag the grip: the whole panel follows the pointer. On release
   the probe prints `moved x y corner margin_x margin_y` and the panel shows the same values. Drop
   the panel partly beyond the screen edge: it settles fully inside the work area.
5. With two monitors of different DPI: drag the panel onto the other monitor. After release it is
   drawn at that monitor's scale (sharp text, same logical size). Change the display scale while it
   runs: the panel follows within a second.
6. Drag the panel onto a secondary monitor, then disconnect that monitor while the probe runs: the
   panel returns to the corner it was nearest to on that monitor, now on the primary monitor.
7. At exit the probe prints `gdi before -> after, user before -> after`. USER objects return to the
   starting count. GDI keeps only what the first GDI use allocates for the process. Repeating
   create/destroy must not grow either count.

## Presentation diagnostics (report version 2)

The report records each synchronization decision: presented, unchanged,
viewport_hidden, empty_viewport, target_lost, target_hidden, target_minimized,
foreground_unavailable, or foreign_foreground. `not_observed` means no decision
was sampled, not that a target was lost. Counts are observations, not elapsed
time. The last observation identifies target/foreground process IDs; the report
does not capture window titles or Scene object contents. Zero frames never prove
successful composition. Gates are unchanged by diagnostics. The target probe
still polls at 100 ms; IPC wakes the normal overlay immediately, with a one-second
fallback for native visibility observation. Heartbeats do not submit frames.

The Unity sample now binds only from the selected, focused Scene callback. While
Inspector, a different Scene, or a different app has focus, it hides conservatively.
Focus restoration requests one repaint; a changed native root requires reconnect.
Verify these cases with both docked and floating Scene and bridge windows.

`scripts/verify-bridge-contracts.ps1 -EditorData <Unity 6 Data>` compiles and runs
pure .NET contracts without starting the Editor, overlay, or a listening server.
It covers host-binding eligibility, hidden notification suppression, heartbeat
cadence, restore, and repeated disposal while connecting. CTest also checks
presentation reasons and repeated bridge-generation/gesture cancellation.
These checks do not establish native GDI/capture or GPU lifetime correctness.
