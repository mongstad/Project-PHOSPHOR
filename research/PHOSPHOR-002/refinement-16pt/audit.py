import hashlib, json, os, subprocess
from pathlib import Path
from PIL import Image, ImageChops
root=Path(__file__).resolve().parent
p=root/'evidence'
historical=Image.open(root.parent/'evidence/native/16pt-material-client.png').convert('RGB')
for name in ['original','original-current']:
    im=Image.open(p/f'{name}-client.png').convert('RGB')
    assert im.size==(1280,900) and ImageChops.difference(im,historical).getbbox() is None
geometries=[]
for name in ['original-current','refined']:
    im=Image.open(p/f'{name}-client.png')
    assert im.size==(1280,900)
    assert Image.open(p/f'{name}-crop-100pct.png').tobytes()==im.crop((16,180,656,515)).tobytes()
    g=json.loads((p/f'{name}-geometry.json').read_text())[0]
    geometries.append({k:g[k] for k in ['frame','client','buffer']})
    assert 'DPR 1 screen QRect(0,0 2560x1440) logical DPI 96' in (p/f'{name}.log').read_text()
assert geometries[0]==geometries[1]
assert json.loads((p/'displays-before.json').read_text())['outputs']==json.loads((p/'displays-after.json').read_text())['outputs']
refined=Image.open(p/'refined-client.png').convert('RGB')
assert ImageChops.difference(historical,refined).getbbox() is not None
for args in [['--refine-16'],['--refine-16','--points','16','--baseline']]:
    result=subprocess.run(['/tmp/phosphor-002/clang-build/phosphor-002',*args],env=dict(os.environ,QT_QPA_PLATFORM='offscreen',QT_FORCE_STDERR_LOGGING='1'),capture_output=True)
    assert result.returncode==2,result.stderr
print('PASS: reference pixel equality, matched 16 pt geometry/DPI, unchanged display state, exact crops, candidate differs, CLI rejects invalid refinement modes')
