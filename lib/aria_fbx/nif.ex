# SPDX-License-Identifier: MIT
# Copyright (c) 2025-present K. S. Ernest (iFire) Lee

defmodule AriaFbx.Nif do
  @moduledoc """
  Python wrapper for ufbx FBX loading functionality using ufbx-python.

  This module provides Elixir bindings to the ufbx-python library via pythonx.
  """

  require Logger
  alias Pythonx

  @doc """
  Loads an FBX file using the ufbx-python library.

  ## Parameters

  - `file_path`: Path to the FBX file

  ## Returns

  - `{:ok, scene_data}` - On successful load
  - `{:error, reason}` - On failure

  ## Examples

      {:ok, scene} = AriaFbx.Nif.load_fbx("/path/to/model.fbx")
  """
  @spec load_fbx(String.t()) :: {:ok, map()} | {:error, String.t()}
  def load_fbx(file_path) when is_binary(file_path) do
    case ensure_pythonx() do
      :ok -> do_load_fbx(file_path)
      :not_available -> {:error, "Pythonx not available"}
    end
  end

  defp do_load_fbx(file_path) do
    code = """
    import json
    import sys

    try:
        import ufbx
        
        # Load FBX file
        scene = ufbx.load('#{file_path}')
        
        # Extract scene data
        scene_data = {
            'version': f"FBX {scene.version.major}.{scene.version.minor}",
            'nodes': [],
            'meshes': [],
            'materials': [],
            'textures': [],
            'animations': []
        }
        
        # Extract nodes
        for i, node in enumerate(scene.nodes):
            node_data = {
                'id': i,
                'name': node.name if hasattr(node, 'name') else f'Node_{i}',
                'parent_id': None,
                'children': [],
                'translation': [node.local_transform.translation.x, node.local_transform.translation.y, node.local_transform.translation.z] if hasattr(node, 'local_transform') else [0.0, 0.0, 0.0],
                'rotation': [node.local_transform.rotation.x, node.local_transform.rotation.y, node.local_transform.rotation.z, node.local_transform.rotation.w] if hasattr(node, 'local_transform') and hasattr(node.local_transform, 'rotation') else [0.0, 0.0, 0.0, 1.0],
                'scale': [node.local_transform.scale.x, node.local_transform.scale.y, node.local_transform.scale.z] if hasattr(node, 'local_transform') else [1.0, 1.0, 1.0],
                'mesh_id': None,
                'extensions': {},
                'extras': {}
            }
            
            # Set parent_id if node has parent
            if hasattr(node, 'parent') and node.parent:
                parent_idx = scene.nodes.index(node.parent) if node.parent in scene.nodes else None
                node_data['parent_id'] = parent_idx
            
            # Set children
            if hasattr(node, 'children'):
                node_data['children'] = [scene.nodes.index(child) for child in node.children if child in scene.nodes]
            
            # Set mesh_id if node has mesh
            if hasattr(node, 'mesh') and node.mesh:
                mesh_idx = scene.meshes.index(node.mesh) if node.mesh in scene.meshes else None
                node_data['mesh_id'] = mesh_idx
            
            scene_data['nodes'].append(node_data)
        
        # Extract meshes
        for i, mesh in enumerate(scene.meshes):
            mesh_data = {
                'id': i,
                'name': mesh.name if hasattr(mesh, 'name') else f'Mesh_{i}',
                'positions': [],
                'normals': [],
                'texcoords': [],
                'indices': [],
                'material_ids': [],
                'extensions': {},
                'extras': {}
            }
            
            # Extract vertex positions
            if hasattr(mesh, 'vertex_positions'):
                mesh_data['positions'] = [[v.x, v.y, v.z] for v in mesh.vertex_positions]
            
            # Extract vertex normals
            if hasattr(mesh, 'vertex_normals'):
                mesh_data['normals'] = [[v.x, v.y, v.z] for v in mesh.vertex_normals]
            
            # Extract vertex UVs
            if hasattr(mesh, 'vertex_uv') and len(mesh.vertex_uv) > 0:
                mesh_data['texcoords'] = [[uv.x, uv.y] for uv in mesh.vertex_uv[0]]
            
            # Extract indices
            if hasattr(mesh, 'indices'):
                mesh_data['indices'] = [int(idx) for idx in mesh.indices]
            
            # Extract material IDs
            if hasattr(mesh, 'materials'):
                mesh_data['material_ids'] = [scene.materials.index(mat) for mat in mesh.materials if mat in scene.materials]
            
            scene_data['meshes'].append(mesh_data)
        
        # Extract materials
        for i, material in enumerate(scene.materials):
            material_data = {
                'id': i,
                'name': material.name if hasattr(material, 'name') else f'Material_{i}',
                'diffuse_color': [1.0, 1.0, 1.0],
                'specular_color': [0.0, 0.0, 0.0],
                'emissive_color': [0.0, 0.0, 0.0],
                'extensions': {},
                'extras': {}
            }
            
            # Extract PBR material properties if available
            if hasattr(material, 'pbr'):
                pbr = material.pbr
                if hasattr(pbr, 'base_color'):
                    material_data['diffuse_color'] = [pbr.base_color.x, pbr.base_color.y, pbr.base_color.z]
                if hasattr(pbr, 'emission_color'):
                    material_data['emissive_color'] = [pbr.emission_color.x, pbr.emission_color.y, pbr.emission_color.z]
            
            scene_data['materials'].append(material_data)
        
        # Extract textures
        for i, texture in enumerate(scene.textures):
            texture_data = {
                'id': i,
                'name': texture.name if hasattr(texture, 'name') else f'Texture_{i}',
                'file_path': texture.filename if hasattr(texture, 'filename') else None,
                'extensions': {},
                'extras': {}
            }
            scene_data['textures'].append(texture_data)
        
        # Extract animations (basic support)
        for i, anim in enumerate(scene.animations):
            anim_data = {
                'id': i,
                'name': anim.name if hasattr(anim, 'name') else f'Animation_{i}',
                'channels': [],
                'extensions': {},
                'extras': {}
            }
            scene_data['animations'].append(anim_data)
        
        json.dumps(scene_data)
    except ImportError as e:
        json.dumps({'status': 'error', 'message': f'Failed to import ufbx: {str(e)}'})
    except Exception as e:
        json.dumps({'status': 'error', 'message': f'Failed to load FBX: {str(e)}'})
    """

    case Pythonx.eval(code, %{}) do
      {result, _globals} ->
        case Pythonx.decode(result) do
          json_str when is_binary(json_str) ->
            case Jason.decode(json_str) do
              {:ok, %{"status" => "error", "message" => msg}} ->
                {:error, msg}

              {:ok, scene_data} when is_map(scene_data) ->
                {:ok, scene_data}

              _ ->
                {:error, "Unexpected response from ufbx-python"}
            end

          _ ->
            {:error, "Failed to decode FBX load result"}
        end

      error ->
        {:error, "Failed to load FBX: #{inspect(error)}"}
    end
  rescue
    e -> {:error, Exception.message(e)}
  end

  @doc """
  Loads an FBX file from binary data using the ufbx-python library.

  ## Parameters

  - `binary_data`: Binary data containing the FBX file content

  ## Returns

  - `{:ok, scene_data}` - On successful load
  - `{:error, reason}` - On failure

  ## Examples

      {:ok, scene} = AriaFbx.Nif.load_fbx_binary(binary_data)
  """
  @spec load_fbx_binary(binary()) :: {:ok, map()} | {:error, String.t()}
  def load_fbx_binary(binary_data) when is_binary(binary_data) do
    case ensure_pythonx() do
      :ok -> do_load_fbx_binary(binary_data)
      :not_available -> {:error, "Pythonx not available"}
    end
  end

  defp do_load_fbx_binary(binary_data) do
    # Use pythonx to load from binary data directly
    base64_data = Base.encode64(binary_data)

    code = """
    import json
    import base64
    import io

    try:
        import ufbx
        
        # Decode base64 binary data
        fbx_data = base64.b64decode('#{base64_data}')
        
        # Load FBX from binary data
        scene = ufbx.load(io.BytesIO(fbx_data))
        
        # Extract scene data (same as load_fbx)
        scene_data = {
            'version': f"FBX {scene.version.major}.{scene.version.minor}",
            'nodes': [],
            'meshes': [],
            'materials': [],
            'textures': [],
            'animations': []
        }
        
        # Extract nodes
        for i, node in enumerate(scene.nodes):
            node_data = {
                'id': i,
                'name': node.name if hasattr(node, 'name') else f'Node_{i}',
                'parent_id': None,
                'children': [],
                'translation': [node.local_transform.translation.x, node.local_transform.translation.y, node.local_transform.translation.z] if hasattr(node, 'local_transform') else [0.0, 0.0, 0.0],
                'rotation': [node.local_transform.rotation.x, node.local_transform.rotation.y, node.local_transform.rotation.z, node.local_transform.rotation.w] if hasattr(node, 'local_transform') and hasattr(node.local_transform, 'rotation') else [0.0, 0.0, 0.0, 1.0],
                'scale': [node.local_transform.scale.x, node.local_transform.scale.y, node.local_transform.scale.z] if hasattr(node, 'local_transform') else [1.0, 1.0, 1.0],
                'mesh_id': None,
                'extensions': {},
                'extras': {}
            }
            
            # Set parent_id if node has parent
            if hasattr(node, 'parent') and node.parent:
                parent_idx = scene.nodes.index(node.parent) if node.parent in scene.nodes else None
                node_data['parent_id'] = parent_idx
            
            # Set children
            if hasattr(node, 'children'):
                node_data['children'] = [scene.nodes.index(child) for child in node.children if child in scene.nodes]
            
            # Set mesh_id if node has mesh
            if hasattr(node, 'mesh') and node.mesh:
                mesh_idx = scene.meshes.index(node.mesh) if node.mesh in scene.meshes else None
                node_data['mesh_id'] = mesh_idx
            
            scene_data['nodes'].append(node_data)
        
        # Extract meshes
        for i, mesh in enumerate(scene.meshes):
            mesh_data = {
                'id': i,
                'name': mesh.name if hasattr(mesh, 'name') else f'Mesh_{i}',
                'positions': [],
                'normals': [],
                'texcoords': [],
                'indices': [],
                'material_ids': [],
                'extensions': {},
                'extras': {}
            }
            
            # Extract vertex positions
            if hasattr(mesh, 'vertex_positions'):
                mesh_data['positions'] = [[v.x, v.y, v.z] for v in mesh.vertex_positions]
            
            # Extract vertex normals
            if hasattr(mesh, 'vertex_normals'):
                mesh_data['normals'] = [[v.x, v.y, v.z] for v in mesh.vertex_normals]
            
            # Extract vertex UVs
            if hasattr(mesh, 'vertex_uv') and len(mesh.vertex_uv) > 0:
                mesh_data['texcoords'] = [[uv.x, uv.y] for uv in mesh.vertex_uv[0]]
            
            # Extract indices
            if hasattr(mesh, 'indices'):
                mesh_data['indices'] = [int(idx) for idx in mesh.indices]
            
            # Extract material IDs
            if hasattr(mesh, 'materials'):
                mesh_data['material_ids'] = [scene.materials.index(mat) for mat in mesh.materials if mat in scene.materials]
            
            scene_data['meshes'].append(mesh_data)
        
        # Extract materials
        for i, material in enumerate(scene.materials):
            material_data = {
                'id': i,
                'name': material.name if hasattr(material, 'name') else f'Material_{i}',
                'diffuse_color': [1.0, 1.0, 1.0],
                'specular_color': [0.0, 0.0, 0.0],
                'emissive_color': [0.0, 0.0, 0.0],
                'extensions': {},
                'extras': {}
            }
            
            # Extract PBR material properties if available
            if hasattr(material, 'pbr'):
                pbr = material.pbr
                if hasattr(pbr, 'base_color'):
                    material_data['diffuse_color'] = [pbr.base_color.x, pbr.base_color.y, pbr.base_color.z]
                if hasattr(pbr, 'emission_color'):
                    material_data['emissive_color'] = [pbr.emission_color.x, pbr.emission_color.y, pbr.emission_color.z]
            
            scene_data['materials'].append(material_data)
        
        # Extract textures
        for i, texture in enumerate(scene.textures):
            texture_data = {
                'id': i,
                'name': texture.name if hasattr(texture, 'name') else f'Texture_{i}',
                'file_path': texture.filename if hasattr(texture, 'filename') else None,
                'extensions': {},
                'extras': {}
            }
            scene_data['textures'].append(texture_data)
        
        # Extract animations (basic support)
        for i, anim in enumerate(scene.animations):
            anim_data = {
                'id': i,
                'name': anim.name if hasattr(anim, 'name') else f'Animation_{i}',
                'channels': [],
                'extensions': {},
                'extras': {}
            }
            scene_data['animations'].append(anim_data)
        
        json.dumps(scene_data)
    except ImportError as e:
        json.dumps({'status': 'error', 'message': f'Failed to import ufbx: {str(e)}'})
    except Exception as e:
        json.dumps({'status': 'error', 'message': f'Failed to load FBX from binary: {str(e)}'})
    """

    case Pythonx.eval(code, %{}) do
      {result, _globals} ->
        case Pythonx.decode(result) do
          json_str when is_binary(json_str) ->
            case Jason.decode(json_str) do
              {:ok, %{"status" => "error", "message" => msg}} ->
                {:error, msg}

              {:ok, scene_data} when is_map(scene_data) ->
                {:ok, scene_data}

              _ ->
                {:error, "Unexpected response from ufbx-python"}
            end

          _ ->
            {:error, "Failed to decode FBX load result"}
        end

      error ->
        {:error, "Failed to load FBX from binary: #{inspect(error)}"}
    end
  rescue
    e -> {:error, Exception.message(e)}
  end

  defp ensure_pythonx do
    if Code.ensure_loaded?(Pythonx) do
      :ok
    else
      Logger.warning("Pythonx not available")
      :not_available
    end
  end
end
