// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
using System;
using System.Linq;
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
        private static string geometry, anchors, selection;
        private static ulong gesture;
        private static double lastHeartbeat, lastScene;
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
            host = DesktopCoordinates.ForegroundHost();
            if (host == IntPtr.Zero) throw new InvalidOperationException("Unity must be the foreground application");
            wire = new BridgeWire(Application.dataPath, view.GetInstanceID().ToString());
            connection = new PipeConnection(pipeName);
            connection.Send(wire.Message(1, w => w.Write((ulong)host.ToInt64())));
            geometry = anchors = selection = null; gesture = 0; lastScene = lastHeartbeat = EditorApplication.timeSinceStartup;
            SceneView.duringSceneGui += Observe; EditorApplication.update += Update;
            Selection.selectionChanged += SelectionChanged; view.Repaint(); Status = "Connecting";
        }
        internal static void Disconnect()
        {
            SceneView.duringSceneGui -= Observe; EditorApplication.update -= Update;
            Selection.selectionChanged -= SelectionChanged;
            connection?.Dispose(); connection = null; wire = null; view = null; Status = "Disconnected";
        }
        private static void SelectionChanged() { if (view) view.Repaint(); }
        private static void Update()
        {
            if (connection == null) return;
            if (connection.Error != null) { var error = connection.Error; Disconnect(); Status = error; return; }
            if (!view) { Disconnect(); return; }
            double now = EditorApplication.timeSinceStartup;
            if (now - lastHeartbeat < 1) return;
            lastHeartbeat = now;
            // An unhandled throw here would repeat every editor tick and leave the
            // connection registered; drop the connection instead, as Observe does.
            try
            {
                // A closed/covered dock tab stops producing Scene GUI events. Ask for
                // one repaint per second; timeout hides the native surface meanwhile.
                if (now-lastScene > 1.5) SendVisibility(false);
                Send(wire.Message(6, null)); view.Repaint();
                Status = connection.Connected ? "Connected" : "Connecting";
            }
            catch (Exception e) { Disconnect(); Status = e.Message; Debug.LogException(e); }
        }
        private static void Send(byte[] frame)
        {
            if (!connection.Send(frame)) throw new InvalidOperationException("Tela connection stopped; reconnect from Tools/Tela");
        }
        private static void SendVisibility(bool visible)
        {
            ++wire.Revision;
            Send(wire.Message(2, w => { w.Write(0); w.Write(0); w.Write(0); w.Write(0); w.Write(1f); w.Write(visible); w.Write(false); }));
            geometry = null;
        }
        private static void Observe(SceneView current)
        {
            if (current != view || connection == null) return;
            try { ObserveFrame(current); }
            catch (Exception e) { Disconnect(); Status = e.Message; Debug.LogException(e); }
        }
        private static void ObserveFrame(SceneView current)
        {
            lastScene = EditorApplication.timeSinceStartup;
            var camera = current.camera; if (!camera) return;
            var origin = DesktopCoordinates.Physical(host, Vector2.zero);
            float scale = EditorGUIUtility.pixelsPerPoint;
            int width = camera.pixelWidth, height = camera.pixelHeight;
            bool focused = DesktopCoordinates.ForegroundHost() != IntPtr.Zero;
            string next = $"{origin.x},{origin.y},{width},{height},{scale:R},{focused}";
            if (next != geometry)
            {
                geometry = next; ++wire.Revision;
                Send(wire.Message(2, w => { w.Write(origin.x); w.Write(origin.y); w.Write(width); w.Write(height); w.Write(scale); w.Write(true); w.Write(focused); }));
                anchors = selection = null;
            }
            var selected = Selection.gameObjects.Take(256).ToArray();
            var ids = selected.Select(x => GlobalObjectId.GetGlobalObjectIdSlow(x).ToString()).ToArray();
            string selectionKey = string.Join("\n", ids);
            if (selection != selectionKey)
            {
                selection = selectionKey;
                Send(wire.Message(4, w => { w.Write((ushort)ids.Length); foreach (var id in ids) BridgeWire.Text(w, id); }));
            }
            var positions = selected.Select(x => DesktopCoordinates.Physical(host, HandleUtility.WorldToGUIPoint(x.transform.position))).ToArray();
            var visible = selected.Select(x => camera.WorldToViewportPoint(x.transform.position).z > 0).ToArray();
            string anchorKey = selectionKey + string.Join(";", positions.Select(x => x.ToString())) + string.Join(";", selected.Select(x=>x.name)) + string.Join(";",visible.Select(x=>x.ToString()));
            if (anchors != anchorKey)
            {
                anchors = anchorKey;
                Send(wire.Message(5, w => { w.Write((ushort)selected.Length); for (int i=0;i<selected.Length;++i) { BridgeWire.Text(w,ids[i]); BridgeWire.Text(w,selected[i].name); w.Write(positions[i].x); w.Write(positions[i].y); w.Write(visible[i]); } }));
            }
            var ev = Event.current;
            byte phase;
            switch (ev.type) { case EventType.MouseMove: case EventType.MouseDrag: phase=0;break; case EventType.MouseDown: phase=1;++gesture;break;case EventType.MouseUp:phase=2;break;case EventType.ScrollWheel:phase=4;break;default:return; }
            var pointer = DesktopCoordinates.Physical(host,ev.mousePosition);
            byte button = ev.button==0?(byte)1:ev.button==1?(byte)2:ev.button==2?(byte)3:(byte)0;
            Send(wire.Message(3,w=>{w.Write(gesture);w.Write(phase);w.Write(button);w.Write(pointer.x);w.Write(pointer.y);w.Write(ev.delta.x);w.Write(ev.delta.y);w.Write((uint)ev.modifiers);}));
            // Observation only: no Event.Use(), SendInput(), or synthetic replay.
        }
    }
}
