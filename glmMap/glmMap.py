#! /usr/bin/env python3

#Pre-requisites software required to run this script:
    #Python 2.7: Modules:: sys, re, os
    #Graphviz
        ##If you just installed Graphviz, "restart" your computer for Graphviz commandline execution to work

#Purpose of this script and How can a modeler use it:
    #Convert a GridLAB-D file (.glm) to a Graphviz file (.dot)
    #Use that .dot to generate .svg
    #Use any web browser to open the .svg file - SIMPLE

#Usage:
    #python glmMap.py inputfile.glm outputfile.dot

#Licensing: Copyright 2016; licensing falls under GridLAB-D
#Email Contact: sri at pnnl dot gov
#Devloped at: Pacific Northwest National Laboratory
#Acknowledgements: This tool is developed using a base script provided by Philip Douglass, Technical University of Denmark in 2013

#Version and Patch Notes:
    ##05/20/2016: Added    :: line spacing; object names; object color coding
    ##06/02/2016: Bug Fix  :: If the node names has ':' in it, grapgviz errors out. replaced ':' with '_' as well

import sys;
import re;
import os

# # argument validation
if len(sys.argv) < 3:
    raise Exception("Usage: python3 glmMap.py model.glm model.dot")
# if

# # recursively include dependencies

# get initial lines
with open(sys.argv[1], 'r') as f:
    lines = f.readlines()
# with

# helper function
def extract_field(_line)->str:
    line = _line.strip()

    field = None
    if line.startswith("object "):
        tmp_list = line.split(" ");
        new_tmp_list = []
        for tmp in tmp_list:
            tmp = tmp.strip()
            if "" != tmp:
                new_tmp_list.append(tmp)
            # if
        # for
        field = new_tmp_list[1]

    else:
        field_type = line.split(" ")[0]
        if field_type in ["#include", "name", "from", "to", "phases", "length"]:
            field = line[(len(field_type) + 1):].rstrip(";")
        # if
    # elif line.endswith(";"):
    #     field_type = line.split(" ")[0]
    #     if field_type in ["#include", "name", "from", "to", "phases", "length"]:
    #         field = line[(len(field_type) + 1):-1]
    #     # if
    # if-else

    return field
# if

# failsafe counter to mitigate cyclic includes
_append_includes_depth = 0
def append_includes(_lines:list)->list:
    global _append_includes_depth
    _append_includes_depth += 1

    new_lines = []
    for l in _lines:
        l = l.strip()
        if "" != l:
            new_lines.append(l)
        # if

        if l.startswith("#include"):
            # inc_file = l.split(" ")[1][1:-1]
            inc_file = extract_field(l)
            if inc_file.startswith("\""):
                inc_file = inc_file[1:-1]
            # if
            with open(inc_file, "r") as f:
                inc_lines = f.readlines()
            # with
            new_lines.extend(append_includes(inc_lines))
        # if
    # for

    _append_includes_depth -= 1
    return new_lines
# get includes

# recurse
lines = append_includes(lines)

# # sanitize lines by removing comments and blank lines

new_lines = []
for s in range(len(lines)):
    new_line = lines[s]

    # discard comments
    if "//" in lines[s]:
        new_line = new_line[:new_line.index("//")].strip()
    # if

    # skip empty lines
    if "" == new_line:
        continue
    # if

    # add sanitized line
    new_lines.append(new_line)
# for
lines = new_lines

# # extract information from objects

# helper function
def verify_none(_obj, _field):
    if None is not _obj[_field]:
        raise Exception(f"object {_obj['type']}:{_obj['name']} has more than one `{_field}`.")
    # if
# verify_none()

# helper function
obj_list = []
def extract_object(_lines:list, _s:int):
    lines = [l.strip() for l in _lines]
    s = _s

    # validation
    if not lines[s].startswith("object "):
        return s + 1
    # if

    # object we will save
    obj = {
        "name": None,
        "type": None,
        "from": None,
        "to": None,
        "phases": None,
        "length": None,
        "shape": "box", # default
        "raw": []
    }

    # extract object type
    obj["type"] = extract_field(lines[s])

    # find end of object
    end_s = s + 1
    while end_s < len(lines):
        # recursive or termination conditions
        if lines[end_s].startswith("object "):
            # recurse
            end_s = extract_object(lines, end_s)
        elif lines[end_s].startswith("}"):
            break
        # if-elif-else

        # termination after recursion
        if end_s >= len(lines):
            break
        # if

        # various important fields
        if lines[end_s].startswith("name "):
            verify_none(obj, "name")
            obj["name"] = extract_field(lines[end_s])

        elif lines[end_s].startswith("from "):
            verify_none(obj, "from")
            obj["from"] = extract_field(lines[end_s])

        elif lines[end_s].startswith("to "):
            verify_none(obj, "to")
            obj["to"] = extract_field(lines[end_s])

        elif lines[end_s].startswith("phases "):
            verify_none(obj, "phases")
            obj["phases"] = extract_field(lines[end_s]).upper()

        elif lines[end_s].startswith("length "):
            verify_none(obj, "length")
            length = extract_field(lines[end_s])
            # remove possible `ft` at end
            obj["length"] = length.split(" ")[0]

        # if-elif

        obj["raw"].append(lines[end_s])

        end_s += 1
    # while
    global obj_list
    obj_list.append(obj)

    return end_s
