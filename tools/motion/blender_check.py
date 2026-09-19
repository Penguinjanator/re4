"""Headless Blender check of an exported glTF (run by motion_export.py export --blender-check):

    blender -b --python tools/motion/blender_check.py -- <file.gltf> <ref.json> [<render dir>]

Imports the file, asserts the armature has the bone count and the action the frame range the
reference describes, samples the world positions of the reference bones at its frames and
compares them with the helper's parts world positions (printed, max abs error), and renders
4 frames (solid shading, workbench) to PNGs in the render directory.
"""
import json
import math
import os
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

if render_dir:
    os.makedirs(render_dir, exist_ok=True)
    # a camera in front of the model, framing the skeleton's bounding box
    scene.frame_set(0)
    bpy.context.view_layer.update()
    pts = [arm.matrix_world @ pb.head for pb in arm.pose.bones]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    center = (lo + hi) / 2
    size = max(hi - lo) or 1.0
    cam_data = bpy.data.cameras.new('cam')
    cam = bpy.data.objects.new('cam', cam_data)
    scene.collection.objects.link(cam)
    cam.location = center + Vector((0.0, -size * 2.2, size * 0.15))
    direction = center - cam.location
    cam.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
    scene.camera = cam
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'SINGLE'
    scene.display.shading.show_object_outline = True
    scene.render.resolution_x = 640
    scene.render.resolution_y = 640
    scene.render.image_settings.file_format = 'PNG'
    for o in bpy.data.objects:
        if o.type == 'ARMATURE':
            o.show_in_front = True
            o.data.display_type = 'OCTAHEDRAL'
    n = ref['frames']
    for f in sorted(set([0, n // 3, (2 * n) // 3, n - 1])):
        scene.frame_set(f)
        scene.render.filepath = os.path.join(render_dir, f'{ref["name"]}_f{f:03d}.png')
        bpy.ops.render.render(write_still=True)
        print(f'blender: rendered {scene.render.filepath}')
print('blender: OK')
