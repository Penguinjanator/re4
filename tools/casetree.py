#!/usr/bin/env python3
"""Model of GCC 2.95 stmt.c case-tree emission (group/balance/emit_case_nodes) for an int index.
cases: list of (value, label). Emits a list of (value, cond, target) compares in order.
"""
import itertools, sys

INT_MIN, INT_MAX = -2**31, 2**31 - 1


class Node:
    def __init__(self, lo, hi, label):
        self.low, self.high, self.label = lo, hi, label
        self.left = self.right = self.parent = None


def build(cases):
    cases = sorted(cases)
    nodes = [Node(v, v, l) for v, l in cases]
    # group
    out = []
    i = 0
    while i < len(nodes):
        n = nodes[i]
        j = i + 1
        while j < len(nodes) and nodes[j].label == n.label and nodes[j].low == n.high + 1:
            n.high = nodes[j].high
            j += 1
        out.append(n)
        i = j
    return out


def balance(lst, parent):
    """lst: python list of nodes (the 'right' chain). Returns root."""
    if not lst:
        return None
    n = len(lst)
    ranges = sum(1 for x in lst if x.low != x.high)
    if n > 2:
        if n == 3:
            k = 1
        else:
            i = (n + ranges + 1) // 2
            k = 0
            while True:
                if lst[k].low != lst[k].high:
                    i -= 1
                i -= 1
                if i <= 0:
                    break
                k += 1
        root = lst[k]
        root.parent = parent
        root.left = balance(lst[:k], root)
        root.right = balance(lst[k + 1:], root)
        return root
    else:
        root = lst[0]
        root.parent = parent
        cur = root
        for x in lst[1:]:
            cur.right = x
            x.parent = cur
            cur = x
        return root


def has_low(node):
    if node.low == INT_MIN:
        return True
    if node.left:
        return False
    p = node.parent
    while p:
        if p.high == node.low - 1:
            return True
        p = p.parent
    return False


def has_high(node):
    if node.high == INT_MAX:
        return True
    if node.right:
        return False
    p = node.parent
    while p:
        if p.low == node.high + 1:
            return True
        p = p.parent
    return False


def bounded(node):
    return has_low(node) and has_high(node)


def emit(node, out):
    D = 'default'
    if bounded(node):
        out.append(('jmp', node.label))
    elif node.low == node.high:
        out.append((node.low, 'EQ', node.label))
        if node.right and node.left:
            if bounded(node.right):
                out.append((node.high, 'GT', node.right.label))
                emit(node.left, out)
            elif bounded(node.left):
                out.append((node.high, 'LT', node.left.label))
                emit(node.right, out)
            else:
                out.append((node.high, 'GT', 'test'))
                emit(node.left, out)
                out.append(('jmp', D))
                emit(node.right, out)
        elif node.right:
            r = node.right
            if r.right or r.left or r.low != r.high:
                if not has_low(node):
                    out.append((node.high, 'LT', D))
                emit(r, out)
            else:
                out.append((r.low, 'EQ', r.label))
        elif node.left:
            l = node.left
            if l.left or l.right or l.low != l.high:
                if not has_high(node):
                    out.append((node.high, 'GT', D))
                emit(l, out)
            else:
                out.append((l.low, 'EQ', l.label))
    else:
        if node.right and node.left:
            if bounded(node.right):
                out.append((node.high, 'GT', node.right.label))
                test = False
            else:
                out.append((node.high, 'GT', 'test'))
                test = True
            out.append((node.low, 'GE', node.label))
            emit(node.left, out)
            if test:
                out.append(('jmp', D))
                emit(node.right, out)
        elif node.right:
            if not has_low(node):
                out.append((node.low, 'LT', D))
            out.append((node.high, 'LE', node.label))
            emit(node.right, out)
        elif node.left:
            if not has_high(node):
                out.append((node.high, 'GT', D))
            out.append((node.low, 'GE', node.label))
            emit(node.left, out)
        else:
            if not has_high(node):
                out.append((node.high, 'GT', D))
            if not has_low(node):
                out.append((node.low, 'LT', D))
            out.append(('jmp', node.label))


def tree(cases):
    lst = build(cases)
    root = balance(lst, None)
    out = []
    emit(root, out)
    return out


def compares(cases):
    return [(v, c) for v, c, *_ in tree(cases) if v != 'jmp']


if __name__ == '__main__':
    # target compare sequence for em28DmCk (surviving compares)
    target = [(0x11, 'GT'), (0x10, 'GE'), (0xA, 'GT'), (9, 'GE'), (6, 'GT'), (5, 'GE'), (0, 'LT'), (0xD, 'EQ'), (0xD, 'LT'),
              (0xE, 'EQ'), (0x21, 'EQ'), (0x21, 'GT'), (0x15, 'GT'), (0x14, 'GE'), (0x1B, 'EQ'), (0x1D, 'EQ'), (0x29, 'LE'),
              (0x2B, 'EQ'), (0x28, 'GE'), (0x26, 'EQ')]

    def subseq(t, s):
        it = iter(s)
        return all(x in it for x in t)

    fixed = {7: 'X', 8: 'X', 0x21: 'X', 0xE: 'Y'}
    # candidate values that may be explicit cases, with label A/B/C or absent
    cand = [0, 1, 2, 3, 4, 5, 6, 9, 0xA, 0xB, 0xC, 0xD, 0xF, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x1B, 0x1D, 0x26, 0x27,
            0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D]
    # group candidates into blocks that must share a label (from the target's ranges)
    blocks = [[0, 1, 2, 3, 4], [5, 6], [9, 0xA], [0xB, 0xC], [0xD], [0xF], [0x10, 0x11], [0x12, 0x13], [0x14, 0x15],
              [0x1B], [0x1D], [0x26], [0x27], [0x28, 0x29], [0x2A], [0x2B], [0x2C], [0x2D]]
    labels = ['A', 'B', 'C', None]
    best = []
    for assign in itertools.product(labels, repeat=len(blocks)):
        cases = [(v, l) for v, l in fixed.items()]
        for blk, lab in zip(blocks, assign):
            if lab:
                cases += [(v, lab) for v in blk]
        cs = compares(cases)
        if subseq(target, cs):
            extra = len(cs) - len(target)
            best.append((extra, assign))
    best.sort()
    for extra, assign in best[:40]:
        print(extra, {tuple(b): l for b, l in zip(blocks, assign) if l})
    print(len(best), 'candidates')
