using System;
using System.IO;
using System.Text.Json;
using System.Threading.Tasks;

using NLog;

using Optional;

using Sanity.Editor.Project;
using Sanity.Editor.Storage;

using Windows.Storage;

namespace Sanity.Editor
{
    public class SanityEditor
    {
        private static readonly Logger log = LogManager.GetCurrentClassLogger();

        /// <summary>
        /// Creates a new project with the specified info
        /// </summary>
        /// <param name="info">Information about the new project</param>
        /// <returns>A user-friendly error string, if creating a project failed, or Option.None() if the project was successfully created</returns>
        public async Task<Option<string>> CreateProject(ProjectInfo info)
        {
            var trimmedName = info.Name.Trim();
            if(trimmedName.Length == 0)
            {
                return Option.Some("Project name has no value");
            }

            if(info.ProjectParentFolder == null)
            {
                return Option.Some("No project folder selected");
            }

            var projectFolder = await info.ProjectParentFolder.CreateFolderSafeAsync(trimmedName);
            if(projectFolder == null)
            {
                return Option.Some("Project directory already exists");
            }

            Console.WriteLine("Creating project {0} in directory {1}", trimmedName, info.ProjectParentFolder.Name);

            // TODO: Copy the template project into the new project directory

            await projectFolder.CreateFolderSafeAsync(ProjectInfo.ContentDirectory);
            await projectFolder.CreateFolderSafeAsync(ProjectInfo.SourceDirectory);
            await projectFolder.CreateFolderSafeAsync(ProjectInfo.BuildDirectory);
            await projectFolder.CreateFolderSafeAsync(ProjectInfo.CacheDirectory);
            await projectFolder.CreateFolderSafeAsync(ProjectInfo.UserDataDrectory);

            // Assume these directories got created correctly #yolo

            // Create project info file
            try
            {
                var projectInfoFile = await projectFolder.CreateFileAsync(string.Format("{0}.sanityproject", trimmedName), CreationCollisionOption.ReplaceExisting);
                var json = JsonSerializer.Serialize(info);
                await FileIO.WriteTextAsync(projectInfoFile, json);
            }
            catch(Exception e)
            {
                return Option.Some(e.Message);
            }


            return Option.None<string>();
        }

        internal void LoadProject(ProjectInfo newProjectInfo) => throw new System.NotImplementedException();
    }
}
