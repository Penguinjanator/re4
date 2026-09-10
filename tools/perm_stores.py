#!/usr/bin/env python3
"""perm.py <mod> <func> <spec.json> [maxperms]
spec.json: {"anchor": "<text of the block to replace>", "fixed_head": ["stmt", ...], "perm": ["stmt", ...],
            "fixed_tail": [...], "indent": "        "}
Every permutation of "perm" (between head and tail) replaces the anchor block in src/<mod>/<mod>.cpp
(temp copy), compiles with the module flags, and prints the masked diff word count of <func>.
"""
import sys, json, os, subprocess, tempfile, itertools
R = '/home/adityas/Projects/re4'
mod, func, sfile = sys.argv[1:4]
maxp = int(sys.argv[4]) if len(sys.argv) > 4 else 100000
spec = json.load(open(sfile))
src = open(f'{R}/src/{mod}/{mod}.cpp').read()
assert spec['anchor'] in src, 'anchor not found'
ind = spec.get('indent', '        ')
tmpd = tempfile.mkdtemp(prefix='perm_', dir='/tmp/em_last4')
cflags = ['-O2', '-mfast-cast', '-I', f'{R}/include', '-I', f'{R}/src', '-I', f'{R}/build/G4BE08/include',
          '-DBUILD_VERSION=0', '-DVERSION_G4BE08', '-DNDEBUG=1', '-G', '0', f'-DREL_MODULE={mod}', '-lang=c++']
best = None
n = 0
for perm in itertools.permutations(spec['perm']):
    n += 1
    if n > maxp:
        break
    body = ''.join(ind + x + '\n' for x in spec.get('fixed_head', []) + list(perm) + spec.get('fixed_tail', []))
    s = src.replace(spec['anchor'], body)
    d = os.path.join(tmpd, f'p{n}')
    os.makedirs(d, exist_ok=True)
    cpp = os.path.join(d, f'{mod}.cpp')
    open(cpp, 'w').write(s)
    obj = os.path.join(d, f'{mod}.o')
    cmd = ['python3', f'{R}/tools/ngccc.py', '--prodg-dir', 'build/compilers/ProDG/3.9.3', '--wrapper',
           'build/tools/wibo', '--native-dir', 'build/compilers/ProDG/3.9.3-v1.79', '-c', cpp, '-o', obj] + cflags
    r = subprocess.run(cmd, cwd=R, capture_output=True, text=True)
    if r.returncode != 0:
        print(f'p{n}: COMPILE ERROR\n{r.stderr[-400:]}')
        break
    r2 = subprocess.run(['python3', '/tmp/em_last4/mcmp.py', mod, '-q', func], cwd=R,
                        capture_output=True, text=True, env=dict(os.environ, OBJ=obj))
    cnt = int(r2.stdout.strip() or 9999)
    if best is None or cnt < best[0]:
        best = (cnt, perm)
        print(f'p{n}: {cnt}  {perm}', flush=True)
    if cnt == 0:
        break
    subprocess.run(['rm', '-rf', d])
print('best', best)
