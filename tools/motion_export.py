#!/usr/bin/env python3
"""RE4 (GameCube) skeletal motion export: the "FCV" motion entries of the character archives
(files/em/plNN.drs, emNN.drs, wepNN.drs) and the yz2-compressed room archives (files/St*/rNNN.das)
evaluated with the game's own code (tools/motion/host) and written as glTF 2.0 (and BVH)
animations on the model's parts hierarchy, with the character's skinned, textured mesh from the
model .bin / .tpl entries.

usage: motion_export.py list <archive|disc> [--character leon]
       motion_export.py export <archive|disc> --motion N [--archive plNN.drs|rNNN.das] [--character leon | --model <bin|[stem:]index>]
                               [--tpl N] [--body-only] [--mesh [stem:]BIN[:[stem:]TPL] ...] [--no-mesh]
                               -o out.gltf [--bvh out.bvh] [--no-ik] [--no-root] [--fps 30] [--scale 0.001]
                               [--blender-check [render_dir] [--strip]] [--render-nice DIR]
       motion_export.py export <archive> --all -o <dir> [--model ...]
       motion_export.py verify <archive|disc> [--dump <file.bin> | --dolphin] [--fma]

N is the entry index of the archive (the game's PL_ARC index minus 4). --model defaults to the
character table's body of the motion's archive (its entry 0 for the players), its texture palette
to the entry after it (--tpl). The head, hair, eyes, hands ... of a known character come along by
default: the table in tools/motion/character.py, read off the game's set-up code
(cPlLeon::setModel, cPlAda::setModel, cPlAshley::setModel, em10ModelInit ...). --body-only leaves
them out; --mesh adds an attachment (or replaces the palette of one in the table); an entry of
another archive is written 'stem:N' (the weapon-grip hand of the shotgun module for Leon: --mesh
wep04:6:13). An attachment the disc cannot render (empty entry, texture id beyond the palette) is
reported and skipped. --archive picks the archive of a disc.
The player's melee motions are split over two archives (character.MELEE): the roundhouse kick is
PL_ARC 0x25 of the player's archive, the alternate kick (0x29D), suplex (0xD6), knee kick / palm
strike (0x2B4) and neck break (0x2B9) are entries of the Ganado's em10.drs that the em10 routines
play on the player (`pl->subArc = em->subArc`). `list --character leon` shows them;
`export --archive em10.drs --motion 665 --character leon` plays one on Leon's body.
`verify` decompresses every .das, checks the container, re-serialises every motion, sequence table
and model and counts byte-identical round-trips, decodes every texture (re-encoding the lossless
formats), then compares the helper's poses with a Dolphin memory dump when given one (see
tools/motion/README.md).
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from motion import archive, fcv, modelbin, meshbin, gxtex, evalhost, gltf, bvh, character  # noqa: E402


def load_motion(entry):
    if entry.tag != 'FCV':
        sys.exit(f'{entry.name}: tag {entry.tag!r}, not a motion (FCV)')
    return fcv.parse(entry.data)


def table_body(stem):
    """(stem, entry) of the character table's body for an archive stem, or (stem, 0)."""
    table = character.attachments(stem)
    body = [a for a in table if a.role == 'body'][0] if table else None
    return (stem, body.bin) if body else (stem, 0)


def load_model(ref, arc, src):
    """(skeleton, name, body entry or None for a .bin file). Default: the character table's body
    entry of the motion's archive (em10: 440), else entry 0; 'stem:N' takes the body from another
    archive of the source (the player's, for a melee motion stored in the Ganado's em10.drs)."""
    if ref is None:
        e = arc.entry(table_body(arc.stem)[1])
        if e.tag != 'BIN':
            sys.exit(f'{arc.name} is not in character.TABLE and its entry 0 is {e.tag!r}, not a model: pass '
                     f'--character NAME or --model [stem:]N (a room archive holds no default body)')
    elif os.path.isfile(ref):
        m = modelbin.parse(open(ref, 'rb').read())
        m.check_tree()
        return m, ref, None
    else:
        try:
            e = src.entry(character.parse_ref(ref, arc.stem))
        except MeshSourceError as ex:
            sys.exit(f'--model {ref}: {ex}')
    if e.tag != 'BIN':
        sys.exit(f'{e.name}: tag {e.tag!r}, not a model (BIN)')
    m = modelbin.parse(e.data)
    m.check_tree()
    return m, e.name, e


