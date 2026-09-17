# Tela bridge protocol v1

The transport is one local Windows byte-mode named pipe (`tela-scene` by default).
The server creates the first instance, rejects remote clients and grants access
only to the current Windows user SID. Hello's HWND must belong to the client PID
reported by Windows. This authenticates the local user/process boundary, not an
application signature; another application running as the same user is trusted.
Do not expose this protocol through a network proxy.

Each frame starts with a u32 **little-endian** payload length, at most 65536 bytes.
No native struct layout, platform-endian writes, newline conversion or BOM is used.
The common payload is:

| Field | Encoding |
|---|---|
| magic | u32 `0x31574c54` (`TLW1`) |
| version | u16 `1` |
| kind | u16 |
| connection generation | u64, nonzero |
| sequence | u64, nonzero, strictly increasing |
| viewport revision | u64 |
| host identity | u16 UTF-8 byte length followed by bytes |
| view identity | same |

Strings are at most 4096 bytes, valid UTF-8 without NUL. Collections are at most
256 entries. Integers are little-endian, coordinates signed i32 physical desktop
pixels (negative origins supported), floats IEEE754 binary32, booleans u8 0/1.

| Kind | Payload after common fields |
|---|---|
| 1 hello | u64 host HWND; first message of a new transport connection only |
| 2 viewport | i32 x, y, width, height; f32 DPI scale; bool visible, focused |
| 3 pointer | u64 gesture; u8 phase, button; i32 x,y; f32 wheel x,y; u32 modifiers |
| 4 selection | u16 count, then stable object ID strings |
| 5 anchors | u16 count, then object ID, label, i32 x,y, bool visible per entry |
| 6 heartbeat | empty |

Pointer phase: move=0, down=1, up=2, cancel=3, wheel=4. Button: none=0,
primary=1, secondary=2, middle=3. Only primary down/up activate buttons.
Host input is observed; it is never replayed or retroactively consumed. Exclusive
input arrives through native windows, shared input through the pipe. Duplicate,
foreign-generation and stale-revision events do not execute actions.

Transport delivery holds at most 256 messages. Overflow disconnects and discards
that connection's queued events rather than losing a down/up ordering constraint.
Three seconds without a complete read disconnects; Unity sends one heartbeat per
second. Destruction cancels pending overlapped I/O and joins the owning worker.
One connection tracks one selected Scene view; switching views requires Connect
again and establishes a new generation. Selection IDs use Unity GlobalObjectId.

Unity never sends native draw commands. Orbis and Iter can implement this same
protocol or link `Tela::Core`/`Tela::Windows` directly without a Unity dependency.

## Editor connection API

`Tela.Editor.SceneOverlayConnection` exposes `Connect(localPipe)`, `Disconnect()`
and read-only `Status` to optional Editor adapters such as Praeforma. It delegates
to the existing Scene bridge and preserves its one-connection ownership and reload /
quit cleanup. Invalid pipe names are rejected before disconnecting an existing
connection. Names are nonempty local pipe identifiers of at most 128 characters;
letters, digits, dash, underscore and dot are accepted (except a lone dot).
The adapter never launches the native process or interprets specification/instruction
text. Native startup remains an Excubitor operation from the main checkout.
