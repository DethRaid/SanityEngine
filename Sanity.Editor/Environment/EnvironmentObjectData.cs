using System;
using System.Text.Json.Serialization;

namespace Sanity.Editor.Environment
{
    using EnvironmentObjectDataReference = Guid;
    using EntityReference = Guid;
    using DensityMapPipelineReference = Guid;

    /// <summary>
    /// Editor data for an object that can get spawned in the environment
    /// </summary>
    /// This class gets serialized to disk
    public class EnvironmentObjectData
    {
        /// <summary>
        /// Name of this environment object
        /// </summary>
        string Name
        {
            get; set;
        }

        /// <summary>
        /// Entity to spawn for this environment object
        /// </summary>
        public EntityReference Entity
        {
            get; set;
        }

        /// <summary>
        /// Density map pipeline that controls where this environment object can spawn
        /// </summary>
        public DensityMapPipelineReference DensityMapPipeline
        {
            get; set;
        }
    }
}
