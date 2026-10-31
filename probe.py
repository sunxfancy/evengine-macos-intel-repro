"""Run a pinned SDK without rebuilding the engine; preserve evidence on failure."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

evidence = Path('evidence')
evidence.mkdir(exist_ok=True)
exe = next(Path('sdk').rglob('bin/eve')).resolve()
started = time.monotonic()
with (evidence / 'engine.log').open('w', encoding='utf-8') as log:
    child = subprocess.Popen(
        [str(exe), 'run', str(Path('game').resolve())], stdout=log,
        stderr=subprocess.STDOUT,
        env={**os.environ, 'EVENGINE_VULKAN_VALIDATION': '1',
             'MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS': '0'},
    )
    timed_out = False
    try:
        child.wait(timeout=30)
    except subprocess.TimeoutExpired:
        timed_out = True
        child.terminate()
        try:
            child.wait(timeout=3)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait()
result = {'exit_code': child.returncode, 'harness_timeout': timed_out,
          'seconds': time.monotonic() - started, 'frames': {}}
for name in ('warm', 'early', 'final'):
    source = Path('/tmp/eve-map-' + name + '.png')
    if source.exists():
        target = evidence / source.name
        shutil.copyfile(source, target)
        # macOS ships Swift/AppKit: no pip dependency or desktop screenshot.
        output = subprocess.check_output(['swift', 'pixels.swift', str(target)], text=True)
        result['frames'][name] = json.loads(output)
text = (evidence / 'engine.log').read_text(encoding='utf-8', errors='replace')
result['apple_paravirt_observed'] = 'AppleParavirtDevice' in text
final = result['frames'].get('final', {})
result['map_visible'] = final.get('map_difference', 0) > 0.05
result['solid_visible'] = final.get('solid_red', 0) > 0.8
(evidence / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
print(text)
print(json.dumps(result, indent=2))
sys.exit(0 if result['map_visible'] and result['solid_visible'] else 1)
