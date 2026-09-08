// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using UnityEditor;
using UnityEngine;

namespace Tela.Editor
{
    internal static class DesktopCoordinates
    {
        [StructLayout(LayoutKind.Sequential)] private struct Point { public int X, Y; }
        [DllImport("user32.dll")] private static extern IntPtr GetForegroundWindow();
        [DllImport("user32.dll")] private static extern IntPtr GetAncestor(IntPtr window, uint flags);
        [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint pid);
        [DllImport("user32.dll")] private static extern bool LogicalToPhysicalPointForPerMonitorDPI(IntPtr window, ref Point point);
        internal static IntPtr ForegroundHost()
        {
            var window = GetAncestor(GetForegroundWindow(), 2); GetWindowThreadProcessId(window, out uint pid);
            return pid == (uint)Process.GetCurrentProcess().Id ? window : IntPtr.Zero;
        }
        internal static Vector2Int Physical(IntPtr host, Vector2 guiPosition)
        {
            Vector2 screen = GUIUtility.GUIToScreenPoint(guiPosition);
            var point = new Point { X = Mathf.RoundToInt(screen.x), Y = Mathf.RoundToInt(screen.y) };
            // GUIToScreenPoint gives desktop coordinates, not a viewport-relative
            // vector. Do not scale negative/mixed-DPI desktop origins by pixelsPerPoint.
            if (!LogicalToPhysicalPointForPerMonitorDPI(host, ref point))
                throw new InvalidOperationException("Cannot convert Unity desktop coordinates to physical pixels");
            return new Vector2Int(point.X, point.Y);
        }
    }
}
