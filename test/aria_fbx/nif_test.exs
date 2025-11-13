# SPDX-License-Identifier: MIT
# Copyright (c) 2025-present K. S. Ernest (iFire) Lee

defmodule AriaFbx.NifTest do
  use ExUnit.Case
  alias AriaFbx.Nif

  describe "load_fbx/1" do
    test "returns error for non-existent file" do
      result = Nif.load_fbx("/nonexistent/file.fbx")
      assert {:error, _reason} = result
    end
  end

  describe "load_fbx_binary/1" do
    test "returns error for invalid binary data" do
      invalid_data = <<0, 1, 2, 3>>
      result = Nif.load_fbx_binary(invalid_data)
      assert {:error, _reason} = result
    end
  end

  describe "write_fbx/3" do
    test "writes a minimal FBX file with a single mesh" do
      {:ok, temp_file} = Briefly.create(extname: ".fbx")

      scene_data = %{
        "version" => "FBX 7.4",
        "nodes" => [
          %{
            "id" => 1,
            "name" => "RootNode",
            "translation" => [0.0, 0.0, 0.0],
            "rotation" => [0.0, 0.0, 0.0, 1.0],
            "scale" => [1.0, 1.0, 1.0],
            "mesh_id" => 1
          }
        ],
        "meshes" => [
          %{
            "id" => 1,
            "name" => "TestMesh",
            "positions" => [
              0.0,
              0.0,
              0.0,
              1.0,
              0.0,
              0.0,
              0.0,
              1.0,
              0.0
            ],
            "indices" => [0, 1, 2],
            "normals" => [
              0.0,
              0.0,
              1.0,
              0.0,
              0.0,
              1.0,
              0.0,
              0.0,
              1.0
            ]
          }
        ],
        "materials" => [],
        "textures" => [],
        "animations" => []
      }

      result = Nif.write_fbx(temp_file, scene_data, :binary)

      try do
        case result do
          {:ok, file_path} ->
            assert File.exists?(file_path)
            # file_path might be a charlist, convert to string for comparison
            file_path_str = if is_list(file_path), do: List.to_string(file_path), else: file_path
            assert file_path_str == temp_file

            # Verify we can read it back
            read_result = Nif.load_fbx(file_path_str)
            assert {:ok, _read_scene} = read_result

          {:error, reason} ->
            flunk("Failed to write FBX: #{inspect(reason)}")
        end
      after
        # Cleanup
        if File.exists?(temp_file), do: File.rm(temp_file)
      end
    end

    test "writes FBX file in ASCII format" do
      {:ok, temp_file} = Briefly.create(extname: ".fbx")

      scene_data = %{
        "version" => "FBX 7.4",
        "nodes" => [],
        "meshes" => [
          %{
            "id" => 1,
            "name" => "SimpleMesh",
            "positions" => [0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0],
            "indices" => [0, 1, 2]
          }
        ],
        "materials" => [],
        "textures" => [],
        "animations" => []
      }

      result = Nif.write_fbx(temp_file, scene_data, :ascii)

      try do
        case result do
          {:ok, file_path} ->
            assert File.exists?(file_path)

            # ASCII files should be readable as text
            content = File.read!(file_path)
            assert String.contains?(content, "FBX")

          {:error, reason} ->
            flunk("Failed to write ASCII FBX: #{inspect(reason)}")
        end
      after
        if File.exists?(temp_file), do: File.rm(temp_file)
      end
    end

    test "handles empty scene data" do
      {:ok, temp_file} = Briefly.create(extname: ".fbx")

      scene_data = %{
        "version" => "FBX 7.4",
        "nodes" => [],
        "meshes" => [],
        "materials" => [],
        "textures" => [],
        "animations" => []
      }

      result = Nif.write_fbx(temp_file, scene_data, :binary)

      try do
        case result do
          {:ok, file_path} ->
            assert File.exists?(file_path)

          {:error, reason} ->
            # Empty scenes might be valid or invalid depending on ufbx_write
            # Just verify we get a proper error message
            assert is_binary(reason)
        end
      after
        if File.exists?(temp_file), do: File.rm(temp_file)
      end
    end

    test "handles mesh with UV coordinates" do
      {:ok, temp_file} = Briefly.create(extname: ".fbx")

      scene_data = %{
        "version" => "FBX 7.4",
        "nodes" => [
          %{
            "id" => 1,
            "name" => "MeshNode",
            "translation" => [0.0, 0.0, 0.0],
            "rotation" => [0.0, 0.0, 0.0, 1.0],
            "scale" => [1.0, 1.0, 1.0],
            "mesh_id" => 1
          }
        ],
        "meshes" => [
          %{
            "id" => 1,
            "name" => "TexturedMesh",
            "positions" => [
              0.0,
              0.0,
              0.0,
              1.0,
              0.0,
              0.0,
              0.0,
              1.0,
              0.0
            ],
            "indices" => [0, 1, 2],
            "normals" => [
              0.0,
              0.0,
              1.0,
              0.0,
              0.0,
              1.0,
              0.0,
              0.0,
              1.0
            ],
            "texcoords" => [
              0.0,
              0.0,
              1.0,
              0.0,
              0.0,
              1.0
            ]
          }
        ],
        "materials" => [],
        "textures" => [],
        "animations" => []
      }

      result = Nif.write_fbx(temp_file, scene_data, :binary)

      try do
        case result do
          {:ok, file_path} ->
            assert File.exists?(file_path)

            # Verify we can read it back
            file_path_str = if is_list(file_path), do: List.to_string(file_path), else: file_path
            read_result = Nif.load_fbx(file_path_str)
            assert {:ok, _read_scene} = read_result

          {:error, reason} ->
            flunk("Failed to write FBX with UVs: #{inspect(reason)}")
        end
      after
        if File.exists?(temp_file), do: File.rm(temp_file)
      end
    end
  end
end
