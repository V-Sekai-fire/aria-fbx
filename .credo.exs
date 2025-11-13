# SPDX-License-Identifier: MIT
# Copyright (c) 2025-present K. S. Ernest (iFire) Lee

%{
  configs: [
    %{
      name: "default",
      files: %{
        included: ["lib/", "test/", "config/"],
        excluded: [
          "thirdparty/",
          "deps/",
          "_build/",
          "priv/"
        ]
      },
      checks: [
        {Credo.Check.Readability.ModuleDoc, false}
      ]
    }
  ]
}

