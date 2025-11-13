# AriaFbx

FBX file processing library for Elixir using the ufbx C library.

## Overview

AriaFbx provides FBX file import functionality using the ufbx library via NIFs (Native Implemented Functions). It converts FBX files into structured Elixir data structures that can be processed uniformly with other 3D formats.

## Features

- **FBX Import**: Load FBX files (binary and ASCII formats)
- **Scene Structure**: Nodes, meshes, materials, textures, and animations
- **BMesh Conversion**: Convert FBX meshes to BMesh format for n-gon geometry
- **NIF Integration**: Native performance via ufbx C library

## Installation

Add `aria_fbx` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:aria_fbx, git: "https://github.com/V-Sekai-fire/aria-fbx.git"},
    {:aria_bmesh, git: "https://github.com/V-Sekai-fire/aria-bmesh.git"}
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

### Converting to BMesh

```elixir
# Get an FBX mesh
fbx_mesh = document.meshes |> List.first()

# Convert to BMesh format
{:ok, bmesh} = AriaFbx.BmeshConverter.from_fbx_mesh(fbx_mesh)
```

## Core Modules

- `AriaFbx.Import` - FBX file loading
- `AriaFbx.Document` - FBX document structure
- `AriaFbx.Scene` - Scene data types (Node, Mesh, Material, Texture, Animation)
- `AriaFbx.Parser` - Converts ufbx data to Elixir structures
- `AriaFbx.Nif` - NIF wrapper for ufbx library
- `AriaFbx.BmeshConverter` - Converts FBX meshes to BMesh format

## Requirements

- Elixir ~> 1.18
- C compiler (for building NIFs)
- `aria_bmesh` package

## Building

The project uses `elixir_make` to compile the ufbx NIF. Run:

```bash
mix compile
```

This will build the NIF shared library in `priv/ufbx_nif.so` (or `.dylib` on macOS).

## License

MIT