class MeshSourceError(Exception):
    pass


def mesh_source(bin_entry, tpl_entry, role=None):
    if bin_entry.tag != 'BIN':
        raise MeshSourceError(f'{bin_entry.name}: tag {bin_entry.tag!r}, not a model (BIN)' + (' (empty entry)' if not bin_entry.data else ''))
    if tpl_entry.tag != 'TPL':
        raise MeshSourceError(f'{tpl_entry.name}: tag {tpl_entry.tag!r}, not a texture palette (TPL); pass the TPL entry explicitly')
    mesh = meshbin.parse(bin_entry.data)
    textures = gxtex.parse_tpl(tpl_entry.data)
    # materialSetup binds part.texId (and alphaTex / bumpTex when the flags say so) from the palette:
    # those must exist; the header's nTex may exceed the palette (pl01:6 says 18, uses texture 13 of 17)
    used = {p.tex_id for p in mesh.parts} | {p.alpha_tex for p in mesh.parts if p.flags & 4} | {p.bump_tex for p in mesh.parts if p.flags & 1}
    if max(used) >= len(textures):
        raise MeshSourceError(f'{bin_entry.name}: model uses texture {max(used)}, {tpl_entry.name} has {len(textures)}')
    bstem = os.path.splitext(bin_entry.archive)[0]
    tstem = os.path.splitext(tpl_entry.archive)[0]
    return gltf.MeshSource(mesh, textures, f'{bstem}_{bin_entry.index:03d}', f'{tstem}_{tpl_entry.index:03d}', role)


def parse_mesh_spec(spec, stem):
    """--mesh '[stem:]BIN[:[stem:]TPL]' -> ((stem, bin), (stem, tpl)); TPL defaults to BIN + 1."""
    toks = spec.split(':')
    refs = []
    while toks:
        if toks[0].isdigit() or toks[0].lower().startswith('0x'):
            refs.append((stem, int(toks.pop(0), 0)))
        elif len(toks) >= 2:
            refs.append((toks[0], int(toks[1], 0)))
            toks = toks[2:]
        else:
            sys.exit(f'--mesh {spec}: expected [stem:]BIN[:[stem:]TPL]')
    if len(refs) == 1:
        refs.append(None)
    if len(refs) != 2:
        sys.exit(f'--mesh {spec}: expected [stem:]BIN[:[stem:]TPL]')
    return refs[0], refs[1]


class Source:
    """The archives a source offers: every archive of a disc (em/*.drs, the St*/rNNN.das rooms,
    etc/*.das), or one container file and the sibling files of its directory (a cross-archive
    reference 'wep04:6' or 'em10:665' next to pl00.drs)."""

    def __init__(self, path):
        self.arcs = {a.name: a for a in archive.open_source(path)}
        self.is_disc = archive.is_disc(path)
        self.dir = None if self.is_disc else os.path.dirname(os.path.abspath(path))

    def get(self, stem):
        for ext in archive.EXTS:
            name = stem + ext
            if name in self.arcs:
                return self.arcs[name]
        if self.is_disc:
            return None
        for ext in archive.EXTS:
            p = os.path.join(self.dir, stem + ext)
            if os.path.isfile(p):
                self.arcs[stem + ext] = archive.Archive(p)
                return self.arcs[stem + ext]
        return None

    def entry(self, ref):
        stem, idx = ref
        arc = self.get(stem)
        if arc is None:
            raise MeshSourceError(f'{stem}:{idx}: archive {stem}.drs / .das not in the source')
        if idx >= len(arc.drs.entries):
            raise MeshSourceError(f'{stem}:{idx}: the archive has {len(arc.drs.entries)} entries')
        return arc.entry(idx)


