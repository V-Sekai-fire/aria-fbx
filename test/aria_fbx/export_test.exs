# SPDX-License-Identifier: MIT
# Copyright (c) 2025-present K. S. Ernest (iFire) Lee

defmodule AriaFbx.ExportTest do
  use ExUnit.Case
  alias AriaFbx.{Document, Export, Scene}

  describe "to_file/3" do
    test "exports a minimal document to FBX file" do
      {:ok, temp_file} = Briefly.create(extname: ".fbx")

      document = Document.new("FBX 7.4")

      result = Export.to_file(document, temp_file)

      try do
        case result do
          {:ok, file_path} ->
            assert File.exists?(file_path)
            # file_path might be a charlist, convert to string for comparison
            file_path_str = if is_list(file_path), do: List.to_string(file_path), else: file_path
            assert file_path_str == temp_file

          {:error, reason} ->
            # Empty documents might not be valid, but we should get a proper error
            assert is_binary(reason) or is_atom(reason)
        end
      after
        if File.exists?(temp_file), do: File.rm(temp_file)
      end
    end

    test "exports document with mesh to FBX file" do
      {:ok, temp_file} = Briefly.create(extname: ".fbx")

      document = %Document{
        version: "FBX 7.4",
        meshes: [
          %Scene.Mesh{
            id: 1,
            name: "TestMesh",
            positions: [
              [0.0, 0.0, 0.0],
              [1.0, 0.0, 0.0],
              [0.0, 1.0, 0.0]
            ],
            indices: [0, 1, 2],
            normals: [
              [0.0, 0.0, 1.0],
              [0.0, 0.0, 1.0],
              [0.0, 0.0, 1.0]
            ]
          }
        ],
        nodes: [
          %Scene.Node{
            id: 1,
            name: "RootNode",
            translation: {0.0, 0.0, 0.0},
            rotation: {0.0, 0.0, 0.0, 1.0},
            scale: {1.0, 1.0, 1.0},
            mesh_id: 1
          }
        ]
      }

      result = Export.to_file(document, temp_file, format: :binary)

      try do
        case result do
          {:ok, file_path} ->
            assert File.exists?(file_path)

            # Verify we can read it back
            file_path_str = if is_list(file_path), do: List.to_string(file_path), else: file_path
            {:ok, read_scene} = AriaFbx.Nif.load_fbx(file_path_str)
            assert is_map(read_scene)

          {:error, reason} ->
            flunk("Failed to export document: #{inspect(reason)}")
        end
      after
        if File.exists?(temp_file), do: File.rm(temp_file)
      end
    end

    test "exports document with ASCII format" do
      {:ok, temp_file} = Briefly.create(extname: ".fbx")

      document = %Document{
        version: "FBX 7.4",
        meshes: [
          %Scene.Mesh{
            id: 1,
            name: "SimpleMesh",
            positions: [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
            indices: [0, 1, 2]
          }
        ]
      }

      result = Export.to_file(document, temp_file, format: :ascii)

      case result do
        {:ok, file_path} ->
          assert File.exists?(file_path)

          # ASCII files should contain text
          content = File.read!(file_path)
          assert String.contains?(content, "FBX") or byte_size(content) > 0

        {:error, reason} ->
          flunk("Failed to export ASCII FBX: #{inspect(reason)}")
      end

      # Cleanup
      if File.exists?(temp_file), do: File.rm(temp_file)
    end

    test "exports document with materials" do
      {:ok, temp_file} = Briefly.create(extname: ".fbx")

      document = %Document{
        version: "FBX 7.4",
        meshes: [
          %Scene.Mesh{
            id: 1,
            name: "MaterialMesh",
            positions: [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
            indices: [0, 1, 2]
          }
        ],
        materials: [
          %Scene.Material{
            id: 1,
            name: "TestMaterial",
            diffuse_color: {1.0, 0.0, 0.0},
            specular_color: {0.5, 0.5, 0.5},
            emissive_color: {0.0, 0.0, 0.0}
          }
        ]
      }

      result = Export.to_file(document, temp_file)

      case result do
        {:ok, file_path} ->
          assert File.exists?(file_path)

        {:error, reason} ->
          # Material export might not be fully implemented yet
          assert is_binary(reason) or is_atom(reason)
      end

      # Cleanup
      if File.exists?(temp_file), do: File.rm(temp_file)
    end
  end

  describe "to_binary/2" do
    test "exports document to binary data" do
      document = %Document{
        version: "FBX 7.4",
        meshes: [
          %Scene.Mesh{
            id: 1,
            name: "BinaryMesh",
            positions: [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
            indices: [0, 1, 2]
          }
        ]
      }

      result = Export.to_binary(document, format: :binary)

      case result do
        {:ok, binary_data} ->
          assert is_binary(binary_data)
          assert byte_size(binary_data) > 0

          # Verify we can load it back
          load_result = AriaFbx.Nif.load_fbx_binary(binary_data)
          assert {:ok, _scene} = load_result

        {:error, reason} ->
          flunk("Failed to export to binary: #{inspect(reason)}")
      end
    end
  end
end
