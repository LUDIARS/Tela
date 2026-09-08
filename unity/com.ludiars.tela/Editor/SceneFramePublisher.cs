// @spec Unity bridge
using System;
using System.Linq;
using UnityEditor;
using UnityEngine;
namespace Tela.Editor
{
    // Owns change detection and conversion of one Scene snapshot into wire messages.
    internal sealed class SceneFramePublisher
    {
        private readonly IntPtr host;
        private readonly BridgeWire wire;
        private readonly Action<byte[]> Send;
        private string geometry, anchors, selection;
        private ulong gesture;
        internal SceneFramePublisher(IntPtr host, BridgeWire wire, Action<byte[]> send)
        {
            this.host = host; this.wire = wire; Send = send;
        }
        internal void Hide()
        {
            ++wire.Revision;
            Send(wire.Message(2, w => { w.Write(0); w.Write(0); w.Write(0); w.Write(0); w.Write(1f); w.Write(false); w.Write(false); }));
            geometry = anchors = selection = null;
        }
        internal void Publish(SceneView current)
        {
            var camera = current.camera; if (!camera) return;
            var origin = DesktopCoordinates.Physical(host, Vector2.zero);
            float scale = EditorGUIUtility.pixelsPerPoint;
            int width = camera.pixelWidth, height = camera.pixelHeight;
            // Called only from the attached, focused Scene callback.
            bool focused = true;
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
