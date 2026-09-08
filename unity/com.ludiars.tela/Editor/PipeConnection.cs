// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
using System;
using System.Collections.Generic;
using System.IO;
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
        private readonly AutoResetEvent wake = new AutoResetEvent(false);
        private readonly Thread worker;
        private readonly string name;
        private NamedPipeClientStream pipe;
        private volatile bool stopped;
        internal volatile bool Connected;
        internal string Error { get; private set; }
        internal PipeConnection(string name)
        {
            if (string.IsNullOrWhiteSpace(name) || name.Length > 100) throw new ArgumentException("Invalid Tela pipe name");
            this.name = name;
            worker = new Thread(Run) { IsBackground = true, Name = "Tela pipe" }; worker.Start();
        }
        internal bool Send(byte[] frame)
        {
            lock (gate)
            {
                if (stopped) return false;
                if (queue.Count >= 256) { Error = "Tela queue overflow; reconnect required"; stopped = true; pipe?.Dispose(); wake.Set(); return false; }
                queue.Enqueue(frame);
            }
            wake.Set(); return true;
        }
        private void Run()
        {
            try
            {
                var created = new NamedPipeClientStream(".", name, PipeDirection.Out, PipeOptions.Asynchronous, TokenImpersonationLevel.Identification);
                lock (gate) { if (stopped) { created.Dispose(); return; } pipe = created; }
                created.Connect(2000); Connected = true;
                while (!stopped)
                {
                    byte[] next;
                    lock (gate) { next = queue.Count > 0 ? queue.Dequeue() : null; }
                    if (next == null) { wake.WaitOne(500); continue; }
                    // Close from Dispose interrupts an outstanding asynchronous write.
                    created.WriteAsync(next, 0, next.Length).GetAwaiter().GetResult();
                }
            }
            catch (Exception e) when (e is IOException || e is TimeoutException || e is ObjectDisposedException || e is UnauthorizedAccessException)
            { if (!stopped) Error = e.Message; }
            finally { Connected = false; stopped = true; lock (gate) { pipe?.Dispose(); pipe = null; queue.Clear(); } }
        }
        public void Dispose()
        {
            stopped = true; lock (gate) { pipe?.Dispose(); } wake.Set();
            // Runs on the Editor main thread from beforeAssemblyReload/quitting.
            // Disposing the stream normally aborts a pending write, but a write
            // already accepted by the OS can outlive it; never block the Editor
            // indefinitely. The worker is a background thread, so an abandoned
            // one cannot keep the process alive.
            // An abandoned worker still waits on wake, so disposing the handle
            // here would fault it; leave the handle to finalization in that case.
            if (!worker.Join(2000)) { Error = Error ?? "Tela pipe worker did not stop; abandoned"; return; }
            wake.Dispose();
        }
    }
}
