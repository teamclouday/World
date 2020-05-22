@ECHO OFF
ECHO Compiling Simple Shaders
glslc -fshader-stage=fragment simple.frag.glsl -o simple.frag.spv
glslc -fshader-stage=vertex simple.vert.glsl -o simple.vert.spv