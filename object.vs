#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

uniform mat4 u_MVP;
uniform mat4 u_Model;

out vec3 v_normal;
out vec2 v_uv;
out vec3 v_worldPos;

void main()
{
    v_normal = mat3(u_Model) * a_normal;
    v_uv = a_uv;
    v_worldPos = (u_Model * vec4(a_position, 1.0)).xyz;
    gl_Position = u_MVP * vec4(a_position, 1.0);
}
