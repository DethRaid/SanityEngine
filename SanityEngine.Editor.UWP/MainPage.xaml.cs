using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;

using Microsoft.Toolkit.Uwp.UI.Controls;

using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

using WinUIMenuBarItem = Microsoft.UI.Xaml.Controls.MenuBarItem;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace SanityEngineEditor
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public List<MenuItem> MainMenuItems
        {
            get; private set;
        }

        public MainPage()
        {
            this.InitializeComponent();

            MainMenuBar.Items.Add(new WinUIMenuBarItem { Name = "File" });
            MainMenuBar.Items.Add(new WinUIMenuBarItem { Name = "Project" });
            MainMenuBar.Items.Add(new WinUIMenuBarItem { Name = "Environment" });
            MainMenuBar.Items.Add(new WinUIMenuBarItem { Name = "Help" });
        }
    }
}
