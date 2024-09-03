#version 450

layout (location = 0) out vec2 outUV;

void main() {
	vec2 positions[3] = vec2[3](
		vec2(+1,+1),
		vec2(-1,+1),
		vec2(+0,-1)
	);
	vec2 clip_pos = vec2(positions[gl_VertexIndex]);
	outUV = clip_pos / 2.0 + 0.5;
    gl_Position = vec4(clip_pos, 0.0f, 1.0f);
}