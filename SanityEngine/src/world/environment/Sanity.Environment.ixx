export module Sanity.Environment;

namespace Sanity::Environment {
    /// <summary>
    /// Builtin environment textures that SanityEngine automatically generates
    /// </summary>
    export enum BuiltinTextureType {
        /// <summary>
        /// Heightmap of the terrain
        /// </summary>
        /// Single-channel floating-point 
        Height,

        /// <summary>
        /// Depth of the surface water at any given location
        /// </summary>
        /// Single-channel floating-point
        /// 
        /// 0 means no water, negative numbers are a bug
        WaterDepth,

        /// <summary>
        /// Average wind vector within ten meters of the surface
        /// </summary>
        /// Three-channel floating-point
        /// 
        /// The magnitude of the vector stored at each pixel of the texture is the wind speed in km/h, you can 
        /// normalize the vector to get the wind's direction
        Wind,

        /// <summary>
        /// Average rainfall, in centimeters per year
        /// </summary>
        /// Single-channel floating point
        Rainfall,
    };
}