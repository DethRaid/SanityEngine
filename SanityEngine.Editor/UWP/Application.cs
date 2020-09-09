using Sanity;

using System;

using Windows.ApplicationModel;
using Windows.ApplicationModel.Activation;
using Windows.ApplicationModel.Core;
using Windows.UI.Core;

namespace Sanity.Editor
{
    public class UwpApp : IFrameworkView, IDisposable
    {
        private static string projectName;

        private SanityEditor? editor = null;

        private bool windowClosed = false;
        private bool windowVisible = false;

        [MTAThread]
        private static int Main(string[] args)
        {
            projectName = args[0];

            var sanityEngineApplicationSource = new SanityEngineApplicaionSource();
            CoreApplication.Run(sanityEngineApplicationSource);
            return ReturnCodes.Success;
        }

        public void Initialize(CoreApplicationView applicationView)
        {
            applicationView.Activated += OnActivated;
            CoreApplication.Suspending += OnSuspending;
            CoreApplication.Resuming += OnResuming;
        }

        public void Load(string entryPoint)
        {
            if(editor is null)
            {
                editor = new(projectName);
            }
        }

        public void Run()
        {
            while(!windowClosed)
            {
                if(windowVisible)
                {
                    CoreWindow.GetForCurrentThread().Dispatcher.ProcessEvents(CoreProcessEventsOption.ProcessAllIfPresent);
                }
                else
                {
                    CoreWindow.GetForCurrentThread().Dispatcher.ProcessEvents(CoreProcessEventsOption.ProcessOneAndAllPending);
                }

                editor?.Tick(windowVisible);
            }
        }

        public void SetWindow(CoreWindow window)
        {
            window.SizeChanged += OnWindowSizeChanged;
            window.VisibilityChanged += OnWindowVisibilityChanged;
            window.Closed += OnWindowClosed;

            // TODO: idk man
            // application?.SetRenderSurface(window.swapChain);
        }

        public void Uninitialize() => throw new NotImplementedException();

        private void OnActivated(CoreApplicationView sender, IActivatedEventArgs args)
        {
            // TODO: Other activation logic?
            CoreWindow.GetForCurrentThread().Activate();
        }

        private void OnSuspending(object? sender, SuspendingEventArgs e)
        {
            SuspendingDeferral deferral = e.SuspendingOperation.GetDeferral();

            // TODO: do the thing

            deferral.Complete();
        }

        private void OnResuming(object? sender, object e)
        {
            // TODO: Do the thing
        }

        private void OnWindowSizeChanged(CoreWindow sender, WindowSizeChangedEventArgs args)
        {
            // Resize the swapchain and any swapchain-relative sized render targets
            editor.SetWindowSize(args.Size);
        }

        private void OnWindowClosed(CoreWindow sender, CoreWindowEventArgs args)
        {
            windowClosed = true;
        }

        private void OnWindowVisibilityChanged(CoreWindow sender, VisibilityChangedEventArgs args)
        {
            windowVisible = args.Visible;
        }

        public void Dispose() => ((IDisposable)editor).Dispose();

        private static class ReturnCodes
        {
            public const int Success = 0;
        }
    }

    internal class SanityEngineApplicaionSource : IFrameworkViewSource
    {
        public IFrameworkView CreateView()
        {
            return new UwpApp();
        }
    }
}
