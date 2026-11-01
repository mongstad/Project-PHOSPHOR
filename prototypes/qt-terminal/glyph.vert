#version 440
layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec4 color;
layout(location=0) out vec2 uv;
layout(location=1) out vec4 tint;
layout(std140,binding=0) uniform buf { mat4 matrix; float opacity; } ubuf;
void main() { uv=texcoord; tint=color; gl_Position=ubuf.matrix*vec4(position,0,1); }
