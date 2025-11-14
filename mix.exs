# SPDX-License-Identifier: MIT
# Copyright (c) 2025-present K. S. Ernest (iFire) Lee

defmodule AriaFbx.MixProject do
  use Mix.Project

  def project do
    [
      app: :aria_fbx,
      version: "0.1.0",
      elixir: "~> 1.18",
      start_permanent: Mix.env() == :prod,
      deps: deps(),
      compilers: [:elixir_make | Mix.compilers()],
      make_clean: ["clean"],
      description: "ARIA FBX - FBX file processing library using ufbx C library via NIFs",
      package: package()
    ]
  end

  def application do
    [
      extra_applications: [:logger]
    ]
  end

  defp deps do
    [
      {:jason, "~> 1.4"},
      {:elixir_make, "~> 0.7", runtime: false},
      {:briefly, git: "https://github.com/CargoSense/briefly.git"},
      {:aria_usd, git: "https://github.com/V-Sekai-fire/aria-usd.git"},
      {:credo, "~> 1.7", only: [:dev, :test], runtime: false}
    ]
  end

  defp package do
    [
      maintainers: ["K. S. Ernest (iFire) Lee"],
      licenses: ["MIT"],
      links: %{"GitHub" => "https://github.com/V-Sekai-fire/aria-fbx"}
    ]
  end
end
