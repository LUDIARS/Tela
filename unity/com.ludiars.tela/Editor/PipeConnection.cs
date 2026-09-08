// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
using System;
using System.Collections.Generic;
using System.IO.Pipes;
using System.Security.Principal;
using System.Threading;

namespace Tela.Editor
{
    // The worker owns byte transport only; it never calls Unity or Editor APIs.
    internal sealed class PipeConnection : IDisposable
    {
        private readonly Queue<byte[]> queue = new Queue<byte[]>();
        private readonly object gate = new object();
        private readonly AutoResetEvent wake;
        private readonly Thread worker;
        private readonly string name;
        private NamedPipeClientStream pipe;
        private volatile bool stopped;
        private bool disposed, workerExited;
        internal volatile bool Connected;
        private volatile string error;
        internal string Error { get { return error; } private set { error = value; } }
        internal PipeConnection(string name)
        {
            if (string.IsNullOrWhiteSpace(name) || name.Length > 100) throw new ArgumentException("Invalid Tela pipe name");
            this.name = name;
            wake = new AutoResetEvent(false);
            worker = new Thread(Run) { IsBackground = true, Name = "Tela pipe" };
            try { worker.Start(); }
            catch { wake.Dispose(); throw; }
        }
        internal bool Send(byte[] frame)
        {
            lock (gate)
            {
                if (stopped) return false;
                if (queue.Count >= 256) { Error = "Tela queue overflow; reconnect required"; stopped = true; pipe?.Dispose(); wake.Set(); return false; }
                queue.Enqueue(frame);
                wake.Set();
            }
            return true;
        }
        private void Run()
        {
            try
            {
                var created = new NamedPipeClientStream(".", name, PipeDirection.Out, PipeOptions.Asynchronous, TokenImpersonationLevel.Identification);
                lock (gate) { if (stopped) { created.Dispose(); return; } pipe = created; }
                created.Connect(2000);
                lock (gate) { if (stopped) return; Connected = true; }
                while (!stopped)
                {
                    byte[] next;
                    lock (gate) { next = queue.Count > 0 ? queue.Dequeue() : null; }
                    // Set() from Send/Dispose is never missed: both take gate, and
                    // stopped is re-read after every wake, so this cannot park
                    // past a shutdown that signalled before the wait began.
                    if (next == null) { wake.WaitOne(); continue; }
                    // Close from Dispose interrupts an outstanding asynchronous write.
                    created.WriteAsync(next, 0, next.Length).GetAwaiter().GetResult();
                }
            }
            // An escaping exception on this background thread would terminate the
            // Editor process; surface every failure through Error instead.
            catch (Exception e) { if (!stopped) Error = e.Message; }
            finally
            {
                lock (gate)
                {
                    Connected = false; stopped = true;
                    pipe?.Dispose(); pipe = null; queue.Clear();
                    // The waiting thread owns the event through every exit path,
                    // including exit after the caller's bounded join has timed out.
                    wake.Dispose(); workerExited = true;
                }
            }
        }
        public void Dispose()
        {
            lock (gate)
            {
                if (disposed) return;
                disposed = true; stopped = true; pipe?.Dispose();
                if (!workerExited) wake.Set();
            }
            // Runs on the Editor main thread from beforeAssemblyReload/quitting.
            // Disposing the stream normally aborts a pending write, but a write
            // already accepted by the OS can outlive it; never block the Editor
            // indefinitely. The worker is a background thread, so an abandoned
            // one cannot keep the process alive.
            // The worker's finally block owns event disposal even after timeout.
            if (!worker.Join(2000)) { Error = Error ?? "Tela pipe worker did not stop; abandoned"; return; }
        }
    }
}
