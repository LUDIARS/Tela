# Native probe completed without presenting

- Date: 2026-09-08
- Area: Windows overlay / Unity host lifecycle

## Evidence

Main commit 6cfbe3a, Excubitor probe: 120.611 seconds, CPU 343.75 ms,
frames 0, handles 68 -> 95, GDI 0 -> 0. Computer Use pipe was unavailable.
Startup and exit succeeded; visual/input acceptance did not. This is an initial
acceptance failure, not a demonstrated regression from working presentation.

## Diagnosis and requirements

The old report cannot distinguish foreground suppression, hidden/empty viewport,
lost target, and unchanged content. Record these decisions without weakening
visibility gates. Foreground suppression is a hypothesis, not an observed cause.
Bind Unity only while the selected Scene owns focus; the bridge window must not
become the host. Send hidden state once and stop periodic Scene repaint requests.
Exercise cancellation, reconnect generations, worker/event lifetime and restored
presentation. A process handle delta alone is not proof of a leak.

## Follow-up: explicit probe visibility

Main 1d763b5 completed a 120.962-second run with 0 frames and 1105
`viewport_hidden` observations. Target and foreground PID were both 41648.
This narrows suppression to the probe viewport, but the report did not include
the individual target visibility/minimized flags.

The probe does assign `next.visible` and `next.focused`; an earlier suggestion
that those assignments were missing was a reading error, not a source defect.
Excubitor's breakaway launcher starts the child with `windowsHide: true`.
Win32 consumes STARTUPINFO on the first ShowWindow call, so SW_SHOW on that
first call does not guarantee a visible host. The visual probe now consumes
startup preference and explicitly shows its own window with a second call.
It prints startup flags, startup show state, and resulting visibility. The
overlay report additionally serializes every PresentationObservation boolean.
Overlay visibility gates remain unchanged.

Reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow

This is a source-level correction awaiting an Excubitor run from main; it is
not yet evidence that transparent composition or input acceptance has passed.

## Runtime verification boundary

Pure contract tests and Unity reference compilation can run without a desktop.
Real composition/input/DPI/GPU acceptance still requires the main project through
Excubitor and a Concordia claim. No worktree app startup is permitted.
