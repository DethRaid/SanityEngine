using System;

namespace Sanity.Editor.Project
{
    public record ProjectInfo(string Name, string Directory, string Author, DateTime CreationTime)
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
    }
}
