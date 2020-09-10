using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;

using Sanity.Editor.UI.Project;

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

namespace Sanity.Editor
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        public void NewProject_Click(object _, RoutedEventArgs e)
        {
            // Show the user the project creation wizard

            Frame rootFrame = Window.Current.Content as Frame;

            // No parameters - the page should not get populated with any data, allowing the user to fill out their project's information
            rootFrame.Navigate(typeof(EditProjectPage), default);
        }

        public void SaveProject_Click(object sender, RoutedEventArgs e)
        {
            // Save all files in the project
        }

        public void OpenProject_Click(object sender, RoutedEventArgs e)
        {
            // Show the user the project launcher, which lets them launch any project that Sanity Engine is aware of 
            // (or select a project folder on their own)
        }

        public void EditProject_Click(object sender, RoutedEventArgs e)
        {
            // Open a dialog to edit project settings
        }

        public void EditWorld_Click(object sender, RoutedEventArgs e)
        {
            // Open a dialog to edit world settings
        }

        public void NewAiSense_Click(object sender, RoutedEventArgs e)
        {
            // Open a dialog to add a new AI sense
        }

        public void EditAiSense_Click(object sender, RoutedEventArgs e)
        {
            // Open a dialog to let the user select an AI Sense to edit
        }

        public void NewAiBehaviorTree_Click(object sender, RoutedEventArgs e)
        {
            // Open a dialog to add a behavior tree
        }

        public void EditAiBehaviorTree_Click(object sender, RoutedEventArgs e)
        {
            // Open a dialog to let the user select a behavior tree to edit
        }
    }
}

/*
 * 
        <WinUI:MenuBar x:Name="MainMenuBar" HorizontalAlignment="Left" VerticalAlignment="Top" TabNavigation="Cycle">
            <WinUI:MenuBarItem Title="File">
                <MenuFlyoutItem Text="New Project" Click="NewProject_Click" />
                <MenuFlyoutItem Text="Save Project" Click="SaveProject_Click" />
                <MenuFlyoutItem Text="Open Project" Click="OpenProject_Click" />
            </WinUI:MenuBarItem>
            <WinUI:MenuBarItem Title="Project">
                <MenuFlyoutItem Text="Edit Project Settings" Click="EditProject_Click" />
            </WinUI:MenuBarItem>
            <WinUI:MenuBarItem Title="World">
                <MenuFlyoutItem Text="Edit World Settings" Click="EditWorld_Click" />
            </WinUI:MenuBarItem>
            <WinUI:MenuBarItem Title="AI">
                <MenuFlyoutSubItem Text="Senses">
                    <MenuFlyoutItem Text="New" Click="NewAiSense_Click" />
                    <MenuFlyoutItem Text="Edit" Click="EditAiSense_Click" />
                </MenuFlyoutSubItem>
                <MenuFlyoutSubItem Text="Behavior Trees">
                    <MenuFlyoutItem Text="New" Click="NewAiBehaviorTree_Click" />
                    <MenuFlyoutItem Text="Edit" Click="EditAiBehaviorTree_Click" />
                </MenuFlyoutSubItem>
            </WinUI:MenuBarItem>
        </WinUI:MenuBar>
 */