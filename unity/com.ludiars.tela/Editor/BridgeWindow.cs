// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
using UnityEditor;
using UnityEngine;

namespace Tela.Editor
{
    internal sealed class BridgeWindow : EditorWindow
    {
        private string pipeName = "tela-scene";
        [MenuItem("Tools/Tela/Scene Bridge")]
        private static void Open() { GetWindow<BridgeWindow>("Tela Scene Bridge"); }
        private void OnGUI()
        {
            pipeName = EditorGUILayout.TextField("Local pipe",pipeName);
            EditorGUILayout.HelpBox(SceneBridge.Status,MessageType.Info);
            if (GUILayout.Button("Connect active Scene view")) SceneBridge.Connect(pipeName);
            if (GUILayout.Button("Disconnect")) SceneBridge.Disconnect();
        }
    }
}
