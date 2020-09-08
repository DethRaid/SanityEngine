## SanityEngine

C++ core. Contains rendering, IO, simulation update, physics, and some basic environment simulation

## SanityEngine.CodeGen

Implements the C++ codegen routines

We generate WinRT runtime class definitions for C++ classes with the `[sanity::runtimeclass]` attribute

## SanityEngine.NET

Generates C# language projections for the generated WinRT classes

## SanityEngine.Editor

SanityEngine world editor. Similar in purpose to the Unity or Unreal Engine game editor. One key difference: while 
those tools allow you to build the entire world from the group up, SanityEngine is inherently a procedurally-generated,
simulated world. The SanityEngine editor is aimed at modifying the procedural generation and simulation logic

## SanityEngine.Runtime

Runtime host for games that use SanityEngine
