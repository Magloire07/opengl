#version 330 core
layout(location = 0) in vec3 a_position;

uniform mat4 u_MVP;
out vec3 v_direction;

void main()
{
    // use raw vertex position as cubemap lookup direction (world space)
    v_direction = a_position;
    gl_Position = u_MVP * vec4(a_position, 1.0);
}