def load_meshes(args, src, body):
    """[MeshSource]: the body .bin with its TPL (--tpl, default: the entry after the body), the
    character's attachments from character.TABLE when the body's archive is known (head, hair,
    hands ... as the game's setModel loads them; --body-only leaves them out) and every --mesh
    spec, which adds a model or replaces the palette of a table entry."""
    if args.no_mesh:
        return None
    if body is None:
        sys.exit('--model is a file: pass --no-mesh (no texture palette to go with it)')
    stem = os.path.splitext(body.archive)[0]
    body_ref = (stem, body.index)
    tpl_ref = (stem, args.tpl if args.tpl is not None else body.index + 1)
    wanted = [(body_ref, tpl_ref, 'body', 'the model archive')]
    table = character.attachments(stem)
    if args.body_only:
        pass
    elif table is None:
        print(f'note: {stem} is not in character.TABLE: body only (add attachments with --mesh)')
    elif body_ref != table_body(stem):
        print(f'note: --model {body.index} is not the character table\'s body ({table_body(stem)[1]}): body only')
    else:
        for a in table:
            if a.role == 'body':
                if args.tpl is None:
                    wanted[0] = (body_ref, character.ref(a.tpl, stem), 'body', a.source)
                continue
            wanted.append((character.ref(a.bin, stem), character.ref(a.tpl, stem), a.role, a.source))
    for spec in args.mesh or []:
        b, t = parse_mesh_spec(spec, stem)
        for i, w in enumerate(wanted):
            if w[0] == b:
                wanted[i] = (b, t or w[1], w[2], f'--mesh {spec}')
                break
        else:
            wanted.append((b, t or (b[0], b[1] + 1), 'mesh', f'--mesh {spec}'))
    srcs = []
    for b, t, role, source in wanted:
        try:
            srcs.append(mesh_source(src.entry(b), src.entry(t), role))
        except MeshSourceError as e:
            if role == 'body':
                sys.exit(str(e))
            # the disc cannot draw it either (pl02: empty hand entry, hand texture beyond the palette)
            print(f'note: {role} skipped: {e}')
    print('  models: ' + ', '.join(f'{s.role} {s.label} (tpl {s.tex_prefix})' for s in srcs))
    return srcs


def pick_archive(args, src):
    arcs = list(src.arcs.values())
    if len(arcs) == 1:
        return arcs[0]
    if not args.archive:
        sys.exit('a disc holds many archives: pass --archive plNN.drs (or rNNN.das)')
    if args.archive not in src.arcs:
        sys.exit(f'{args.archive}: not on the disc')
    return src.arcs[args.archive]


def list_melee(src, stem):
    """The player's melee motions (character.MELEE): the entries the em10 routines play on the
    player, from the player's archive and the Ganado's."""
    rows = character.melee(stem)
    if not rows:
        return
    print(f'melee motions of {stem} (em10/em10.cpp, pl->subArc = em->subArc):')
    for arc_stem, m in rows:
        arc = src.get(arc_stem)
        if arc is None:
            print(f'  {arc_stem}:{m.index}  {m.role}: archive not in the source')
            continue
        e = arc.entry(m.entry)
        desc = describe_entry(e)
        print(f'  {arc_stem}:{e.index:<3} arc {e.arc_no:#05x} {m.role:<16} {desc}')
        print(f'      {m.source}')


def describe_entry(e):
    if e.tag == 'FCV':
        try:
            m = fcv.parse(e.data)
            kinds = sorted(set(j.target() or ('root_pos' if j.is_root_pos() else 'root_rot') for j in m.joints))
            ik = sum(1 for j in m.joints if j.kind & 0x30)
            return (f'FCV  {m.max_frame + 1:4} frames, {len(m.joints):2} joints ({", ".join(kinds)}), {ik} IK chains, '
                    f'parts <= {max([j.parts_no for j in m.joints], default=-1)}')
        except Exception as ex:
            return f'FCV  MALFORMED: {ex}'
    return f'{e.tag:4} {len(e.data):#x} bytes'


