"""Headless Blender check of an exported glTF (run by motion_export.py export --blender-check):

    blender -b --python tools/motion/blender_check.py -- <file.gltf> <ref.json> [<render dir>]

Imports the file, asserts the armature has the bone count and the action the frame range the
reference describes, that the mesh objects hold the exporter's vertex / triangle counts (ref
"mesh", every mesh skinned to the armature), samples the world positions of the reference bones
at its frames and compares them with the helper's parts world positions (printed, max abs error),
and renders 4 frames (workbench; textured when there is a mesh, solid for the stick figure) to
PNGs in the render directory. ref "strip": also every frame at 30 fps to <name>_strip.png (one
row of 256 px frames) and <name>.mp4.
"""
import json
import math
import os
import subprocess
import sys

import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index('--') + 1:]
gltf_path, ref_path = argv[0], argv[1]
render_dir = argv[2] if len(argv) > 2 else None
ref = json.load(open(ref_path))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.render.fps = int(round(ref['fps']))   # the importer maps seconds to frames with it
bpy.ops.import_scene.gltf(filepath=gltf_path)

armatures = [o for o in bpy.data.objects if o.type == 'ARMATURE']
assert len(armatures) == 1, f'expected one armature, found {len(armatures)}'
arm = armatures[0]
n_bones = len(arm.data.bones)
assert n_bones == ref['bones'], f'armature has {n_bones} bones, expected {ref["bones"]}'

# the importer's bone display shape (bone_heuristic BLENDER: an "Icosphere" object) is not model geometry
shapes = {pb.custom_shape for pb in arm.pose.bones if pb.custom_shape}
meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH' and o not in shapes]
if ref.get('mesh'):
    exp = ref['mesh']
    assert len(meshes) == exp['meshes'], f'{len(meshes)} mesh objects, expected {exp["meshes"]}'
    n_v = sum(len(o.data.vertices) for o in meshes)
    n_t = sum(len(o.data.polygons) for o in meshes)
    assert all(len(p.vertices) == 3 for o in meshes for p in o.data.polygons), 'non-triangle faces'
    assert n_v == exp['vertices'], f'{n_v} vertices imported, exporter wrote {exp["vertices"]}'
    assert n_t == exp['triangles'], f'{n_t} triangles imported, exporter wrote {exp["triangles"]}'
    for o in meshes:
        mods = [m for m in o.modifiers if m.type == 'ARMATURE']
        assert mods and mods[0].object == arm, f'{o.name}: not skinned to the armature'
        assert o.data.uv_layers and o.data.color_attributes, f'{o.name}: missing UVs or vertex colours'
        assert all(s.material and s.material.use_nodes for s in o.material_slots), f'{o.name}: material without nodes'
    n_img = len([i for i in bpy.data.images if i.filepath])
    assert n_img == exp['materials'], f'{n_img} images, exporter wrote {exp["materials"]} textures'
    print(f'blender: mesh: {len(meshes)} object(s), {n_v} vertices, {n_t} triangles, {n_img} textures, all skinned to {arm.name}')

actions = list(bpy.data.actions)
assert actions, 'no action imported'
scene = bpy.context.scene
first = min(a.frame_range[0] for a in actions)
last = max(a.frame_range[1] for a in actions)
exp_last = ref['frames'] - 1
assert abs(first) < 1e-3 and abs(last - exp_last) < 1e-3, f'action frame range {first}..{last}, expected 0..{exp_last}'
scene.frame_start = 0
scene.frame_end = ref['frames'] - 1
print(f'blender: armature {arm.name}: {n_bones} bones, action range {first:g}..{last:g}, {len(actions)} action(s)')

# world positions of the reference bones (bone heads = parts origins) at the reference frames
max_err = 0.0
for frame, bones in ref['samples'].items():
    scene.frame_set(int(frame))
    bpy.context.view_layer.update()
    for name, expect in bones.items():
        pb = arm.pose.bones[name]
        head = arm.matrix_world @ pb.head
        err = max(abs(head[i] - expect[i]) for i in range(3))
        max_err = max(max_err, err)
        print(f'blender: frame {frame} {name}: blender ({head.x:.5f}, {head.y:.5f}, {head.z:.5f})'
              f' helper ({expect[0]:.5f}, {expect[1]:.5f}, {expect[2]:.5f}) err {err:.2e}')
