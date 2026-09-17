// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
using System;

namespace Tela.Editor
{
    // Public adapter boundary; the Scene bridge remains the sole connection owner.
    public static class SceneOverlayConnection
    {
        public static string Status => SceneBridge.Status;
        public static void Connect(string localPipe)
        {
            LocalPipeName.Validate(localPipe);
            SceneBridge.Connect(localPipe);
        }
        public static void Disconnect() => SceneBridge.Disconnect();
    }
}
