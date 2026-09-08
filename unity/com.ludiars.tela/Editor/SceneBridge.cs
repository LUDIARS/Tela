// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
using System;
using UnityEditor;
using UnityEngine;

namespace Tela.Editor
{
    [InitializeOnLoad]
    internal static class SceneBridge
    {
        private static PipeConnection connection;
        private static BridgeWire wire;
        private static SceneView view;
        private static IntPtr host;
        private static SceneFramePublisher publisher;
        private static string pendingPipe;
        private static SceneActivity activity = new SceneActivity();
        internal static string Status { get; private set; } = "Disconnected";
        static SceneBridge()
        {
            AssemblyReloadEvents.beforeAssemblyReload += Disconnect;
            EditorApplication.quitting += Disconnect;
        }
        internal static void Connect(string pipeName)
        {
            Disconnect();
            if (Application.platform != RuntimePlatform.WindowsEditor) throw new NotSupportedException("Tela native bridge currently requires Windows");
            view = SceneView.lastActiveSceneView;
            if (!view) throw new InvalidOperationException("Open and focus a Scene view first");
            pendingPipe = pipeName;
            activity = new SceneActivity();
            SceneView.duringSceneGui += Observe; EditorApplication.update += Update;
            Selection.selectionChanged += SelectionChanged;
            // Binding is deferred to this Scene's GUI callback after focus changes.
            view.Focus(); view.Repaint(); Status = "Waiting for selected Scene focus";
        }
        internal static void Disconnect()
        {
            SceneView.duringSceneGui -= Observe; EditorApplication.update -= Update;
            Selection.selectionChanged -= SelectionChanged;
            connection?.Dispose(); connection = null; wire = null; view = null;
            host = IntPtr.Zero; pendingPipe = null; publisher = null; activity = new SceneActivity(); Status = "Disconnected";
        }
        private static void SelectionChanged() { if (view) view.Repaint(); }
        private static void Update()
        {
            if (!view) { Disconnect(); return; }
            if (connection == null) return;
            try { MaintainConnection(); }
            catch (Exception e) { Fail(e); }
        }
        private static void MaintainConnection()
        {
            if (connection.Error != null) throw new InvalidOperationException(connection.Error);
            UpdateActivity();
            if (activity.HeartbeatDue(EditorApplication.timeSinceStartup)) Send(wire.Message(6, null));
            Status = connection.Connected ? "Connected" : "Connecting";
        }
        private static void UpdateActivity()
        {
            bool available = SceneHostBinding.IsCurrent(EditorWindow.focusedWindow == view,
                DesktopCoordinates.ForegroundHost(), host);
            if (!activity.Changed(available)) return;
            if (available) view.Repaint();
            else publisher.Hide();
        }
        private static void Fail(Exception error)
        {
            Disconnect(); Status = error.Message; Debug.LogException(error);
        }
        private static void Send(byte[] frame)
        {
            if (!connection.Send(frame)) throw new InvalidOperationException("Tela connection stopped; reconnect from Tools/Tela");
        }
        private static void Observe(SceneView current)
        {
            if (current != view) return;
            try { ProcessScene(current); }
            catch (Exception e) { Fail(e); }
        }
        private static void ProcessScene(SceneView current)
        {
            if (!AttachFocusedScene(current)) return;
            activity.Changed(true);
            ObserveFrame(current);
        }
        private static bool AttachFocusedScene(SceneView current)
        {
            var foreground = DesktopCoordinates.ForegroundHost();
            if (!SceneHostBinding.CanBind(EditorWindow.focusedWindow == current, foreground)) return false;
            if (connection == null) Bind(foreground);
            if (foreground != host) throw new InvalidOperationException("Scene host changed; reconnect from Tools/Tela");
            return true;
        }
        private static void Bind(IntPtr foreground)
        {
            host = foreground;
            wire = new BridgeWire(Application.dataPath, view.GetInstanceID().ToString());
            connection = new PipeConnection(pendingPipe);
            // Publish before the hello can throw, so no tick observes a bound
            // connection without the publisher that Hide()/Publish() require.
            publisher = new SceneFramePublisher(host, wire, Send);
            Send(wire.Message(1, w => w.Write((ulong)host.ToInt64())));
            Status = "Connecting";
        }
        private static void ObserveFrame(SceneView current)
        {
            publisher.Publish(current);
        }
    }
}
