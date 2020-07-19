# Sanity Engine

A 3D video game engine made to increase my sanity

## Goals

- Fun to work on
- Allow me to explore realtime 3D graphics
- Generate pretty pictures to impress my friends and family

## Build requirements

- Python 3
- `dxc` must be in your PATH
- Visual Studio 16.6
- Windows Kit 10.0.19042.0
- vcpkg with [manifest file support](https://github.com/microsoft/vcpkg/blob/master/docs/specifications/manifests.md)

## Runtime requirements

- Windows 10 2004/20H1 or better
- Graphics driver with support for DirectX 12 Ultimate. At the time of writing, only Nvidia driver 451.48 support DX12U
- Graphics hardware with support for DXR 1.1. At the time of writing, only Nvidia's Turing GPUs support DXR 1.1

## Acknowledgement

Sanity Engine uses a lot of third-party code. Most of this is stored in `SanityEngine/extern` and can be seen there - the one exception is the D3D12 helper library, which is stored at `SanityEngine/src/rhi/d3dx12.hpp`

Special thanks goes to graphitemaster, the developer of [`rex`](https://github.com/BuckeyeSoftware/rex/). He's been incredibly receptive to feature requests and he's put an incredible amount of work into `rex`. Through his incredible dedication he's created vocabulary types which are a joy to work with
