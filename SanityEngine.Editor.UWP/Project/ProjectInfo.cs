using System.Text.Json.Serialization;

using Windows.Storage;

namespace Sanity.Editor.Project
{
    public class ProjectInfo
    {
        public static string ContentDirectory
        {
            get => "Content";
        }

        public static string SourceDirectory
        {
            get => "Source";
        }

        public static string BuildDirectory
        {
            get => "Build";
        }

        public static string CacheDirectory
        {
            get => "Cache";
        }

        public static string UserDataDrectory
        {
            get => "User";
        }

        public string Name
        {
            get; private set;
        }

        [JsonIgnore]
        public StorageFolder ProjectParentFolder
        {
            get;
            private set;
        }

        public static ProjectInfo Create(string name, StorageFolder projectParentFolder)
        {
            return new ProjectInfo { Name = name, ProjectParentFolder = projectParentFolder };
        }
    }
}
