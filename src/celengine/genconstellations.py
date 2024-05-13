#!/usr/bin/env python
#
# Copyright (C) 2023-present, Celestia Development Team
#
# This generator incorporates elements of the original constellations.cpp
# header file Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

"""Creates a prefix tree for constellations"""

from __future__ import annotations

from collections import deque
from typing import Optional


CONSTELLATIONS = [
    ("Aries", "Arietis", "Ari"),
    ("Taurus", "Tauri", "Tau"),
    ("Gemini", "Geminorum", "Gem"),
    ("Cancer", "Cancri", "Cnc"),
    ("Leo", "Leonis", "Leo"),
    ("Virgo", "Virginis", "Vir"),
    ("Libra", "Librae", "Lib"),
    ("Scorpius", "Scorpii", "Sco"),
    ("Sagittarius", "Sagittarii", "Sgr"),
    ("Capricornus", "Capricorni", "Cap"),
    ("Aquarius", "Aquarii", "Aqr"),
    ("Pisces", "Piscium", "Psc"),
    ("Ursa Major", "Ursae Majoris", "UMa"),
    ("Ursa Minor", "Ursae Minoris", "UMi"),
    ("Bootes", "Bootis", "Boo"),
    ("Orion", "Orionis", "Ori"),
    ("Canis Major", "Canis Majoris", "CMa"),
    ("Canis Minor", "Canis Minoris", "CMi"),
    ("Lepus", "Leporis", "Lep"),
    ("Perseus", "Persei", "Per"),
    ("Andromeda", "Andromedae", "And"),
    ("Cassiopeia", "Cassiopeiae", "Cas"),
    ("Cepheus", "Cephei", "Cep"),
    ("Cetus", "Ceti", "Cet"),
    ("Pegasus", "Pegasi", "Peg"),
    ("Carina", "Carinae", "Car"),
    ("Puppis", "Puppis", "Pup"),
    ("Vela", "Velorum", "Vel"),
    ("Hercules", "Herculis", "Her"),
    ("Hydra", "Hydrae", "Hya"),
    ("Centaurus", "Centauri", "Cen"),
    ("Lupus", "Lupi", "Lup"),
    ("Ara", "Arae", "Ara"),
    ("Ophiuchus", "Ophiuchi", "Oph"),
    ("Serpens", "Serpentis", "Ser"),
    ("Aquila", "Aquilae", "Aql"),
    ("Auriga", "Aurigae", "Aur"),
    ("Corona Australis", "Coronae Australis", "CrA"),
    ("Corona Borealis", "Coronae Borealis", "CrB"),
    ("Corvus", "Corvi", "Crv"),
    ("Crater", "Crateris", "Crt"),
    ("Cygnus", "Cygni", "Cyg"),
    ("Delphinus", "Delphini", "Del"),
    ("Draco", "Draconis", "Dra"),
    ("Equuleus", "Equulei", "Equ"),
    ("Eridanus", "Eridani", "Eri"),
    ("Lyra", "Lyrae", "Lyr"),
    ("Piscis Austrinus", "Piscis Austrini", "PsA"),
    ("Sagitta", "Sagittae", "Sge"),
    ("Triangulum", "Trianguli", "Tri"),
    ("Antlia", "Antliae", "Ant"),
    ("Apus", "Apodis", "Aps"),
    ("Caelum", "Caeli", "Cae"),
    ("Camelopardalis", "Camelopardalis", "Cam"),
    ("Canes Venatici", "Canum Venaticorum", "CVn"),
    ("Chamaeleon", "Chamaeleontis", "Cha"),
    ("Circinus", "Circini", "Cir"),
    ("Columba", "Columbae", "Col"),
    ("Coma Berenices", "Comae Berenices", "Com"),
    ("Crux", "Crucis", "Cru"),
    ("Dorado", "Doradus", "Dor"),
    ("Fornax", "Fornacis", "For"),
    ("Grus", "Gruis", "Gru"),
    ("Horologium", "Horologii", "Hor"),
    ("Hydrus", "Hydri", "Hyi"),
    ("Indus", "Indi", "Ind"),
    ("Lacerta", "Lacertae", "Lac"),
    ("Leo Minor", "Leonis Minoris", "LMi"),
    ("Lynx", "Lyncis", "Lyn"),
    ("Microscopium", "Microscopii", "Mic"),
    ("Monoceros", "Monocerotis", "Mon"),
    ("Mensa", "Mensae", "Men"),
    ("Musca", "Muscae", "Mus"),
    ("Norma", "Normae", "Nor"),
    ("Octans", "Octantis", "Oct"),
    ("Pavo", "Pavonis", "Pav"),
    ("Phoenix", "Phoenicis", "Phe"),
    ("Pictor", "Pictoris", "Pic"),
    ("Pyxis", "Pyxidis", "Pyx"),
    ("Reticulum", "Reticuli", "Ret"),
    ("Sculptor", "Sculptoris", "Scl"),
    ("Scutum", "Scuti", "Sct"),
    ("Sextans", "Sextantis", "Sex"),
    ("Telescopium", "Telescopii", "Tel"),
    ("Triangulum Australe", "Trianguli Australis", "TrA"),
    ("Tucana", "Tucanae", "Tuc"),
    ("Volans", "Volantis", "Vol"),
    ("Vulpecula", "Vulpeculae", "Vul"),
]

