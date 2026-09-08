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

## Verification boundary

Pure contract tests and Unity reference compilation can run without a desktop.
Real composition/input/DPI/GPU acceptance still requires the main project through
Excubitor and a Concordia claim. No worktree app startup is permitted.
