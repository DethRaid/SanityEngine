# Horus Scripting System

Sanity Engine includes a scripting system called Horus. It's built on the Wren scripting language, with lots of foreign methods and classes to allow scripts to interact with the engine

Scripts should be small and self-contained. They should only serve to tie together systems built in C++, or to implement interesting game logic. For instance, the rules for how to place objects in a biome might be implemented in scripting, but the actual evaluation of those rules and placement of objects will be C++. Scripts shine when used for high-level or frequently-changed code, while C++ should be preferred for lower-level, performance-critical, or infrequently-changed code


