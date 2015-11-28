#version 420 core
in vec2 fs_position;
in vec2 fs_texCoord;
out vec2 vs_texCoord;

out gl_PerVertex {
	vec4 gl_Position;
};

void main()
{
   vs_texCoord = fs_texCoord;
   gl_Position = vec4(fs_position, 0.0, 1.0);
}
