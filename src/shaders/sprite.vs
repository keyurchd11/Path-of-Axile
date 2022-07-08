#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>

out vec2 TexCoords;

uniform mat4 projection;
out vec4 frag_position;
uniform mat4 model;


void main()
{
    TexCoords = vertex.zw;
    gl_Position = projection * model * vec4(vertex.xy, 0.0, 1.0);
    frag_position = model * vec4(vertex.xy, 0.0, 1.0);
}