"""glTF 2.0 writer for a posed parts hierarchy.

Scene: node "model" (the cModel: root motion) with the parts as a node tree (parent links and
rest translations from the .bin), a skin whose joints are the parts nodes (inverse bind
matrices from the rest pose) and a stick-figure mesh bound to it (one thin triangle per parts
towards its parent) so that importers create an armature; one animation with translation /
rotation / scale samplers per parts node and per the model node, sampled at every motion frame
(frame / fps seconds, LINEAR: exact at the frame times, which is what the game evaluates), values
straight from the game's parts matrices. Units: the game's millimetres times `scale`.
"""
import base64
import json
import math
import struct

FLOAT = 5126
U16 = 5123
U8 = 5121
ARRAY_BUFFER = 34962
ELEMENT_ARRAY_BUFFER = 34963


def normalize(v):
    n = math.sqrt(sum(c * c for c in v))
    return [c / n for c in v] if n else list(v)


def decompose(l_mat):
    """3x4 row-major (game Mtx) -> translation, columns' lengths (scale), rotation 3x4 with unit columns."""
    m = l_mat
    t = [m[3], m[7], m[11]]
    cols = [[m[0], m[4], m[8]], [m[1], m[5], m[9]], [m[2], m[6], m[10]]]
    sc = [math.sqrt(sum(c * c for c in col)) for col in cols]
    r = [0.0] * 12
    for j in range(3):
        col = normalize(cols[j])
        r[j] = col[0]
        r[4 + j] = col[1]
        r[8 + j] = col[2]
    return t, sc, r


def quat_normalize(q):
    n = math.sqrt(sum(c * c for c in q))
    return [c / n for c in q]


def quat_continuous(prev, q):
    """Flip q to the hemisphere of prev (same rotation, shortest interpolation)."""
    if prev is not None and sum(a * b for a, b in zip(prev, q)) < 0:
        return [-c for c in q]
    return q


def invert_affine(m):
    """Inverse of a 3x4 row-major rigid transform with unit rotation columns (rest pose)."""
    r = [[m[0], m[1], m[2]], [m[4], m[5], m[6]], [m[8], m[9], m[10]]]
    t = [m[3], m[7], m[11]]
    rt = [[r[j][i] for j in range(3)] for i in range(3)]
    it = [-sum(rt[i][j] * t[j] for j in range(3)) for i in range(3)]
    return [rt[0][0], rt[0][1], rt[0][2], it[0], rt[1][0], rt[1][1], rt[1][2], it[1], rt[2][0], rt[2][1], rt[2][2], it[2]]


def col_major_4x4(m34, scale):
    """glTF column-major 4x4 from a 3x4 row-major game matrix (translation scaled)."""
    out = []
    for c in range(4):
        for r in range(3):
            v = m34[4 * r + c]
            out.append(v * scale if c == 3 else v)
        out.append(1.0 if c == 3 else 0.0)
    return out


class Builder:
    def __init__(self):
        self.blob = bytearray()
        self.buffer_views = []
        self.accessors = []

    def accessor(self, fmt, comp_type, type_name, values, target=None, minmax=False):
        st = struct.Struct('<' + fmt)
        n = len(values)
        while len(self.blob) % 4:
            self.blob += b'\0'
        off = len(self.blob)
        for v in values:
            self.blob += st.pack(*v) if isinstance(v, (tuple, list)) else st.pack(v)
        bv = {'buffer': 0, 'byteOffset': off, 'byteLength': len(self.blob) - off}
        if target:
            bv['target'] = target
        self.buffer_views.append(bv)
        acc = {'bufferView': len(self.buffer_views) - 1, 'componentType': comp_type, 'count': n, 'type': type_name}
        if minmax:
            if isinstance(values[0], (tuple, list)):
                acc['min'] = [min(v[i] for v in values) for i in range(len(values[0]))]
                acc['max'] = [max(v[i] for v in values) for i in range(len(values[0]))]
            else:
                acc['min'] = [min(values)]
                acc['max'] = [max(values)]
        self.accessors.append(acc)
        return len(self.accessors) - 1