print(f'blender: max abs error of sampled bone positions vs helper: {max_err:.3e} (units: {ref["units"]})')
assert max_err < ref['tolerance'], f'bone positions differ from the helper by {max_err}'

if ref.get('mesh'):
    # the deformed mesh must stay near the bones at every sampled frame: an exploded vertex or a
    # stretched limb (wrong weights / bind matrices) reaches far outside the skeleton's box
    worst = 0.0
    for frame in ref['samples']:
        scene.frame_set(int(frame))
        bpy.context.view_layer.update()
        heads = [arm.matrix_world @ pb.head for pb in arm.pose.bones]
        lo = Vector((min(p.x for p in heads), min(p.y for p in heads), min(p.z for p in heads)))
        hi = Vector((max(p.x for p in heads), max(p.y for p in heads), max(p.z for p in heads)))
        margin = 0.25 * max(hi - lo)
        dg = bpy.context.evaluated_depsgraph_get()
        for o in meshes:
            ev = o.evaluated_get(dg)
            for v in ev.data.vertices:
                p = ev.matrix_world @ v.co
                out = max(max(lo[i] - margin - p[i], p[i] - hi[i] - margin, 0.0) for i in range(3))
                worst = max(worst, out)
    print(f'blender: deformed mesh outside the bones\' box + 25%: {worst:.4f} (0 = inside at every sampled frame)')
    assert worst == 0.0, f'mesh vertices reach {worst} outside the skeleton box: exploded / stretched skinning'

if render_dir:
    os.makedirs(render_dir, exist_ok=True)
    # a camera in front of the model, framing the bounding box of the motion (bone heads at the
    # sampled frames; the mesh extends about a head beyond the bones)
    pts = []
    for frame in ref['samples']:
        scene.frame_set(int(frame))
        bpy.context.view_layer.update()
        pts += [arm.matrix_world @ pb.head for pb in arm.pose.bones]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    center = (lo + hi) / 2
    size = max(hi - lo) * 1.15 or 1.0
    cam_data = bpy.data.cameras.new('cam')
    cam = bpy.data.objects.new('cam', cam_data)
    scene.collection.objects.link(cam)
    cam.location = center + Vector((0.0, -size * 1.8, size * 0.1))
    direction = center - cam.location
    cam.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
    scene.camera = cam
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'TEXTURE' if ref.get('mesh') else 'SINGLE'
    scene.display.shading.show_object_outline = not ref.get('mesh')
    scene.render.resolution_x = 640
    scene.render.resolution_y = 640
    scene.render.image_settings.file_format = 'PNG'
    for o in bpy.data.objects:
        if o.type == 'ARMATURE':
            o.show_in_front = True
            o.data.display_type = 'OCTAHEDRAL'
            o.hide_render = bool(ref.get('mesh'))
    n = ref['frames']
    for f in sorted(set([0, n // 3, (2 * n) // 3, n - 1])):
        scene.frame_set(f)
        scene.render.filepath = os.path.join(render_dir, f'{ref["name"]}_f{f:03d}.png')
        bpy.ops.render.render(write_still=True)
        print(f'blender: rendered {scene.render.filepath}')
    if ref.get('strip'):
        # every frame at 256 px: an mp4 at the motion's fps and one long row (the film strip)
        frame_dir = os.path.join(render_dir, f'{ref["name"]}_frames')
        os.makedirs(frame_dir, exist_ok=True)
        scene.render.resolution_x = scene.render.resolution_y = 256
        scene.frame_start = 0
        scene.frame_end = n - 1
        scene.render.filepath = os.path.join(frame_dir, 'f')
        bpy.ops.render.render(animation=True)
        frames = [os.path.join(frame_dir, f'f{f:04d}.png') for f in range(n)]
        mp4 = os.path.join(render_dir, f'{ref["name"]}.mp4')
        subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', str(int(round(ref['fps']))), '-i',
                        os.path.join(frame_dir, 'f%04d.png'), '-pix_fmt', 'yuv420p', mp4], check=True)
        strip = os.path.join(render_dir, f'{ref["name"]}_strip.png')
        subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', os.path.join(frame_dir, 'f%04d.png'),
                        '-filter_complex', f'tile={n}x1', '-frames:v', '1', strip], check=True)
        print(f'blender: strip {strip} ({n} frames), video {mp4}')
print('blender: OK')