# extract_object

# for line, extract object if found
s = 0
while s < len(lines):
    # extract object
    if lines[s].startswith("object "):
        s = extract_object(lines, s)
    # if

    s += 1
# for

# # enumerate nodes/edges

# helper function
def get_obj_by_name(_name:str)->dict:
    # get object
    obj = None
    global obj_list
    for obj_tmp in obj_list:
        if _name == obj_tmp["name"]:
            obj = obj_tmp
            break
        # if
    # for

    return obj
# get_obj_by_name()

# enumerate
names = set([d["name"] for d in obj_list])
nodes = set()
edges = []
for obj in obj_list:
    f = obj["from"]
    if f not in names:
        f = None
    # if
    t = obj["to"]
    if t not in names:
        t = None
    # if

    if (None is not f) and (None is not t):
        # create edge
        name = obj["name"]
        nodes.add(f)
        nodes.add(t)
        d = {
            "name": name,
            "edge": (f, t)
        }
        edges.append(d)

        # if a node is pointed to by a transformer (read: if a node is on the low-side of a transformer), then the node is an oval
        if "transformer" == obj["type"]:
            to_obj = get_obj_by_name(t)
            to_obj["shape"] = "oval"
        # if

    # if
# for

# # write output dot file

# helper function
def sanitize_for_dot(_string:str)->str:
    # don't trust dot to handle dashes or colons
    return _string.rstrip(';').replace('-','_').replace(':','_')   
# sanitize_for_dot()

# write output dot file
nodes = sorted(nodes)
edges = sorted(edges, key=lambda x: x["edge"])
if 0 < len(nodes):
    with open(sys.argv[2], "w") as file:
        # start dot file
        file.write("digraph {\n")

        for node in nodes:
            # get object
            obj = None
            for obj_tmp in obj_list:
                if node == obj_tmp["name"]:
                    obj = obj_tmp
                    break
                # if
            # for

            node = sanitize_for_dot(node)
            file.write(f"\t{node} [shape={obj['shape']}];\n")
        # for
        for edge in edges:
            name = edge["name"]
            edge = edge["edge"]
            f = sanitize_for_dot(edge[0])
            t = sanitize_for_dot(edge[1])

            # default
            style = "solid"
            color = "black"

            # get object
            obj = get_obj_by_name(name)

            # style
            #   four phase --> bold
            #   three phase --> dashed
            #   two/one/zero phase --> solid
            if 3 < len(obj["phases"]):
                style = "bold"
            elif 2 < len(obj["phases"]):
                style = "dashed"
            else:
                pass # style = /default/
            # if-elif-else

            # color (why doesn't anything else have a specific color?)
            #   transformer --> red
            #   triplex_line --> green
            #   fuse --> blue
            if "transformer" == obj["type"]:
                color = "red"
            elif "triplex_line" == obj["type"]:
                color = "green"
            elif "fuse" == obj["type"]:
                color = "blue"
            else:
                pass # color = /default/
            # if-elif-else

            # don't trust dot to handle dashes or colons
            name = sanitize_for_dot(name)

            # create edge label
            label = f"/type: {obj['type']}\\l /name: {name}\\l /phases: {obj['phases'].upper()}\\l"

            # if edge is a line, it has length
            if obj["type"].endswith("line"):
                # default
                length = "?"

                # convert to integer value if found
                if None is not obj["length"]:
                    # add comma separation between thousands
                    length = f"{int(obj['length']):,}"
                # if

                # update label
                label += f"/length: {length} ft\\l"
            # if

            # write edge to file
            file.write(f"\t{f} -> {t} [style={style}, color={color}, label=\"{label}\"];\n")
        # for

        # terminate dot file
        file.write("}\n")
    # with
# if

# # dot to svg

os.system("dot -Tsvg -O " + sys.argv[2])
