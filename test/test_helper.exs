# SPDX-License-Identifier: MIT
# Copyright (c) 2025-present K. S. Ernest (iFire) Lee

ExUnit.start(exclude: [:skip_if_no_usd])

# Ensure briefly application is started for ETS table initialization
# This is required when using --no-start in CI
Application.ensure_all_started(:briefly)
