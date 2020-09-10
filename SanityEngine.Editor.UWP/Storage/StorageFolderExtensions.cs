using System;
using System.Threading.Tasks;

using Windows.Storage;

namespace Sanity.Editor.Storage
{
    static class StorageFolderExtensions
    {
        /// <summary>
        /// Creates a folder without throwing any exceptions
        /// </summary>
        /// <param name="self">Folder to create the new folder within</param>
        /// <param name="folderName">Name of the folder to create</param>
        /// <returns>True if the folder was created anew, false if either the fodler couldn't be created or a folder 
        /// with that name already exists</returns>
#nullable enable
        public static async Task<StorageFolder?> CreateFolderSafeAsync(this StorageFolder self, string folderName)
        {
            try
            {
                var folder = await self.CreateFolderAsync(folderName);
                return folder;
            }
            catch
            {
                return null;
            }
        }
#nullable disable
    }
}
