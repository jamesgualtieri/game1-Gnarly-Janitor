#!/usr/bin/env python

#based on 'export-scene.py'

#which is based on 'export-sprites.py' and 'glsprite.py' from TCHOW Rainbow; code used is released into the public domain.

#Note: Script meant to be executed from within blender, as per:
#blender --background --python export-scene.py -- <infile.blend> <layer> <outfile.scene>

import sys,re

args = []
for i in range(0,len(sys.argv)):
    if sys.argv[i] == '--':
        args = sys.argv[i+1:]

if len(args) != 2:
    print("\n\nUsage:\nblender --background --python export-scene.py -- <infile.blend>[:layer] <outfile.scene>\nExports the transforms of objects in layer (default 1) to a binary blob, indexed by the names of the objects that reference them.\n")
    exit(1)

infile = args[0]
layer = 1
m = re.match(r'^(.*):(\d+)$', infile)
if m:
    infile = m.group(1)
    layer = int(m.group(2))
outfile = args[1]

print("Will export layer " + str(layer) + " from file '" + infile + "' to scene '" + outfile + "'");

import bpy
import mathutils
import struct
import math

#---------------------------------------------------------------------
#Export walkmesh:

bpy.ops.wm.open_mainfile(filepath=infile)

# For my project, the walkmesh is in a dedicated Blender file, so it is meant only 
# to export a single walkmesh and no other mesh files.

#Walkmesh file format:
#triangles - all triangles (magic number tri0)
#vertices - all vertices (magic number vrt0)
#normals - all normals of the vertices, which we need if we want to interpolate 
#   them later? (magic number nrm0)

triangles = b""
vertices = b""
normals = b""

def write_walkmesh(obj):
    #Looked at https://docs.python.org/2.7/library/struct.html#format-characters
    #   bc i didn't know how struct pack works 
    global triangles
    global vertices
    global normals
    for triangle in obj.data.polygons:
        verts = triangle.vertices
        triangles += struct.pack('III', verts[0], verts[1], verts[2])
    for vertex in obj.data.vertices:
        coords = vertex.co
        norms = vertex.normal
        vertices += struct.pack('fff', coords[0], coords[1], coords[2])
        normals += struct.pack('fff', norms[0], norms[1], norms[2])

for obj in bpy.data.objects:
    if obj.type == 'MESH':
        write_walkmesh(obj)
    else: pass

#write the strings chunk and scene chunk to an output blob:
blob = open(outfile, 'wb')

def write_chunk(magic, data):
    blob.write(struct.pack('4s',magic)) #type
    blob.write(struct.pack('I', len(data))) #length
    blob.write(data)

write_chunk(b'tri0', triangles)
write_chunk(b'vrt0', vertices)
write_chunk(b'nrm0', normals)

print("Wrote " + str(blob.tell()) + " bytes to '" + outfile + "'")
blob.close()
