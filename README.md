# EVEngine Intel macOS rendering reproduction

Small diagnostic for EVEngine/EVEngine PR #701. Uses `macos-15-intel`, records the actual GPU, and runs an existing SDK for at most 30 seconds. No engine build, timeout increases, or test skips.

Pinned input: upstream run `41205853504`, artifact `13046085461`, `sdk-macosx-x86_64`; the workflow verifies its SHA256. GitHub Actions artifacts expire, so this diagnostic intentionally fails if the pinned input is no longer available. It never silently switches versions.

The game draws a map and a solid red rectangle. The engine saves its own early/final PNGs; Swift/AppKit checks logical coordinates with drawable scaling. `evidence/result.json` separates process exit, harness termination, GPU identity, and pixel results. Engine validation output and PNGs are retained even on failure. A passing screenshot probe does not prove all native C++ rendering tests pass.

Run the workflow manually from Actions, or locally on Intel macOS after unpacking the SDK beneath `sdk/`: `python3 probe.py`.

No private machine information or credentials are included. The workflow has read-only repository permissions.

## Diagnostic comparisons

The script changes clear colors before restoring the reference background, and scans the entire image for red/blue geometry (rather than relying only on a single pixel). Separate processes compare discrete bindings, argument buffers, and Metal API validation. Native Metal controls generate vertex positions in the shader and upload positions through a buffer, using the public upstream probe at the pinned source commit.

Run 41264612582 observed changing clear colors but no red/blue geometry in discrete mode. Both native Metal controls passed with center RGBA 255,0,0,255. Argument-buffer mode exited with SIGABRT before rendering. These findings narrow the investigation to the engine/Vulkan/MoltenVK draw path; they do not identify the faulty component or prove every readback path is correct.

Run 41265312401 further isolated the behavior in a standalone Vulkan program, without EVEngine, VKBuilder, SDL or a swapchain. The exact same embedded engine color shaders render an offscreen triangle: SDK MoltenVK 1.2.11 returns the blue clear color with Metal validation off, and the red triangle with validation on. The program also passes locally on Linux Lavapipe. An additional lane compares the unmodified official MoltenVK 1.4.2 runtime; this is diagnostic selection of an upstream binary, not an engine dependency update or patched implementation.
