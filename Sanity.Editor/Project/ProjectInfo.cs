using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

namespace Sanity.Editor.Project
{
    public struct ProjectInfo
    {
        public string Name
        {
            get; set;
        }

        [JsonIgnore]
        public string Directory
        {
            get; set;
        }

        public string Author
        {
            get; set;
        }

        public DateTime CreationTime
        {
            get; set;
        }
    }
}
