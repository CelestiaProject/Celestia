#!/usr/bin/env python
#
# Copyright (C) 2023-present, Celestia Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

from enum import Enum
from math import sin, cos, pi

SmallCircleCount = 10
LargeCircleCount = 60

def FillCircleValues(size):
    i = 0
    ary=[]
    while i < size:
        angle = 2.0 * pi * i / size
        ary.append(cos(angle))
        ary.append(sin(angle))
        i+=1
    return ary

def RearrangeForLines(ary, count):
    out = []
    i = 0
    while i < count:
        j = i * 2
        out.append(ary[j])
        out.append(ary[j+1])
        k = ((i + 1) % count) * 2
        out.append(ary[k])
        out.append(ary[k+1])
        i+=1
    return out

### Markers drawn with triangles

class FilledMarkers(Enum):
    FilledSquare = 0
    RightArrow = 1
    LeftArrow = 2
    UpArrow = 3
    DownArrow = 4
    SelPointer = 5
    Crosshair = 6
    Disk = 7
    LargeDisk = 8

filledMarkers = [
    [
        -1.0,     -1.0,
         1.0,     -1.0,
         1.0,      1.0,
        -1.0,      1.0
    ],
    [
        -3.0,  1.0/3.0,
        -3.0, -1.0/3.0,
        -2.0, -1.0/4.0,
        -2.0, -1.0/4.0,
        -2.0,  1.0/4.0,
        -3.0,  1.0/3.0,
        -2.0,  2.0/3.0,
        -2.0, -2.0/3.0,
        -1.0,      0.0
    ],
    [
         3.0, -1.0/3.0,
         3.0,  1.0/3.0,
         2.0,  1.0/4.0,
         2.0,  1.0/4.0,
         2.0, -1.0/4.0,
         3.0, -1.0/3.0,
         2.0, -2.0/3.0,
         2.0,  2.0/3.0,
         1.0,      0.0
    ],
    [
        -1.0/3.0, -3.0,
         1.0/3.0, -3.0,
         1.0/4.0, -2.0,
         1.0/4.0, -2.0,
        -1.0/4.0, -2.0,
        -1.0/3.0, -3.0,
        -2.0/3.0, -2.0,
         2.0/3.0, -2.0,
         0.0,     -1.0
    ],
    [
         1.0/3.0,  3.0,
        -1.0/3.0,  3.0,
        -1.0/4.0,  2.0,
        -1.0/4.0,  2.0,
         1.0/4.0,  2.0,
         1.0/3.0,  3.0,
         2.0/3.0,  2.0,
        -2.0/3.0,  2.0,
         0.0,      1.0
    ],
    [
         0.0,      0.0,
       -20.0,      6.0,
       -20.0,     -6.0
    ],
    [
         0.0,      0.0,
         1.0,     -1.0,
         1.0,      1.0
    ]
]

smallCircle = FillCircleValues(SmallCircleCount)
largeCircle = FillCircleValues(LargeCircleCount)

print("// This is a generated code, don't edit it manually.")
print("// This file is in public domain.")

offsets = []
counts = []
result = []

offset = 0
for m in filledMarkers:
    offsets.append(offset)
    count = len(m) / 2
    counts.append(count)
    offset += count
    result += m

offsets.append(offset)
counts.append(SmallCircleCount)
offset += SmallCircleCount
result += smallCircle

offsets.append(offset)
counts.append(LargeCircleCount)
offset += LargeCircleCount
result += largeCircle

print("\n// Filled Markers\n")
print("constexpr std::array<float, {}> FilledMarkersData = {{".format(len(result)))
i = 0
n = 0
while i < len(result):
    if n < len(offsets) and i / 2 == offsets[n]:
        print("    // {}".format(list(FilledMarkers)[n].name))
        n += 1
    print("    {: f}f, {: f}f{}".format(result[i], result[i+1], ',' if i < len(result) - 2 else ''))
    i += 2
print("};")

print("\nvoid\nRenderFilledMarker(Renderer &r, gl::VertexObject &vo, MarkerRepresentation::Symbol symbol, float size, const Color &color, const Matrices &m)")
print("{")
print("    ShaderProperties shadprop;")
print("    shadprop.texUsage = ShaderProperties::VertexColors;")
print("    shadprop.lightModel = ShaderProperties::UnlitModel;")
print("    shadprop.fishEyeOverride = ShaderProperties::FisheyeOverrideModeDisabled;")
print("    auto* prog = r.getShaderManager().getShader(shadprop);")
print("    if (prog == nullptr) return;")
print("    glVertexAttrib4fv(CelestiaGLProgram::ColorAttributeIndex, color.toVector4().data());")
print("    prog->use();")
print("    prog->setMVPMatrices(*m.projection, *m.modelview);")

