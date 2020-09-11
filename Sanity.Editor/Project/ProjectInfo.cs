using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

namespace Sanity.Editor.Project
{
    public class ProjectInfo
    {
        public string Name;

        [JsonIgnore]
        public string Directory;

        public string Author;

        public DateTime CreationTime;
    }
}
