# EVEngine Intel macOS rendering reproduction

Small diagnostic for EVEngine/EVEngine PR #701. Uses `macos-15-intel`, records the actual GPU, and runs an existing SDK for at most 30 seconds. No engine build, timeout increases, or test skips.

Pinned input: upstream run `41205853504`, artifact `13046085461`, `sdk-macosx-x86_64`; the workflow verifies its SHA256. GitHub Actions artifacts expire, so this diagnostic intentionally fails if the pinned input is no longer available. It never silently switches versions.

The game draws a map and a solid red rectangle. The engine saves its own early/final PNGs; Swift/AppKit checks logical coordinates with drawable scaling. `evidence/result.json` separates process exit, harness termination, GPU identity, and pixel results. Engine validation output and PNGs are retained even on failure. A passing screenshot probe does not prove all native C++ rendering tests pass.

Run the workflow manually from Actions, or locally on Intel macOS after unpacking the SDK beneath `sdk/`: `python3 probe.py`.

No private machine information or credentials are included. The workflow has read-only repository permissions.
