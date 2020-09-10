using System;
using System.Collections.Generic;

using Windows.Foundation;

using Windows.System.Threading;

namespace Sanity.Concurrency
{
    class WinRTNursery : IDisposable
    {
        private List<IAsyncAction> activeActions = new();

        public void StartSoon(Action taskFunction)
        {
            var asyncAction = ThreadPool.RunAsync(workItem => taskFunction());
            activeActions.Add(asyncAction);
        }

        #region IDisposable
        private bool disposedValue;

        protected virtual void Dispose(bool disposing)
        {
            if(!disposedValue)
            {
                if(disposing)
                {
                    // TODO: dispose managed state (managed objects)

                    foreach(var action in activeActions)
                    {
                        action.AsTask().Wait();
                    }
                }

                // TODO: free unmanaged resources (unmanaged objects) and override finalizer
                // TODO: set large fields to null
                disposedValue = true;
            }
        }

        // // TODO: override finalizer only if 'Dispose(bool disposing)' has code to free unmanaged resources
        // ~WinRTNursery()
        // {
        //     // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
        //     Dispose(disposing: false);
        // }

        public void Dispose()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }
        #endregion
    }
}
