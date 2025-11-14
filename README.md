# AriaFbx

FBX file processing library for Elixir using the ufbx-python library.

## Overview

AriaFbx provides FBX file import functionality using the ufbx-python library via pythonx. It converts FBX files into structured Elixir data structures that can be processed uniformly with other 3D formats.

## Features

- **FBX Import**: Load FBX files (binary and ASCII formats)
- **Scene Structure**: Nodes, meshes, materials, textures, and animations
- **USD Integration**: Convert FBX data to USD format using aria_usd
- **Python Integration**: Uses ufbx-python via pythonx

## Installation

Add `aria_fbx` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:aria_fbx, git: "https://github.com/V-Sekai-fire/aria-fbx.git"},
    {:aria_usd, git: "https://github.com/V-Sekai-fire/aria-usd.git"}
  ]
end
```

Then run `mix deps.get` and `mix compile`.

## Usage

### Loading FBX Files

```elixir
# Load an FBX file
{:ok, document} = AriaFbx.Import.from_file("/path/to/model.fbx")

# Skip validation
{:ok, document} = AriaFbx.Import.from_file("/path/to/model.fbx", validate: false)
```

### Converting to USD

```elixir
# Get an FBX mesh
fbx_mesh = document.meshes |> List.first()

# Convert to USD format using aria_usd
# (Example usage - adjust based on aria_usd API)
AriaUsd.create_mesh("output.usd", "/MyMesh")
```

## Core Modules

- `AriaFbx.Import` - FBX file loading
- `AriaFbx.Document` - FBX document structure
- `AriaFbx.Scene` - Scene data types (Node, Mesh, Material, Texture, Animation)
- `AriaFbx.Parser` - Converts ufbx data to Elixir structures
- `AriaFbx.Nif` - Python wrapper for ufbx-python library

## Requirements

- Elixir ~> 1.18
- Python >= 3.9 (for pythonx and ufbx-python)
- `aria_usd` package

## Building

The project uses `pythonx` to manage Python dependencies. Run:

```bash
mix deps.get
mix compile
```

This will install ufbx-python from git via pythonx's uv dependency management.

## License

MIT

