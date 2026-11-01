"""Run a pinned SDK without rebuilding the engine; preserve evidence on failure."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

variant = sys.argv[1] if len(sys.argv) > 1 else 'discrete'
evidence = Path('evidence') / variant
evidence.mkdir(exist_ok=True)
exe = next(Path('sdk').rglob('bin/eve')).resolve()
for name in ('clear-red', 'clear-green', 'early', 'final'):
    Path('/tmp/eve-map-' + name + '.png').unlink(missing_ok=True)
started = time.monotonic()
with (evidence / 'engine.log').open('w', encoding='utf-8') as log:
    child = subprocess.Popen(
        [str(exe), 'run', str(Path('game').resolve())], stdout=log,
        stderr=subprocess.STDOUT,
        env={**os.environ, 'EVENGINE_VULKAN_VALIDATION': '1',
             'MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS': '1' if variant == 'argument' else '0',
             'MTL_DEBUG_LAYER': '1' if variant.startswith('metal-validation') else '0',
             'MVK_CONFIG_LOG_LEVEL': '4'},
    )
    timed_out = False
    completed_capture = False
    try:
        deadline = time.monotonic() + 30
        while child.poll() is None and time.monotonic() < deadline:
            if Path('/tmp/eve-map-final.png').exists():
                completed_capture = True
                break
            time.sleep(0.1)
        if child.poll() is None:
            if not completed_capture:
                raise subprocess.TimeoutExpired(child.args, 30)
            child.terminate()
            child.wait(timeout=3)
    except subprocess.TimeoutExpired:
        timed_out = True
        child.terminate()
        try:
            child.wait(timeout=3)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait()
result = {'exit_code': child.returncode, 'harness_timeout': timed_out,
          'terminated_after_capture': completed_capture, 'variant': variant,
          'seconds': time.monotonic() - started, 'frames': {}}
for name in ('clear-red', 'clear-green', 'early', 'final'):
    source = Path('/tmp/eve-map-' + name + '.png')
    if source.exists():
        target = evidence / source.name
        shutil.copyfile(source, target)
        # macOS ships Swift/AppKit: no pip dependency or desktop screenshot.
        output = subprocess.check_output(['swift', 'pixels.swift', str(target)], text=True)
        result['frames'][name] = json.loads(output)
text = (evidence / 'engine.log').read_text(encoding='utf-8', errors='replace')
displays = Path('evidence/displays.txt').read_text(encoding='utf-8', errors='replace')
result['apple_paravirt_observed'] = (
    'AppleParavirtDevice' in text or 'Apple Paravirtualized Graphics Device' in displays
)
final = result['frames'].get('final', {})
result['map_visible'] = final.get('map_difference', 0) > 0.05
result['solid_visible'] = final.get('solid_red', 0) > 0.8
(evidence / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
print(text)
print(json.dumps(result, indent=2))
sys.exit(0 if result['map_visible'] and result['solid_visible'] else 1)
