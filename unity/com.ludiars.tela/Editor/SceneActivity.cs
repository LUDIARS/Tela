// @spec Unity bridge
namespace Tela.Editor
{
    // Only transitions request repaint/hide. Heartbeats never repaint the Scene.
    internal sealed class SceneActivity
    {
        private bool active;
        private double heartbeat;
        internal bool Changed(bool available)
        {
            if (active == available) return false;
            active = available;
            return true;
        }
        internal bool HeartbeatDue(double now)
        {
            if (now - heartbeat < 1) return false;
            heartbeat = now;
            return true;
        }
    }
}