print("    switch (symbol)")
print("    {")
for i in FilledMarkers:
    if not i.name in ['Disk', 'LargeDisk', 'SelPointer', 'Crosshair']:
        prim = 'gl::VertexObject::Primitive::TriangleFan' if i.name  == 'FilledSquare' else 'gl::VertexObject::Primitive::Triangles'
        print("    case MarkerRepresentation::{}:".format(i.name))
        print("        vo.draw({}, {}, {});".format(prim, counts[i.value], offsets[i.value]))
        print("        break;")
print("    case MarkerRepresentation::Disk:")
print("        if (size <= 40.0f) // TODO: this should be configurable")
print("            vo.draw(gl::VertexObject::Primitive::TriangleFan, {}, {});".format(counts[FilledMarkers.Disk.value], offsets[FilledMarkers.Disk.value]))
print("        else")
print("            vo.draw(gl::VertexObject::Primitive::TriangleFan, {}, {});".format(counts[FilledMarkers.LargeDisk.value], offsets[FilledMarkers.LargeDisk.value]))
print("        break;")
print("    default:")
print("        break;")
print("    }")
print("}\n")

print("constexpr int SelPointerCount  = {};".format(counts[FilledMarkers.SelPointer.value]))
print("constexpr int SelPointerOffset = {};".format(offsets[FilledMarkers.SelPointer.value]))
print("constexpr int CrosshairCount   = {};".format(counts[FilledMarkers.Crosshair.value]))
print("constexpr int CrosshairOffset  = {};".format(offsets[FilledMarkers.Crosshair.value]))

### Markers drawn with lines

class HollowMarkers(Enum):
    Square = 0
    Triangle = 1
    Diamond = 2
    Plus = 3
    X = 4
    Circle = 5
    LargeCircle = 6

hollowMarkers = [
    [
        -1.0, -1.0,  1.0, -1.0,
         1.0, -1.0,  1.0,  1.0,
         1.0,  1.0, -1.0,  1.0,
        -1.0,  1.0, -1.0, -1.0
    ],
    [
        0.0,  1.0,  1.0, -1.0,
        1.0, -1.0, -1.0, -1.0,
       -1.0, -1.0,  0.0,  1.0
    ],
    [
         0.0,  1.0,  1.0,  0.0,
         1.0,  0.0,  0.0, -1.0,
         0.0, -1.0, -1.0,  0.0,
        -1.0,  0.0,  0.0,  1.0
    ],
    [
         0.0,  1.0,  0.0, -1.0,
         1.0,  0.0, -1.0,  0.0
    ],
    [
        -1.0, -1.0,  1.0,  1.0,
         1.0, -1.0, -1.0,  1.0
    ]
]

offsets = []
counts = []
result = []

offset = 0
for m in hollowMarkers:
    offsets.append(offset)
    count = len(m) / 2
    counts.append(count)
    offset += count
    result += m

smallCircle = RearrangeForLines(smallCircle, SmallCircleCount)
largeCircle = RearrangeForLines(largeCircle, LargeCircleCount)

offsets.append(offset)
counts.append(SmallCircleCount*2) # Number of vertices = number of lines * 2
offset += SmallCircleCount*2
result += smallCircle

offsets.append(offset)
counts.append(LargeCircleCount*2)
offset += LargeCircleCount*2
result += largeCircle

print("\n// Hollow Markers\n")
print("constexpr std::array<float, {}> HollowMarkersData = {{".format(len(result)))
i = 0
n = 0
while i < len(result):
    if n < len(offsets) and i / 2 == offsets[n]:
        print("    // {}".format(list(HollowMarkers)[n].name))
        n += 1
    print("    {: f}f, {: f}f{}".format(result[i], result[i+1], ',' if i < len(result) - 2 else ''))
    i += 2
print("};")

print("\nvoid\nRenderHollowMarker(LineRenderer &lr, MarkerRepresentation::Symbol symbol, float size, const Color &color, const Matrices &m)")
print("{")
print("    lr.prerender();")
print("    switch (symbol)")
print("    {")
for i in HollowMarkers:
    if i.name != 'Circle' and i.name != 'LargeCircle':
        print("    case MarkerRepresentation::{}:".format(i.name))
        print("        lr.render(m, color, {}, {});".format(counts[i.value], offsets[i.value]))
        print("        break;")
print("    case MarkerRepresentation::Circle:")
print("        if (size <= 40.0f) // TODO: this should be configurable")
print("            lr.render(m, color, {}, {});".format(counts[HollowMarkers.Circle.value], offsets[HollowMarkers.Circle.value]))
print("        else")
print("            lr.render(m, color, {}, {});".format(counts[HollowMarkers.LargeCircle.value], offsets[HollowMarkers.LargeCircle.value]))
print("        break;")
print("    default:")
print("        break;")
print("    }")
print("    lr.finish();")
print("}")
