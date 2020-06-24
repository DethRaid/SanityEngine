# Biomes

We'll have a procedural biome system based on climate

Climate will come from oceans, lakes, mountains, and other terrain features that affect overall moisture, wind, soil conditions, etc. That data, along with other data that I find useful, will be passed to a procedural biome generation system

The procedural biome system will be based on [that of Horizon: Zero Dawn](https://www.youtube.com/watch?v=ToCozpl1sYY)

## Informational textures

The engine itself generates a few textures - terrain height, water depth, wind direction, humidity, average temperature, etc. You can create `ProceduralTexture`s that are the result of some computations - maybe you use water depth to generate a distance from water, then you use humidity and distance from water to calculate groundwater

## Ecotypes
I may or may not have some sort of ecotype. Will probably be a good optimization - An ecotype would only have to evaluate the spawning logic for a limited number of assets, instead of evaluating every asset at the same time

## ProcedurallySpawnableObject

Sanity Engine's biome system is implemented with what I call `ProcedurallySpawnableObject`s. Each `ProcedurallySpawnableObject` has an associated Wren script that has the logic for when to spawn that object. One script might only spawn objects underwater, one script might only spawn objects near a road, etc. Those scripts all output specific parameters that feed into the hardcoded object placement system, which decides exactly where to spawn a `ProcedurallySpawnedObject`

Each object's script can read from any of the informational textures. It runs some computation and generates a density map for that object. This density map, combined with the object's footprint size, feeds into the object spawning system

Any kind of object may be a `ProcedurallySpawnableObject`. Trees, bushes, villages, rocks scattered on a hillside...

## GPU
This system will likely have to eventually run on the GPU for performance reasons. I'll likely have a Wren -> DXIL compilation pipeline, maybe with a HLSL intermediate step or something

# List of biomes

Here's all the biomes that we can generate:

* Ocean
* Tidal zone
* River
* Lake
* Forest
* Lush grasslands
* Arid grasslands
* Rocky desert
* Sandy desert
