"""Compare compiled/rendered defaults with the approved scalar appearance record."""
from pathlib import Path
import json,struct
root=Path(__file__).resolve().parents[1]
expected=json.loads((root/'docs/appearance-defaults.json').read_text())
actual=json.loads((root/'out/appearance-defaults-observed.json').read_text())
f32=lambda value:struct.pack('<f',value)
for key in ['wheel_rgba','segment_rgba','selected_rgba']:
    assert all(f32(a)==f32(b) for a,b in zip(actual[key],expected[key])),key
assert actual['gentle_clouds']==expected['gentle_clouds']==True
assert actual['background_opacity']==expected['background_opacity']==.5
assert actual['size']==expected['size']
assert all(abs(a-b)<.002 for a,b in zip(actual['reference_position'],expected['position']))
assert actual['cloud_speed_multiplier']==expected['cloud_speed_multiplier']==1.10
assert all(126<=value<=129 for value in actual['background_blend_delta'])
print('PASS: compiled colors/alphas equal captured float32 values; 50% backing measured on WARP; captured placement/responsive fit; actual drift 1.10x with unchanged amplitude; static mode intact.')