CONSTELLATION_ABBREVS = sorted(c[2] for c in CONSTELLATIONS)


class Node:
    """A node in the radix tree"""

    edge: str
    value: Optional[str]
    children: list[Node]

    def __init__(self, edge: str, value: str = None) -> None:
        self.edge = edge
        self.value = value
        self.children = []

    def add(self, key: str, value: str) -> None:
        """Adds a key-value pair to the radix tree"""
        node = self
        while True:
            next_node = next(
                (c for c in node.children if c.edge.startswith(key[0])), None
            )
            if next_node is None:
                node.children.append(Node(key, value))
                return
            if key == next_node.edge:
                if next_node.value is None:
                    next_node.value = value
                elif next_node.value != value:
                    raise RuntimeError("Duplicate key detected")
                return
            if key.startswith(next_node.edge):
                key = key[len(next_node.edge) :]
                node = next_node
                continue
            if next_node.edge.startswith(key):
                split_existing = Node(next_node.edge[len(key) :], next_node.value)
                split_existing.children = next_node.children
                next_node.edge = key
                next_node.value = value
                next_node.children = [split_existing]
                return
            pos = 1
            while next_node.edge[pos] == key[pos]:
                pos += 1
            split_existing = Node(next_node.edge[pos:], next_node.value)
            split_existing.children = next_node.children
            new_node = Node(key[pos:], value)
            next_node.edge = next_node.edge[:pos]
            next_node.value = None
            next_node.children = [split_existing, new_node]
            return

    def sort_edges(self) -> None:
        """Sorts the node edges"""
        nodes = [self]
        while nodes:
            node = nodes.pop()
            node.children.sort(key=lambda c: c.edge)
            nodes += node.children


def create_tree() -> Node:
    """Creates the radix tree"""
    root = Node("")
    for name, genitive, abbrev in CONSTELLATIONS:
        root.add(name.lower(), abbrev)
        root.add(genitive.lower(), abbrev)
        root.add(abbrev.lower(), abbrev)

    root.sort_edges()
    return root


def check_overlap(lhs: str, rhs: str) -> tuple[int, str]:
    """Find maximum overlap between lhs and rhs"""
    min_length = min(len(lhs), len(rhs))
    overlap = min_length
    while overlap > 0:
        if lhs.endswith(rhs[:overlap]):
            combined = lhs + rhs[overlap:]
            break
        if rhs.endswith(lhs[:overlap]):
            combined = rhs + lhs[overlap:]
            break
        overlap -= 1
    else:
        combined = lhs + rhs
    return overlap, combined


def create_edge_str(root: Node) -> str:
    """Compresses the edge data"""
    nodes = [root]
    edge_data: list[str] = []
    node_count = 0
    while nodes:
        node = nodes.pop()
        node_count += 1
        nodes += node.children
        if not node.edge:
            continue
        for i, edge in enumerate(edge_data):
            if node.edge in edge:
                # no need to add this edge, as it is already a substring
                break
            if edge in node.edge:
                # existing edge is a substring of this one, so replace it
                edge_data[i] = node.edge
                break
        else:
            edge_data.append(node.edge)

    # Greedy approach to shortest superstring problem
    # Process by combining strings with the maximal overlap until only one
    # string remains.
    while len(edge_data) > 1:
        max_i = -1
        max_j = -1
        max_overlap = -1
        max_result = None
        for i, lhs in enumerate(edge_data):
            for j in range(i + 1, len(edge_data)):
                if i == j:
                    continue
                overlap, result = check_overlap(lhs, edge_data[j])
                if overlap > max_overlap:
                    max_i = i
                    max_j = j
                    max_overlap = overlap
                    max_result = result
        assert max_i >= 0 and max_j >= 0 and max_overlap >= 0 and max_result is not None
        edge_data[max_i] = max_result
        del edge_data[max_j]

    return edge_data[0]


def compile_tree(root: Node) -> tuple[str, list[int]]:
    """Compiles the tree into data"""
    edge_str = create_edge_str(root)
    out_nodes: list[int] = []
    next_child = 1
    process_nodes = deque([root])
    while process_nodes:
        node = process_nodes.popleft()
        edge_start = edge_str.find(node.edge)
        assert edge_start >= 0
        children_offset = next_child - len(out_nodes) if node.children else 0
        assert children_offset > 0 or not node.children
        value = CONSTELLATION_ABBREVS.index(node.value) if node.value else 255
        assert edge_start < (1 << 8)
        assert len(node.edge) < (1 << 4)
        assert children_offset < (1 << 7)
        assert len(node.children) < (1 << 5)
        assert value < (1 << 8)
        out_nodes.append(
            edge_start
            | (len(node.edge) << 8)
            | (children_offset << 12)
            | (len(node.children) << 19)
            | (value << 24)
        )
        assert out_nodes[-1] < (1 << 32)
        process_nodes.extend(node.children)
        next_child += len(node.children)
    return edge_str, out_nodes


