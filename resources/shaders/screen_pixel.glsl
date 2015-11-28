#version 420 core
in vec2 vs_texCoord;
out vec4 ps_color;
uniform sampler2D sampler_0;

void main()
{
   ps_color = texture(sampler_0, vs_texCoord);
}
