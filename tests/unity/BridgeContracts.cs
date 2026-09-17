// @spec Unity bridge
using System;
using System.Reflection;
using System.Threading;
using Tela.Editor;
internal static class BridgeContracts
{
    private static void Require(bool value, string message)
    {
        if (!value) throw new Exception(message);
    }
    public static int Main()
    {
        try
        {
            LocalPipeName.Validate("tela-scene");
            foreach (var invalid in new[] { "", ".", "../pipe", "\\\\host\\pipe", new string('a', 129) })
            {
                bool rejected = false;
                try { LocalPipeName.Validate(invalid); }
                catch (ArgumentException) { rejected = true; }
                Require(rejected, "Invalid pipe name rejected before changing connection");
            }
            var scene = new IntPtr(10); var bridgeWindow = new IntPtr(20);
            Require(!SceneHostBinding.CanBind(false, bridgeWindow), "Floating bridge must not bind as Scene");
            Require(!SceneHostBinding.CanBind(true, IntPtr.Zero), "Foreign foreground cannot bind");
            Require(SceneHostBinding.IsCurrent(true, scene, scene), "Focused Scene binds");
            Require(!SceneHostBinding.IsCurrent(true, bridgeWindow, scene), "Moved Scene requires a new host");
            var activity = new SceneActivity();
            Require(activity.Changed(true), "First observation enables Scene");
            Require(activity.Changed(false), "Hide once");
            for (int i = 0; i < 1000; ++i) Require(!activity.Changed(false), "Hidden repeats must not notify/repaint");
            Require(activity.HeartbeatDue(10), "Hidden connection heartbeat");
            Require(!activity.HeartbeatDue(10.5), "Heartbeat is bounded");
            Require(activity.Changed(true) && !activity.Changed(true), "Restore repaints only once");
            Require(activity.HeartbeatDue(11), "Next heartbeat");
            for (int i = 0; i < 16; ++i)
            {
                var connection = new PipeConnection("tela-test-absent-" + Guid.NewGuid().ToString("N"));
                var worker = (Thread)typeof(PipeConnection).GetField("worker", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(connection);
                var wake = (AutoResetEvent)typeof(PipeConnection).GetField("wake", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(connection);
                connection.Send(new byte[] { 0 });
                connection.Dispose(); connection.Dispose();
                Require(worker.Join(3000), "Canceled connect worker must exit");
                Require(wake.SafeWaitHandle.IsClosed, "Worker must dispose its wait handle");
                Require(!connection.Send(new byte[] { 0 }) && !connection.Connected, "Disposed transport rejects data");
            }
            var failed = new PipeConnection("tela-test-timeout-" + Guid.NewGuid().ToString("N"));
            var failedWorker = (Thread)typeof(PipeConnection).GetField("worker", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(failed);
            var failedWake = (AutoResetEvent)typeof(PipeConnection).GetField("wake", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(failed);
            Require(failedWorker.Join(5000), "Failed connect must terminate itself");
            Require(failed.Error != null && failedWake.SafeWaitHandle.IsClosed, "Failure releases event without caller disposal");
            failed.Dispose(); failed.Dispose();
            Console.WriteLine("Unity bridge contracts passed (binding, suspension, 16 canceled connects, connect timeout)");
            return 0;
        }
        catch (Exception e) { Console.Error.WriteLine(e); return 1; }
    }
}
