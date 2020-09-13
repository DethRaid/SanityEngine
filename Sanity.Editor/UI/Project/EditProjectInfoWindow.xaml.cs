using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Printing;
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

using Microsoft.VisualBasic.CompilerServices;

using Ookii.Dialogs.Wpf;

using Sanity.Editor.Project;

namespace Sanity.Editor.UI.Project
{
    public enum EditProjectInfoWindowMode
    {
        CreateNewProject,
        EditExistingProject,
    }

    public struct EditProjectInfoWindowParams
    {
        public EditProjectInfoWindowMode mode;
    }

    /// <summary>
    /// Interaction logic for EditProjectInfoWindow.xaml
    /// </summary>
    public partial class EditProjectInfoWindow : Window
    {
        public ProjectInfo Info
        {
            get; private set;
        }

        private readonly EditProjectInfoWindowMode mode;

        private bool isProjectFolderSelected = false;

        private string projectName = "NewProject";

        private string projectParentDirectory = "";

        public EditProjectInfoWindow(EditProjectInfoWindowParams parameters)
        {
            InitializeComponent();

            mode = parameters.mode;
            switch(mode)
            {
                case EditProjectInfoWindowMode.CreateNewProject:
                    PageTitle.Text = "Create Project";
                    CloseWizardButton.Content = "Create";
                    break;
                case EditProjectInfoWindowMode.EditExistingProject:
                    PageTitle.Text = "Edit Project";
                    CloseWizardButton.Content = "Save";
                    break;
            }
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

        private void CloseWizardButton_Click(object sender, RoutedEventArgs e)
        {
            var isValid = true;
            // Validate the data
            var trimmedName = projectName.Trim();
            if(trimmedName.Length == 0)
            {
                ProjectNameErrorMessage.Visibility = Visibility.Visible;
                isValid = false;
            }

            var trimmedProjectAuthor = ProjectAuthorTextBox.Text.Trim();
            if(trimmedProjectAuthor.Length == 0)
            {
                ProjectAuthorErrorMessage.Text = "Project author field is empty";
                isValid = false;
            }

            if(!isProjectFolderSelected)
            {
                ProjectDirectoryErrorMessage.Text = "Must select a project directory";
                ProjectDirectoryErrorMessage.Visibility = Visibility.Visible;
                isValid = false;
            }

            var trimmedParentDirectory = projectParentDirectory.Trim();
            var projectDirectory = string.Format("{0}\\{1}", trimmedParentDirectory, trimmedName);
            if(projectDirectory.Length == 0)
            {
                ProjectDirectoryErrorMessage.Text = "Project directory field is empty";
                ProjectDirectoryErrorMessage.Visibility = Visibility.Visible;
                isValid = false;
            }

            // Check if the project directory already exists
            if(Directory.Exists(projectDirectory))
            {
                ProjectDirectoryErrorMessage.Text = "Project directory already exists";
                ProjectDirectoryErrorMessage.Visibility = Visibility.Visible;
                isValid = false;
            }

            DialogResult = isValid;

            if(isValid)
            {
                Info = new ProjectInfo(projectName, projectDirectory, trimmedProjectAuthor, DateTime.Now);

                // Close ourself
                Close();
            }
        }
    }
}
