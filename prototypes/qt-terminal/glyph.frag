#version 440
layout(location=0) in vec2 uv;
layout(location=1) in vec4 tint;
layout(location=0) out vec4 fragColor;
layout(std140,binding=0) uniform buf { mat4 matrix; float opacity; } ubuf;
layout(binding=1) uniform sampler2D atlas;
void main() {
  vec4 material=texture(atlas,uv);
  // Default text uses the exact accepted premultiplied RGBA material. Explicit
  // SGR foreground uses its brightest channel as a metal-intensity mask.
  vec4 colored=vec4(max(material.r,max(material.g,material.b))*tint.rgb,material.a);
  fragColor=mix(material,colored,tint.a)*ubuf.opacity;
}
