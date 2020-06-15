# Biomes

We'll have a procedural biome system based on climate

Climate will come from oceans, lakes, mountains, and other terrain features that affect overall moisture, wind, soil conditions, etc. That data, along with other data that I find useful, will be passed to a procedural biome generation system

The procedural biome system will be based on [that of Horizon: Zero Dawn](https://www.youtube.com/watch?v=ToCozpl1sYY)

## ProcedurallySpawnableObject

Sanity Engine's biome system is implemented with what I call `ProcedurallySpawnableObject`s. Each `ProceduruallySpawnableObject` has an associated Wren script that has the logic for when to spawn that object. One script might only spawn objects underwater, one script might only spawn objects near a road, etc. Those scripts all output specific parameters that feed into the hardcoded object placement system, which decides exactly where to spawn a `ProcedurallySpawnedObject`
