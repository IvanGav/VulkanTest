Push constants to `vert` shader, with:
- Single struct uniform
- Per instance data struct array
Textures array for fragment shader (at binding=0)
Specify what the shader files are
Maybe compile glsl during runtime

Draw call lets:
- Write the single struct uniform
- Write the per instance data array
- Call a function to draw a MeshRef with number of instances specified