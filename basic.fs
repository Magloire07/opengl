#version 330 core
in vec3 v_direction;
uniform samplerCube u_cubemap;
out vec4 FragColor;

void main()
{
    vec3 dir = normalize(v_direction);
    FragColor = texture(u_cubemap, dir);
}