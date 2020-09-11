using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using Sanity.Editor.UI;
using Sanity.Editor.UI.Project;

namespace Sanity.Editor.UI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void NewProject_Click(object sender, RoutedEventArgs e)
        {
            if(IsInitialized)
            {
                // Open the new project wizard
                var projectDialog = new EditProjectInfoWindow(new EditProjectInfoWindowParams { mode = EditProjectInfoWindowMode.CreateNewProject });
                var result = projectDialog.ShowDialog();
                if(result != null && result == true)
                {
                    // Create a project from the information in the window
                }
            } 
        }
    }
}