def export(model, motion, poses, out_path, name, fps=30.0, scale=0.001, root_motion=True, extras=None, stick=True):
    """poses: evalhost.Pose per frame 0..maxFrame (the game's parts matrices)."""
    n = model.n_parts
    b = Builder()
    nodes = []
    # node 0: the model, nodes 1..n: parts
    model_node = {'name': name and f'{name}_model' or 'model', 'children': []}
    nodes.append(model_node)
    for p in model.parts:
        nodes.append({'name': f'parts_{p.no:03d}', 'translation': [c * scale for c in p.pos], 'children': []})
    for p in model.parts:
        parent = 0 if p.parent < 0 else p.parent + 1
        nodes[parent]['children'].append(p.no + 1)
    for nd in nodes:
        if not nd['children']:
            del nd['children']

    # rest pose world matrices (model at the origin) for the inverse bind matrices
    rest_world = [None] * n
    for p in model.parts:
        base = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0] if p.parent < 0 else rest_world[p.parent]
        m = list(base)
        m[3] += p.pos[0]
        m[7] += p.pos[1]
        m[11] += p.pos[2]
        rest_world[p.no] = m
    ibm = [col_major_4x4(invert_affine(m), scale) for m in rest_world]
    ibm_acc = b.accessor('16f', FLOAT, 'MAT4', ibm)
    skin = {'joints': list(range(1, n + 1)), 'inverseBindMatrices': ibm_acc, 'skeleton': 0, 'name': 'skeleton'}

    meshes = []
    mesh_node = None
    if stick:
        # one thin triangle per parts from its rest position to its parent's, skinned to the parts
        pos = []
        joints = []
        weights = []
        idx = []
        for p in model.parts:
            a = [c * scale for c in rest_world[p.no][3::4]]
            pp = [0.0, 0.0, 0.0] if p.parent < 0 else [c * scale for c in rest_world[p.parent][3::4]]
            d = [pp[i] - a[i] for i in range(3)]
            length = math.sqrt(sum(c * c for c in d)) or 0.02 * scale / 0.001
            w = 0.006 * length + 0.002 * scale / 0.001
            side = normalize([-d[1], d[0], 0.0] if abs(d[2]) < 0.9 * length else [0.0, -d[2], d[1]])
            j = p.no
            pj = j if p.parent < 0 else p.parent
            base = len(pos)
            pos += [(a[0] + side[0] * w, a[1] + side[1] * w, a[2] + side[2] * w),
                    (a[0] - side[0] * w, a[1] - side[1] * w, a[2] - side[2] * w),
                    tuple(pp)]
            joints += [(j, 0, 0, 0), (j, 0, 0, 0), (pj, 0, 0, 0)]
            weights += [(1.0, 0.0, 0.0, 0.0)] * 3
            idx += [base, base + 1, base + 2, base, base + 2, base + 1]
        prim = {
            'attributes': {
                'POSITION': b.accessor('3f', FLOAT, 'VEC3', pos, ARRAY_BUFFER, minmax=True),
                'JOINTS_0': b.accessor('4H', U16, 'VEC4', joints, ARRAY_BUFFER),
                'WEIGHTS_0': b.accessor('4f', FLOAT, 'VEC4', weights, ARRAY_BUFFER),
            },
            'indices': b.accessor('H', U16, 'SCALAR', idx, ELEMENT_ARRAY_BUFFER),
            'mode': 4,
        }
        meshes.append({'name': 'skeleton_sticks', 'primitives': [prim]})
        mesh_node = len(nodes)
        nodes.append({'name': 'skeleton_mesh', 'mesh': 0, 'skin': 0})

    # animation
    times = [f / fps for f in range(len(poses))]
    time_acc = b.accessor('f', FLOAT, 'SCALAR', times, minmax=True)
    samplers = []
    channels = []

    def add_channel(node, path, values, fmt, typ):
        acc = b.accessor(fmt, FLOAT, typ, values)
        samplers.append({'input': time_acc, 'output': acc, 'interpolation': 'LINEAR'})
        channels.append({'sampler': len(samplers) - 1, 'target': {'node': node, 'path': path}})

    for i in range(n):
        tr = []
        rot = []
        sc = []
        prev = None
        for pose in poses:
            t, s, r = decompose(pose.l_mat[i])
            q = quat_continuous(prev, quat_normalize(list(pose.quat(r))))
            prev = q
            tr.append(tuple(c * scale for c in t))
            rot.append(tuple(q))
            sc.append(tuple(s))
        add_channel(i + 1, 'translation', tr, '3f', 'VEC3')
        add_channel(i + 1, 'rotation', rot, '4f', 'VEC4')
        if any(abs(c - 1.0) > 1e-6 for s in sc for c in s):
            add_channel(i + 1, 'scale', sc, '3f', 'VEC3')
    if root_motion and (poses[0].root_pos is not None or poses[0].root_rot is not None):
        tr = [tuple(c * scale for c in pose.root_pos) for pose in poses]
        prev = None
        rot = []
        for pose in poses:
            q = quat_continuous(prev, quat_normalize(list(pose.quat(pose.root_mat))))
            prev = q
            rot.append(tuple(q))
        add_channel(0, 'translation', tr, '3f', 'VEC3')
        add_channel(0, 'rotation', rot, '4f', 'VEC4')

    anim = {'name': name or 'motion', 'samplers': samplers, 'channels': channels}
    doc = {
        'asset': {'version': '2.0', 'generator': 're4 tools/motion_export.py'},
        'scene': 0,
        'scenes': [{'nodes': [0] + ([mesh_node] if mesh_node is not None else [])}],
        'nodes': nodes,
        'skins': [skin],
        'animations': [anim],
        'accessors': b.accessors,
        'bufferViews': b.buffer_views,
    }
    if meshes:
        doc['meshes'] = meshes
    if extras:
        doc['extras'] = extras
    write(doc, bytes(b.blob), out_path)
    return doc


def write(doc, blob, out_path):
    while len(blob) % 4:
        blob += b'\0'
    if out_path.lower().endswith('.glb'):
        doc['buffers'] = [{'byteLength': len(blob)}]
        js = json.dumps(doc, separators=(',', ':')).encode()
        while len(js) % 4:
            js += b' '
        total = 12 + 8 + len(js) + 8 + len(blob)
        with open(out_path, 'wb') as f:
            f.write(struct.pack('<III', 0x46546C67, 2, total))
            f.write(struct.pack('<II', len(js), 0x4E4F534A) + js)
            f.write(struct.pack('<II', len(blob), 0x004E4942) + blob)
    else:
        doc['buffers'] = [{'byteLength': len(blob), 'uri': 'data:application/octet-stream;base64,' + base64.b64encode(blob).decode()}]
        with open(out_path, 'w') as f:
            json.dump(doc, f, separators=(',', ':'))
