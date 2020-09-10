using System;

using Optional.Unsafe;

using Sanity.Editor.Project;

using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace Sanity.Editor.UI.Project
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class EditProjectPage : Page
    {
        private StorageFolder? folder;

        public EditProjectPage()
        {
            this.InitializeComponent();
        }

        private async void ChooseDirectoryButton_OnClick(object sender, RoutedEventArgs e)
        {
            // Open a folder picker of some sort
            var picker = new FolderPicker();
            picker.ViewMode = PickerViewMode.List;
            picker.SuggestedStartLocation = PickerLocationId.DocumentsLibrary;
            picker.FileTypeFilter.Add("*");
            folder = await picker.PickSingleFolderAsync();
        }

        private async void CreateProject_Click(object sender, RoutedEventArgs e)
        {
            var projectTitle = ProjectTitleTextBox.Text;

            var newProjectInfo = ProjectInfo.Create(projectTitle, folder);

            var app = Application.Current as App;
            var result = await app.Editor.CreateProject(newProjectInfo);
            if(result.HasValue)
            {
                // Show an error to the user
                var errorDialog = new ContentDialog
                {
                    Title = "Could not create project",
                    Content = result.ValueOrDefault(),
                    CloseButtonText = "Ok"
                };

                await errorDialog.ShowAsync();
            }
            else
            {
                // Load the project and go back to the main page
                // app.Editor.LoadProject(newProjectInfo);
            }
        }
    }
}