def evaluate_input(text: str, edge_str: str, nodes: list[int]) -> Optional[str]:
    """Evaluates the constellation abbreviation"""
    text = text.lower()
    node = 0
    while True:
        node_data = nodes[node]
        children_offset = (node_data >> 12) & 0x7F
        num_children = (node_data >> 19) & 0x1F
        for offset in range(num_children):
            next_node = node + children_offset + offset
            next_node_data = nodes[next_node]
            edge_start = next_node_data & 0xFF
            edge_length = (next_node_data >> 8) & 0x0F
            edge = edge_str[edge_start : edge_start + edge_length]
            if text.startswith(edge):
                node = next_node
                text = text[edge_length:]
                break
        else:
            value = node_data >> 24
            if value >= len(CONSTELLATION_ABBREVS):
                return None
            return CONSTELLATION_ABBREVS[value]


def output_tree():
    """Creates and outputs the radix tree as C++ code"""

    root = create_tree()
    edge_str, nodes = compile_tree(root)
    for name, genitive, abbrev in CONSTELLATIONS:
        assert evaluate_input(name, edge_str, nodes) == abbrev
        assert evaluate_input(genitive, edge_str, nodes) == abbrev
        assert evaluate_input(abbrev, edge_str, nodes) == abbrev

    print("// This is generated code, it is recommended not to edit it manually.")
    print("// Generated by genconstellations.py")
    print("// Generator copyright (C) 2023-present, Celestia Development Team")
    print("//")
    print("// Generator based on the original version of this file,")
    print("// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>")
    print("//")
    print("// This program is free software; you can redistribute it and/or")
    print("// modify it under the terms of the GNU General Public License")
    print("// as published by the Free Software Foundation; either version 2")
    print("// of the License, or (at your option) any later version.")
    print(
        """
#include "constellation.h"

#include <array>
#include <cstdint>

#include <celutil/stringutils.h>

using namespace std::string_view_literals;


namespace
{
"""
    )

    print(
        f"constexpr std::array<std::string_view, {len(CONSTELLATION_ABBREVS)}> Abbrevs"
    )
    print("{")
    for i in range(0, len(CONSTELLATION_ABBREVS), 8):
        print("   ", end="")
        for j in range(i, min(i + 8, len(CONSTELLATION_ABBREVS))):
            print(f' "{CONSTELLATION_ABBREVS[j]}"sv,', end="")
        print()
    print("};")
    print()

    print("constexpr std::string_view EdgeData =")
    for i in range(0, len(edge_str), 64):
        j = min(i + 64, len(edge_str))
        if j == len(edge_str):
            print(f'    "{edge_str[i:j]}"sv;')
        else:
            print(f'    "{edge_str[i:j]}"')
    print()

    print(f"constexpr std::array<std::uint_least32_t, {len(nodes)}> Nodes")
    print("{")
    for i in range(0, len(nodes), 4):
        print("   ", end="")
        for j in range(i, min(i + 4, len(nodes))):
            print(f" UINT32_C(0x{nodes[j]:08x}),", end="")
        print()
    print("};")

    print(
        """
} // end unnamed namespace


std::tuple<std::string_view, std::string_view>
ParseConstellation(std::string_view name)
{
    std::string_view remaining = name;
    std::string_view abbrev;
    std::string_view suffix = remaining;
    std::uint_least32_t node = 0;
    std::uint_least32_t nodeData = Nodes[node];
    for (;;)
    {
        if (std::uint_least32_t value = nodeData >> 24; value < Abbrevs.size())
        {
            abbrev = Abbrevs[value];
            suffix = remaining;
        }

        std::uint_least32_t childOffset = (nodeData >> 12) & 0x7f;
        std::uint_least32_t numChildren = (nodeData >> 19) & 0x1f;
        bool isMatched = false;
        for (std::uint_least32_t i = 0; i < numChildren; ++i)
        {
            auto nextNode = node + childOffset + i;
            std::uint_least32_t nextNodeData = Nodes[nextNode];
            std::uint_least32_t edgeStart = nextNodeData & 0xff;
            std::uint_least32_t edgeLength = (nextNodeData >> 8) & 0x0f;
            std::string_view edge = EdgeData.substr(edgeStart, edgeLength);
            auto comparison = compareIgnoringCase(remaining, edge, edge.size());
            if (comparison > 0)
                continue;

            if (comparison == 0)
            {
                node = nextNode;
                nodeData = nextNodeData;
                remaining = remaining.substr(edgeLength);
                isMatched = true;
            }
            break;
        }

        if (!isMatched)
            break;
    }

    return std::make_tuple(abbrev, suffix);
}"""
    )


if __name__ == "__main__":
    output_tree()
