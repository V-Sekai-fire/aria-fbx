# SPDX-License-Identifier: MIT
# Copyright (c) 2025-present K. S. Ernest (iFire) Lee

defmodule AriaFbx.Nif do
  @moduledoc """
  C NIF bindings for ufbx FBX loading functionality.

  This module provides Elixir bindings to the ufbx C library via NIFs.
  """

  @on_load :load_nifs

  def load_nifs do
    nif_path = :filename.join(:code.priv_dir(:aria_fbx), "ufbx_nif")
    :erlang.load_nif(nif_path, 0)
  end

  @doc """
  Loads an FBX file using the ufbx C library.

  ## Parameters

  - `file_path`: Path to the FBX file

  ## Returns

  - `{:ok, scene_data}` - On successful load
  - `{:error, reason}` - On failure

  ## Examples

      {:ok, scene} = AriaFbx.Nif.load_fbx("/path/to/model.fbx")
  """
  @spec load_fbx(String.t()) :: {:ok, map()} | {:error, String.t()}
  def load_fbx(_file_path) do
    :erlang.nif_error(:nif_not_loaded)
  end

  @doc """
  Loads an FBX file from binary data using the ufbx C library.

  ## Parameters

  - `binary_data`: Binary data containing the FBX file content

  ## Returns

  - `{:ok, scene_data}` - On successful load
  - `{:error, reason}` - On failure

  ## Examples

      {:ok, scene} = AriaFbx.Nif.load_fbx_binary(binary_data)
  """
  @spec load_fbx_binary(binary()) :: {:ok, map()} | {:error, String.t()}
  def load_fbx_binary(_binary_data) do
    :erlang.nif_error(:nif_not_loaded)
  end

  @doc """
  Writes an FBX file using the ufbx_write C library.

  ## Parameters

  - `file_path`: Path where the FBX file should be written
  - `scene_data`: Map containing scene data (nodes, meshes, materials, etc.)
  - `format`: Format atom - `:binary` (default) or `:ascii`

  ## Returns

  - `{:ok, file_path}` - On successful write
  - `{:error, reason}` - On failure

  ## Examples

      scene_data = %{
        "nodes" => [...],
        "meshes" => [...],
        "materials" => [...]
      }
      {:ok, path} = AriaFbx.Nif.write_fbx("/path/to/output.fbx", scene_data, :binary)
  """
  @spec write_fbx(String.t(), map(), atom()) :: {:ok, String.t()} | {:error, String.t()}
  def write_fbx(_file_path, _scene_data, _format) do
    :erlang.nif_error(:nif_not_loaded)
  end
end
