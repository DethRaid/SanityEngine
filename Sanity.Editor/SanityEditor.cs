using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

using Sanity.Editor.Project;

namespace Sanity.Editor
{
    public class SanityEditor
    {
        public void CreateProject(ProjectInfo info)
        {
            Directory.CreateDirectory(info.Directory);

            var projectInfoFileName = string.Format("{0}\\{1}.sanityproject", info.Directory, info.Name);

            var projectInfoJson = JsonSerializer.Serialize(info);
            File.WriteAllText(projectInfoFileName, projectInfoJson);
        }

        public void OpenProject(ProjectInfo info) => throw new NotImplementedException();
    }
}
