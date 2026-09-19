#!/usr/bin/env python3
"""RE4 (GameCube) skeletal motion export: the "FCV" motion entries of the character archives
(files/em/plNN.drs, emNN.drs, wepNN.drs) evaluated with the game's own code (tools/motion/host)
and written as glTF 2.0 (and BVH) animations on the model's parts hierarchy, with the character's
skinned, textured mesh from the model .bin / .tpl entries.

usage: motion_export.py list <archive|disc>
       motion_export.py export <archive|disc> --motion N [--archive plNN.drs] [--model <bin|archive:index>]
                               [--tpl N] [--mesh BIN[:TPL] ...] [--no-mesh]
                               -o out.gltf [--bvh out.bvh] [--no-ik] [--no-root] [--fps 30] [--scale 0.001]
                               [--blender-check [render_dir] [--strip]]
       motion_export.py export <archive> --all -o <dir> [--model ...]
       motion_export.py verify <archive|disc> [--dump <file.bin> | --dolphin] [--fma]

N is the entry index of the archive (the game's PL_ARC index minus 4). --model defaults to entry 0
of the motion's archive (the character's body .bin), its texture palette to the entry after it
(--tpl). --mesh adds attachment models of the archive (Leon: head 4:3, hair 2:3, eyes 5:3, right
hand 14:13, left hand 17:13, as cPlLeon::setModel loads them). --archive picks the archive of a disc.
`verify` re-serialises every motion, sequence table and model and counts byte-identical round-trips,
decodes every texture (re-encoding the lossless formats), then compares the helper's poses with a
Dolphin memory dump when given one (see tools/motion/README.md).
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from motion import archive, fcv, modelbin, meshbin, gxtex, evalhost, gltf, bvh  # noqa: E402


def load_motion(entry):
    if entry.tag != 'FCV':
        sys.exit(f'{entry.name}: tag {entry.tag!r}, not a motion (FCV)')
    return fcv.parse(entry.data)


def load_model(ref, arc):
    """(skeleton, name, body entry or None for a .bin file)."""
    if ref is None:
        e = arc.entry(0)
    elif os.path.isfile(ref):
        m = modelbin.parse(open(ref, 'rb').read())
        m.check_tree()
        return m, ref, None
    else:
        e = archive.resolve_ref(ref, arc)
    if e.tag != 'BIN':
        sys.exit(f'{e.name}: tag {e.tag!r}, not a model (BIN)')
    m = modelbin.parse(e.data)
    m.check_tree()
    return m, e.name, e


def mesh_source(arc, bin_entry, tpl_entry):
    if bin_entry.tag != 'BIN':
        sys.exit(f'{bin_entry.name}: tag {bin_entry.tag!r}, not a model (BIN)')
    if tpl_entry.tag != 'TPL':
        sys.exit(f'{tpl_entry.name}: tag {tpl_entry.tag!r}, not a texture palette (TPL); pass the TPL entry explicitly')
    mesh = meshbin.parse(bin_entry.data)
    textures = gxtex.parse_tpl(tpl_entry.data)
    if mesh.n_tex > len(textures):
        sys.exit(f'{bin_entry.name}: model wants {mesh.n_tex} textures, {tpl_entry.name} has {len(textures)}')
    stem = os.path.splitext(arc.name)[0]
    return gltf.MeshSource(mesh, textures, f'{stem}_{bin_entry.index:03d}', f'{stem}_{tpl_entry.index:03d}')


def load_meshes(args, arc, body):
    """[MeshSource]: the body .bin with its TPL (--tpl, default: the entry after the body) and every
    --mesh BIN[:TPL] attachment (head, hair, hands ... of the same archive; skinned to the body's parts)."""
    if args.no_mesh:
        return None
    if body is None:
        sys.exit('--model is a file: pass --no-mesh (no texture palette to go with it)')
    srcs = [mesh_source(arc, body, arc.entry(args.tpl if args.tpl is not None else body.index + 1))]
    for spec in args.mesh or []:
        bi, _, ti = spec.partition(':')
        srcs.append(mesh_source(arc, arc.entry(int(bi, 0)), arc.entry(int(ti, 0) if ti else int(bi, 0) + 1)))
    return srcs


def pick_archive(args):
    arcs = archive.open_source(args.source)
    if len(arcs) == 1:
        return arcs[0]
    if not args.archive:
        sys.exit('a disc holds many archives: pass --archive plNN.drs')
    for a in arcs:
        if a.name == args.archive:
            return a
    sys.exit(f'{args.archive}: not on the disc')


def cmd_list(args):
    for arc in archive.open_source(args.source):
        print(f'{arc.name}:')
        entries = list(arc.entries())
        for e in entries:
            if e.tag == 'BIN':
                try:
                    m = modelbin.parse(e.data)
                    kind = 'skeleton' if all(p.attach == p.no for p in m.parts) else 'attachment model'
                    print(f'  [{e.index:3}] arc {e.arc_no:#04x} BIN  {kind}, {m.n_parts} parts, version {m.version:#x}; '
                          f'mesh: {meshbin.describe(meshbin.parse(e.data))}')
                except Exception as ex:
                    print(f'  [{e.index:3}] arc {e.arc_no:#04x} BIN  ({ex})')
            elif e.tag == 'TPL':
                try:
                    ts = gxtex.parse_tpl(e.data)
                    print(f'  [{e.index:3}] arc {e.arc_no:#04x} TPL  {len(ts)} textures: '
                          + ', '.join(f'{t.fmt_name} {t.width}x{t.height}' + (f' {t.max_lod + 1} mips' if t.max_lod else '') for t in ts))
                except Exception as ex:
                    print(f'  [{e.index:3}] arc {e.arc_no:#04x} TPL  ({ex})')
            elif e.tag == 'FCV':
                try:
                    m = fcv.parse(e.data)
                    kinds = sorted(set(j.target() or ('root_pos' if j.is_root_pos() else 'root_rot') for j in m.joints))
                    ik = sum(1 for j in m.joints if j.kind & 0x30)
                    print(f'  [{e.index:3}] arc {e.arc_no:#04x} FCV  {m.max_frame + 1:4} frames, {len(m.joints):2} joints'
                          f' ({", ".join(kinds)}), {ik} IK chains, parts <= {max([j.parts_no for j in m.joints], default=-1)}')
                except Exception as ex:
                    print(f'  [{e.index:3}] arc {e.arc_no:#04x} FCV  MALFORMED: {ex}')
            elif e.tag == 'SEQ':
                s = fcv.parse_seq(e.data)
                print(f'  [{e.index:3}] arc {e.arc_no:#04x} SEQ  {len(s.keys)} keys, flags {s.flags:#x}, frames {s.keys[0].frame_f:g}..{s.keys[-1].frame_f:g}' if s.keys else f'  [{e.index:3}] SEQ empty')


def play(model, motion, args):
    player = evalhost.Player(model, motion, variant='fma' if args.fma else 'plain', ik=not args.no_ik)
    return [player.frame(f) for f in range(motion.n_frames)]


def export_one(arc, entry, model, model_name, meshes, out, args):
    motion = load_motion(entry)
    poses = play(model, motion, args)
    name = f'{os.path.splitext(arc.name)[0]}_{entry.index:03d}'
    extras = {'re4': {'archive': arc.name, 'entry': entry.index, 'arc_index': entry.arc_no, 'model': model_name,
                      'max_frame': motion.max_frame, 'fps': args.fps, 'units': f'mm x {args.scale}',
                      'ik': not args.no_ik,
                      'meshes': [s.label for s in meshes] if meshes else [],
                      'joints': [{'kind': j.kind, 'channel': j.channel, 'fcc_type': j.fcc_type, 'parts': j.parts_no}
                                 for j in motion.joints]}}
    _, stats = gltf.export(model, motion, poses, out, name, fps=args.fps, scale=args.scale, root_motion=not args.no_root,
                           extras=extras, meshes=meshes)
    print(f'{entry.name}: {motion.n_frames} frames, {len(motion.joints)} joints -> {out}')
    if meshes:
        print(f'  mesh: {stats["meshes"]} .bin, {stats["vertices"]} vertices, {stats["triangles"]} triangles, {stats["materials"]} materials'
              + (f', {stats["duplicate_faces"]} duplicate face(s) dropped' if stats['duplicate_faces'] else ''))
    if args.bvh:
        bvh_path = args.bvh if not args.all else os.path.splitext(out)[0] + '.bvh'
        bvh.export(model, poses, bvh_path, fps=args.fps, scale=args.scale, name=name)
        print(f'  BVH -> {bvh_path}')
    if args.blender_check is not None:
        blender_check(model, motion, poses, out, name, stats if meshes else None, args)


def blender_check(model, motion, poses, out, name, mesh_stats, args):
    """Headless Blender import of `out` (tools/motion/blender_check.py): bone count, frame range,
    bone world positions against the helper's parts world positions (glTF Y-up -> Blender Z-up:
    (x, y, z) -> (x, -z, y)), the mesh's vertex / triangle counts, 4 rendered frames (textured
    when there is a mesh) and, with --strip, every frame at 30 fps as a film strip + mp4."""
    import json
    import subprocess
    n = motion.n_frames
    frames = sorted(set([0, n // 3, (2 * n) // 3, n - 1]))
    samples = {}
    for f in frames:
        pose = poses[f]
        bones = {}
        for i in range(model.n_parts):
            w = pose.world[i]
            if args.no_root:
                x, y, z = w
            else:   # the model node carries the root keys: world = RotMatrix(root_rot) * parts + root_pos
                m = pose.root_mat
                x = m[0] * w[0] + m[1] * w[1] + m[2] * w[2] + pose.root_pos[0]
                y = m[4] * w[0] + m[5] * w[1] + m[6] * w[2] + pose.root_pos[1]
                z = m[8] * w[0] + m[9] * w[1] + m[10] * w[2] + pose.root_pos[2]
            bones[f'parts_{i:03d}'] = [x * args.scale, -z * args.scale, y * args.scale]
        samples[str(f)] = bones
    ref = {'name': name, 'bones': model.n_parts, 'frames': n, 'fps': args.fps, 'units': f'mm x {args.scale}',
           'tolerance': 2e-4 * args.scale / 0.001, 'samples': samples, 'mesh': mesh_stats, 'strip': bool(args.strip)}
    ref_path = os.path.splitext(out)[0] + '.ref.json'
    with open(ref_path, 'w') as f:
        json.dump(ref, f)
    script = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'motion', 'blender_check.py')
    cmd = ['blender', '-b', '--python', script, '--', out, ref_path]
    if args.blender_check:
        cmd.append(args.blender_check)
    r = subprocess.run(cmd, capture_output=True, text=True)
    lines = [l for l in r.stdout.splitlines() if l.startswith('blender:')]
    print('\n'.join('  ' + l for l in lines))
    if r.returncode or 'blender: OK' not in lines:
        sys.exit(f'blender check failed:\n{r.stdout[-3000:]}\n{r.stderr[-3000:]}')


def cmd_export(args):
    arc = pick_archive(args)
    model, model_name, body = load_model(args.model, arc)
    meshes = load_meshes(args, arc, body)
    if args.all:
        os.makedirs(args.output, exist_ok=True)
        for e in arc.entries('FCV'):
            try:
                m = fcv.parse(e.data)
            except ValueError as ex:
                print(f'{e.name}: skipped, malformed motion ({ex})')
                continue
            if any(j.target() is not None and j.parts_no >= model.n_parts for j in m.joints):
                print(f'{e.name}: skipped, targets parts beyond the model ({model.n_parts})')
                continue
            out = os.path.join(args.output, f'{os.path.splitext(arc.name)[0]}_{e.index:03d}.gltf')
            export_one(arc, e, model, model_name, meshes, out, args)
        return
    if args.motion is None:
        sys.exit('export: --motion N or --all')
    export_one(arc, arc.entry(args.motion), model, model_name, meshes, args.output, args)


def cmd_verify(args):
    arcs = archive.open_source(args.source)
    total = ok = 0
    seq_total = seq_ok = 0
    bin_total = bin_ok = 0
    tpl_total = tex_total = tex_lossless = tex_ok = 0
    failures = []
    for arc in arcs:
        for e in arc.entries('BIN'):
            bin_total += 1
            try:
                if meshbin.serialise(meshbin.parse(e.data)) == e.data:
                    bin_ok += 1
                else:
                    failures.append(f'{e.name}: model re-serialisation differs')
            except ValueError as ex:
                failures.append(f'{e.name}: model {ex}')
        for e in arc.entries('TPL'):
            tpl_total += 1
            try:
                ts = gxtex.parse_tpl(e.data)
                tex_total += len(ts)
                n, k, f = gxtex.verify(ts)
                tex_lossless += n
                tex_ok += k
                failures += [f'{e.name}: {x}' for x in f]
            except ValueError as ex:
                failures.append(f'{e.name}: palette {ex}')
        for e in arc.entries('FCV'):
            total += 1
            try:
                m = fcv.parse(e.data)
                if fcv.serialise(m) == e.data:
                    ok += 1
                else:
                    failures.append(f'{e.name}: re-serialisation differs')
            except ValueError as ex:
                failures.append(f'{e.name}: {ex}')
        for e in arc.entries('SEQ'):
            seq_total += 1
            try:
                if fcv.serialise_seq(fcv.parse_seq(e.data)) == e.data:
                    seq_ok += 1
                else:
                    failures.append(f'{e.name}: sequence re-serialisation differs')
            except ValueError as ex:
                failures.append(f'{e.name}: sequence {ex}')
    print(f'{len(arcs)} archives: {total} motions, {ok} byte-identical round-trips; {seq_total} sequence tables, {seq_ok} byte-identical')
    print(f'  {bin_total} models, {bin_ok} byte-identical round-trips; {tpl_total} texture palettes, {tex_total} textures decoded, '
          f'{tex_lossless} lossless (I4 / IA8), {tex_ok} byte-identical re-encodings')
    for f in failures:
        print(f'  FAIL {f}')
    status = 0 if not failures else 1
    if args.dump or args.dolphin:
        from motion import dolphin
        if args.dolphin:
            dumps = dolphin.capture(args.source, args.dolphin_iso, args.dump_out)
        else:
            dumps = [args.dump]
        for dump_path in dumps:
            status |= dolphin.compare(dump_path, arcs, variant='fma' if args.fma else 'plain', verbose=True)
            if args.fma_too:
                status |= dolphin.compare(dump_path, arcs, variant='fma', verbose=True)
    sys.exit(status)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    p = sub.add_parser('list')
    p.add_argument('source')
    p.set_defaults(func=cmd_list)
    p = sub.add_parser('export')
    p.add_argument('source')
    p.add_argument('--archive', help='archive name when the source is a disc')
    p.add_argument('--motion', type=lambda s: int(s, 0))
    p.add_argument('--all', action='store_true')
    p.add_argument('--model', help='model .bin file or archive:index (default: entry 0 of the archive)')
    p.add_argument('--tpl', type=lambda s: int(s, 0), help='texture palette entry of the body (default: the entry after the model)')
    p.add_argument('--mesh', action='append', metavar='BIN[:TPL]',
                   help='attachment model entry (head, hair, hands ...) skinned to the body parts, with its TPL entry (default BIN + 1); repeatable')
    p.add_argument('--no-mesh', action='store_true', help='skeleton only (stick-figure placeholder mesh)')
    p.add_argument('--strip', action='store_true', help='with --blender-check RENDER_DIR: also render every frame into a film strip PNG and an mp4')
    p.add_argument('-o', '--output', required=True)
    p.add_argument('--bvh')
    p.add_argument('--no-ik', action='store_true', help='skip the IK (raw keys on the parts)')
    p.add_argument('--no-root', action='store_true', help='no root motion track on the model node')
    p.add_argument('--fps', type=float, default=30.0)
    p.add_argument('--scale', type=float, default=0.001, help='units per game millimetre (default 0.001: metres)')
    p.add_argument('--fma', action='store_true', help='use the fused multiply-add helper build')
    p.add_argument('--blender-check', nargs='?', const='', metavar='RENDER_DIR',
                   help='import the result in headless Blender, compare bone positions, render 4 frames to RENDER_DIR')
    p.set_defaults(func=cmd_export)
    p = sub.add_parser('verify')
    p.add_argument('source')
    p.add_argument('--dump', help='Dolphin memory dump (tools/motion/dolphin.py capture) to compare poses against')
    p.add_argument('--dolphin', action='store_true', help='run the game in Dolphin and capture a dump first')
    p.add_argument('--dolphin-iso', default='orig/G4BE08/re4_debug_disc1.iso')
    p.add_argument('--dump-out', default='/tmp/mot/dolphin_dump.bin')
    p.add_argument('--fma', action='store_true')
    p.add_argument('--fma-too', action='store_true', help='also compare with the fused multiply-add helper build')
    p.set_defaults(func=cmd_verify)
    args = ap.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
