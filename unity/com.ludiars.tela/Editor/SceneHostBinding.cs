// @spec Unity bridge
using System;
namespace Tela.Editor
{
    internal static class SceneHostBinding
    {
        // A nonzero foreground HWND alone could be the floating bridge window.
        internal static bool CanBind(bool selectedSceneFocused, IntPtr foreground)
        {
            return selectedSceneFocused && foreground != IntPtr.Zero;
        }
        internal static bool IsCurrent(bool selectedSceneFocused, IntPtr foreground, IntPtr bound)
        {
            return CanBind(selectedSceneFocused, foreground) && foreground == bound;
        }
    }
}
