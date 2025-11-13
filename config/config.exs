import Config

# Pythonx configuration for uv-based Python dependency management
# This initializes Pythonx with the pyproject.toml configuration at compile time
# ufbx-python is installed from git repository
config :pythonx, :uv_init,
  pyproject_toml: """
  [project]
  name = "aria-fbx"
  version = "0.1.0"
  requires-python = ">=3.9"
  dependencies = [
      # ufbx-python from git
      "ufbx @ git+https://github.com/bqqbarbhg/ufbx-python.git",
  ]
  """