def cmd_list(args):
    if args.character:
        stem = character.character_stem(args.character)
        src = Source(args.source)
        arc = src.get(stem)
        if arc is None:
            sys.exit(f'{stem}: not in the source')
        arcs = [arc]
    else:
        arcs = archive.open_source(args.source)
        src = None
    for arc in arcs:
        arc.drs
        print(f'{arc.name}:' + (f' (yz2 {arc.das.packed:#x} -> {arc.das.unpacked:#x} bytes)' if arc.das and arc.das.compressed else ''))
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
            elif e.tag:
                print(f'  [{e.index:3}] arc {e.arc_no:#04x} {e.tag:4} {len(e.data):#x} bytes')
    if args.character:
        list_melee(src, stem)


def play(model, motion, args):
    player = evalhost.Player(model, motion, variant='fma' if args.fma else 'plain', ik=not args.no_ik)
    return [player.frame(f) for f in range(motion.n_frames)]


def export_one(arc, entry, model, model_name, meshes, out, args):
    motion = load_motion(entry)
    poses = play(model, motion, args)
    name = f'{arc.stem}_{entry.index:03d}'
    if meshes and meshes[0].label.rsplit('_', 1)[0] != arc.stem:   # a melee motion on another archive's body
        name += '_' + meshes[0].label.rsplit('_', 1)[0]
    extras = {'re4': {'archive': arc.name, 'entry': entry.index, 'arc_index': entry.arc_no, 'model': model_name,
                      'max_frame': motion.max_frame, 'fps': args.fps, 'units': f'mm x {args.scale}',
                      'ik': not args.no_ik,
                      'meshes': [{'label': s.label, 'role': s.role, 'palette': s.tex_prefix, 'shape_keys': s.shape_names()}
                                 for s in meshes] if meshes else [],
                      'joints': [{'kind': j.kind, 'channel': j.channel, 'fcc_type': j.fcc_type, 'parts': j.parts_no}
                                 for j in motion.joints]}}
    _, stats = gltf.export(model, motion, poses, out, name, fps=args.fps, scale=args.scale, root_motion=not args.no_root,
                           extras=extras, meshes=meshes)
    print(f'{entry.name}: {motion.n_frames} frames, {len(motion.joints)} joints -> {out}')
    if meshes:
        print(f'  mesh: {stats["meshes"]} .bin, {stats["vertices"]} vertices, {stats["triangles"]} triangles, {stats["materials"]} materials'
              + (f', {stats["duplicate_faces"]} duplicate face(s) dropped' if stats['duplicate_faces'] else '')
              + ''.join(f'; {k}: shape keys {", ".join(v)}' for k, v in stats['shape_keys'].items()))
    if args.bvh:
        bvh_path = args.bvh if not args.all else os.path.splitext(out)[0] + '.bvh'
        bvh.export(model, poses, bvh_path, fps=args.fps, scale=args.scale, name=name)
        print(f'  BVH -> {bvh_path}')
    if args.blender_check is not None or args.render_nice:
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
    ref = {'name': name, 'bones': model.n_parts, 'frames': n, 'fps': args.fps, 'units': f'mm x {args.scale}', 'scale': args.scale,
           'tolerance': 2e-4 * args.scale / 0.001, 'samples': samples, 'mesh': mesh_stats, 'strip': bool(args.strip),
           'nice': args.render_nice}
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
    src = Source(args.source)
    arc = pick_archive(args, src)
    if args.character:
        if args.model:
            sys.exit('--character and --model: pass one')
        stem = character.character_stem(args.character)
        args.model = f'{stem}:{table_body(stem)[1]}'
    model, model_name, body = load_model(args.model, arc, src)
    meshes = load_meshes(args, src, body)
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
    bin_total = bin_ok = shaped = shape_keys = 0
    tpl_total = tex_total = tex_lossless = tex_ok = 0
    das_total = das_yz2 = das_ok = 0
    das_entries = 0
    failures = []
    for arc in list(arcs):
        # .das: the yz2 decode + the container parse (drs.Drs asserts the entry table: offsets
        # ascending, 0x20-aligned, within the body; the record table; the sound bank)
        if arc.ext == '.das':
            das_total += 1
            try:
                arc.drs
            except (AssertionError, ValueError, IndexError) as ex:
                failures.append(f'{arc.name}: {type(ex).__name__}: {ex}')
                arcs.remove(arc)
                continue
            das_yz2 += arc.das.compressed
            das_entries += len(arc.drs.entries)
            bad = [f'entry {i} tag {t!r}' for i, (t, _) in enumerate(arc.drs.entries)
                   if t != b'\0\0\0\0' and not (t[3] == 0 and all(48 <= c <= 90 for c in t[:3]))]
            if arc.das.compressed and arc.das.unpacked % 0x20:
                bad.append(f'unpacked size {arc.das.unpacked:#x} not 0x20-aligned')
            if arc.das.compressed and not 0 <= arc.das.used - arc.das.packed <= 4:
                bad.append(f'coder read {arc.das.used:#x} of a {arc.das.packed:#x} packed stream')
            failures += [f'{arc.name}: {b}' for b in bad]
            das_ok += not bad
    for arc in arcs:
        for e in arc.entries('BIN'):
            bin_total += 1
            try:
                m = meshbin.parse(e.data)
                if m.shapes:
                    shaped += 1
                    shape_keys += len(m.shapes)
                if meshbin.serialise(m) == e.data:
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
    # the character table against the source: every default attachment must load
    src = Source(args.source)
    tab_total = tab_ok = 0
    for stem, table in character.TABLE.items():
        if src.get(stem) is None:
            continue
        for a in table:
            tab_total += 1
            try:
                mesh_source(src.entry(character.ref(a.bin, stem)), src.entry(character.ref(a.tpl, stem)), a.role)
                tab_ok += 1
            except MeshSourceError as e:
                failures.append(f'character.TABLE {stem} {a.role}: {e}')
    print(f'{len(arcs)} archives: {total} motions, {ok} byte-identical round-trips; {seq_total} sequence tables, {seq_ok} byte-identical')
    if das_total:
        print(f'  {das_total} .das archives ({das_yz2} yz2-compressed): {das_ok} decompress to the header\'s size and parse as '
              f'consistent containers, {das_entries} entries (their FCV / SEQ / BIN / TPL are in the counts above)')
    print(f'  {bin_total} models, {bin_ok} byte-identical round-trips ({shaped} with a shape table, {shape_keys} shape keys); '
          f'{tpl_total} texture palettes, {tex_total} textures decoded, '
          f'{tex_lossless} lossless (I4 / IA8), {tex_ok} byte-identical re-encodings')
    print(f'  character table: {tab_ok} of {tab_total} attachments load from this source')
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
    p.add_argument('--character', help='leon / ada / hunk / krauser / wesker (or plNN): the player archive of the source '
                                       'plus the melee motions the em10 routines play on the player (from em10.drs)')
    p.set_defaults(func=cmd_list)
    p = sub.add_parser('export')
    p.add_argument('source')
    p.add_argument('--archive', help='archive name when the source is a disc (plNN.drs, em10.drs, rNNN.das)')
    p.add_argument('--motion', type=lambda s: int(s, 0))
    p.add_argument('--all', action='store_true')
    p.add_argument('--model', help='model .bin file or [stem:]index (default: the character table\'s body of the '
                                   'motion\'s archive, else its entry 0); "pl00:0" takes the body from another archive')
    p.add_argument('--character', help='leon / ada / hunk / krauser / wesker (or plNN): play the motion on that '
                                       'player\'s body with the character table\'s attachments (= --model plNN:body)')
    p.add_argument('--tpl', type=lambda s: int(s, 0), help='texture palette entry of the body (default: the entry after the model)')
    p.add_argument('--mesh', action='append', metavar='[stem:]BIN[:[stem:]TPL]',
                   help='attachment model entry (head, hair, hands ...) skinned to the body parts, with its TPL entry '
                        '(default: the table\'s palette for a table entry, else BIN + 1); "wep04:6:13" takes the .bin from '
                        'another archive of the source; repeatable')
    p.add_argument('--body-only', action='store_true', help='no attachments from the character table')
    p.add_argument('--no-mesh', action='store_true', help='skeleton only (stick-figure placeholder mesh)')
    p.add_argument('--render-nice', metavar='DIR', help='with the Blender check: EEVEE renders with three area lights, the camera '
                                                       'framed on the animated character (and one per shape key) into DIR')
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
