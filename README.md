# Sanity Engine

A 3D video game engine made to increase my sanity

## Goals

- Fun to work on
- Allow me to explore realtime 3D graphics
- Generate pretty pictures to impress my friends and family

## Build requirements

- Python 3
- Visual Studio 16.8 preview 5
- Windows Kit 10.0.19042.0
- vcpkg with [manifest file support](https://github.com/microsoft/vcpkg/blob/master/docs/specifications/manifests.md)

## Runtime requirements

- Windows 10 2004/20H1 or better
- Graphics driver with support for DirectX 12 Ultimate. At the time of writing, only Nvidia driver 451.48 support DX12U
- Graphics hardware with support for DXR 1.1. At the time of writing, Nvidia's 20XX and 30XX GPUs, along with AMD's 
    60XX GPUs, support DXR 1.1

## Acknowledgement

Sanity Engine uses a lot of third-party code. Most of this is in the form of VCPKG packages - check out `vcpkg.json` 
for details. There's a couple things I've added manually to `SanityEngine/extern`. That's way simpler than dealing 
with CMake packages

Special thanks goes to graphitemaster, the developer of [`rex`](https://github.com/BuckeyeSoftware/rex/). He's been 
incredibly receptive to feature requests and he's put an incredible amount of work into `rex`. Through his incredible 
dedication he's created vocabulary types which are a joy to work with
