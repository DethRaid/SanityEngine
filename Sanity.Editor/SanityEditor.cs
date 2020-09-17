using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

using Sanity;

using NLog;

using Sanity.Editor.Project;
using System.Windows;
using System.Windows.Controls;

namespace Sanity.Editor
{
    public class SanityEditor
    {
        private static readonly Logger log = LogManager.GetCurrentClassLogger();

        private ProjectInfo curProjectInfo;

        public SanityEngine Engine
        {
            get; init;
        } = new();

        public string contentDirectory
        {
            get;
            private set;
        }

        public void CreateProject(ProjectInfo projectInto)
        {
            Directory.CreateDirectory(projectInto.Directory);

            var projectInfoFileName = string.Format("{0}\\{1}.sanityproject", projectInto.Directory, projectInto.Name);

            var projectInfoJson = JsonSerializer.Serialize(projectInto);
            File.WriteAllText(projectInfoFileName, projectInfoJson);

            var contentDirectory = string.Format("{0}\\{1}", projectInto.Directory, ProjectInfo.ContentDirectory);
            Directory.CreateDirectory(contentDirectory);

            var sourceDirectory = string.Format("{0}\\{1}", projectInto.Directory, ProjectInfo.SourceDirectory);
            Directory.CreateDirectory(sourceDirectory);

            var buildDirectory = string.Format("{0}\\{1}", projectInto.Directory, ProjectInfo.BuildDirectory);
            Directory.CreateDirectory(buildDirectory);

            var cacheDirectory = string.Format("{0}\\{1}", projectInto.Directory, ProjectInfo.CacheDirectory);
            Directory.CreateDirectory(cacheDirectory);

            var userDataDirectory = string.Format("{0}\\{1}", projectInto.Directory, ProjectInfo.UserDataDrectory);
            Directory.CreateDirectory(userDataDirectory);

            log.Info("Created project {0}", projectInto.Name);
        }

        public void OpenProject(ProjectInfo projectInfo)
        {
            curProjectInfo = projectInfo;
            SetProjectDirectory(projectInfo.Directory);

            // TODO: Validate caches, compile shaders, munge textures, etc
        }

        private void SetProjectDirectory(string directory)
        {
            contentDirectory = string.Format("{0}\\{1}", directory, ProjectInfo.ContentDirectory);
            Engine.SetContentDirectory(contentDirectory);

            // TODO: Set other directories as needed
        }
    }
}
