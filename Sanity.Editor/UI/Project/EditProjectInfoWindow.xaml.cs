using System;
using System.Collections.Generic;
using System.IO;
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
using System.Windows.Shapes;

using Ookii.Dialogs.Wpf;

namespace Sanity.Editor.UI.Project
{
    /// <summary>
    /// Interaction logic for EditProjectInfoWindow.xaml
    /// </summary>
    public partial class EditProjectInfoWindow : Window
    {
        private bool isProjectFolderSelected = false;

        private string projectName = "NewProject";

        private string projectParentDirectory = "";

        public EditProjectInfoWindow()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Shows a folder picker that allows the user to select a directory for 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ProjectSelectDirectory_Click(object sender, RoutedEventArgs e)
        {
            if(IsInitialized)
            {
                var folderPicker = new VistaFolderBrowserDialog();

                var folderSelected = folderPicker.ShowDialog();
                if(folderSelected != null && folderSelected == true)
                {
                    // Select the project folder that the user selected

                    projectParentDirectory = folderPicker.SelectedPath;

                    if(Directory.Exists(projectParentDirectory))
                    {
                        UpdateProjectDirectoryLabel();
                        isProjectFolderSelected = true;
                    }
                    else
                    {
                        // Show an error that the directory is not valid
                    }
                }
            }
        }

        private void UpdateProjectDirectoryLabel() => ProjectDirectoryLabel.Text = string.Format("{0}\\{1}", projectParentDirectory, projectName);

        private void ProjectNameTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if(IsInitialized)
            {
                projectName = ProjectNameTextBox.Text;
                UpdateProjectDirectoryLabel();
            }
        }
    }
}
