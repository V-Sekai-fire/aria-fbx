# SPDX-License-Identifier: MIT
# Copyright (c) 2025-present K. S. Ernest (iFire) Lee

defmodule AriaFbx.Export do
  @moduledoc """
  FBX file export functionality.

  Provides functions to write FBX files using ufbx_write via C NIFs.
  Converts FBXDocument structures to FBX files, aligned with the
  AriaGltf.Export API.
  """

  require Logger
  alias AriaFbx.Document
  alias AriaFbx.Nif

  @doc """
  Exports an FBXDocument to an FBX file.

  ## Options

  - `:format` - FBX format: `:binary` (default) or `:ascii`
  - `:version` - FBX version (default: "FBX 7.4")

  ## Examples

      {:ok, _} = AriaFbx.Export.to_file(document, "/path/to/output.fbx")
      {:ok, _} = AriaFbx.Export.to_file(document, "/path/to/output.fbx", format: :ascii)
  """
  @spec to_file(Document.t(), String.t(), keyword()) :: {:ok, String.t()} | {:error, term()}
  def to_file(%Document{} = document, file_path, opts \\ []) when is_binary(file_path) do
    format = Keyword.get(opts, :format, :binary)
    _version = Keyword.get(opts, :version, document.version)

    # Build scene data from document for ufbx_write
    scene_data = build_scene_data_for_write(document)

    # Call the NIF to write the FBX file
    Nif.write_fbx(file_path, scene_data, format)
  end

  @doc """
  Exports an FBXDocument to binary data.

  ## Options

  - `:format` - FBX format: `:binary` (default) or `:ascii`
  - `:version` - FBX version (default: "FBX 7.4")

  ## Examples

      {:ok, binary_data} = AriaFbx.Export.to_binary(document)
  """
  @spec to_binary(Document.t(), keyword()) :: {:ok, binary()} | {:error, term()}
  def to_binary(%Document{} = document, opts \\ []) do
    format = Keyword.get(opts, :format, :binary)
    version = Keyword.get(opts, :version, document.version)

    # For binary export, we'll write to a temporary file and read it back
    {:ok, temp_file} = Briefly.create(extname: ".fbx")

    try do
      case to_file(document, temp_file, format: format, version: version) do
        {:ok, _} ->
          case File.read(temp_file) do
            {:ok, binary_data} -> {:ok, binary_data}
            {:error, reason} -> {:error, "Failed to read exported FBX: #{inspect(reason)}"}
          end

        error ->
          error
      end
    after
      File.rm(temp_file)
    end
  end

  # Build scene data structure for ufbx_write from FBXDocument
  defp build_scene_data_for_write(%Document{} = document) do
    %{
      "version" => document.version,
      "nodes" => encode_nodes_for_write(document.nodes),
      "meshes" => encode_meshes_for_write(document.meshes),
      "materials" => encode_materials_for_write(document.materials),
      "textures" => encode_textures_for_write(document.textures),
      "animations" => encode_animations_for_write(document.animations)
    }
  end

  defp encode_nodes_for_write(nil), do: []

  defp encode_nodes_for_write(nodes) when is_list(nodes) do
    Enum.map(nodes, fn node ->
      %{
        "id" => node.id,
        "name" => node.name || "",
        "parent_id" => node.parent_id,
        "children" => node.children || [],
        "translation" => encode_vec3(node.translation),
        "rotation" => encode_vec4(node.rotation),
        "scale" => encode_vec3(node.scale),
        "mesh_id" => node.mesh_id
      }
    end)
  end

  defp encode_meshes_for_write(nil), do: []

  defp encode_meshes_for_write(meshes) when is_list(meshes) do
    Enum.map(meshes, fn mesh ->
      %{
        "id" => mesh.id,
        "name" => mesh.name,
        "positions" => flatten_positions(mesh.positions),
        "normals" => flatten_normals(mesh.normals),
        "texcoords" => flatten_texcoords(mesh.texcoords),
        "indices" => mesh.indices || [],
        "material_ids" => mesh.material_ids || []
      }
    end)
  end

  defp encode_materials_for_write(nil), do: []

  defp encode_materials_for_write(materials) when is_list(materials) do
    Enum.map(materials, fn material ->
      %{
        "id" => material.id,
        "name" => material.name,
        "diffuse_color" => encode_vec3(material.diffuse_color),
        "specular_color" => encode_vec3(material.specular_color),
        "emissive_color" => encode_vec3(material.emissive_color)
      }
    end)
  end

  defp encode_textures_for_write(nil), do: []

  defp encode_textures_for_write(textures) when is_list(textures) do
    Enum.map(textures, fn texture ->
      %{
        "id" => texture.id,
        "name" => texture.name,
        "file_path" => texture.file_path
      }
    end)
  end

  defp encode_animations_for_write(nil), do: []

  defp encode_animations_for_write(animations) when is_list(animations) do
    Enum.map(animations, fn animation ->
      %{
        "id" => animation.id,
        "name" => animation.name,
        "node_id" => animation.node_id,
        "keyframes" => encode_keyframes(animation.keyframes)
      }
    end)
  end

  defp encode_keyframes(nil), do: []

  defp encode_keyframes(keyframes) when is_list(keyframes) do
    Enum.map(keyframes, fn keyframe ->
      %{
        "time" => keyframe.time,
        "translation" => encode_vec3(keyframe.translation),
        "rotation" => encode_vec4(keyframe.rotation),
        "scale" => encode_vec3(keyframe.scale)
      }
    end)
  end

  defp encode_vec3(nil), do: [0.0, 0.0, 0.0]
  defp encode_vec3({x, y, z}) when is_float(x) and is_float(y) and is_float(z), do: [x, y, z]
  defp encode_vec3([x, y, z]) when is_number(x) and is_number(y) and is_number(z), do: [x, y, z]
  defp encode_vec3(_), do: [0.0, 0.0, 0.0]

  defp encode_vec4(nil), do: [0.0, 0.0, 0.0, 1.0]

  defp encode_vec4({x, y, z, w}) when is_float(x) and is_float(y) and is_float(z) and is_float(w),
    do: [x, y, z, w]

  defp encode_vec4([x, y, z, w])
       when is_number(x) and is_number(y) and is_number(z) and is_number(w),
       do: [x, y, z, w]

  defp encode_vec4(_), do: [0.0, 0.0, 0.0, 1.0]

  defp flatten_positions(nil), do: []

  defp flatten_positions(positions) when is_list(positions) do
    Enum.flat_map(positions, fn
      [x, y, z] when is_number(x) and is_number(y) and is_number(z) -> [x, y, z]
      {x, y, z} when is_number(x) and is_number(y) and is_number(z) -> [x, y, z]
      _ -> []
    end)
  end

  defp flatten_normals(nil), do: []

  defp flatten_normals(normals) when is_list(normals) do
    Enum.flat_map(normals, fn
      [x, y, z] when is_number(x) and is_number(y) and is_number(z) -> [x, y, z]
      {x, y, z} when is_number(x) and is_number(y) and is_number(z) -> [x, y, z]
      _ -> []
    end)
  end

  defp flatten_texcoords(nil), do: []

  defp flatten_texcoords(texcoords) when is_list(texcoords) do
    Enum.flat_map(texcoords, fn
      [u, v] when is_number(u) and is_number(v) -> [u, v]
      {u, v} when is_number(u) and is_number(v) -> [u, v]
      _ -> []
    end)
  end
end
