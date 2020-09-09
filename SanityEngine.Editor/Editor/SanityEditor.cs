using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Sanity.Concurrency;

using Windows.Foundation;

namespace Sanity.Editor
{
    class SanityEditor : IDisposable
    {
        public SanityEngine Engine
        {
            get; private set;
        }

        private WinRTNursery nursery = new();

        public SanityEditor(string projectDirectory)
        {
            nursery.StartSoon(RunUi);

            // Kick off tasks to scan for the world and environment definition files
            nursery.StartSoon(BeginLoadWorld);

            // Then kick off tasks to load structures like villages and cities
            // Next, kick off tasks for animals, characters, and their AI
            // That's probably close to it
        }

        public void Tick(bool windowVisible)
        {
            Engine.Tick(windowVisible);
        }

        public void SetWindowSize(Size size)
        {
        }

        public void Dispose() => ((IDisposable)nursery).Dispose();

        /// <summary>
        /// Does the needful to keep the UI chugging along. Runs in a separate thread. Yay.
        /// </summary>
        private void RunUi()
        {
        }

        private void BeginLoadWorld()
        {

        }
    }
}
