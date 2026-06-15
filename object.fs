#version 330 core
in vec3 v_normal;
in vec2 v_uv;
in vec3 v_worldPos;

uniform sampler2D u_Texture;
uniform samplerCube u_Cubemap;
uniform int u_UseTexture;
uniform vec3 u_Color;
uniform vec3 u_ViewPos;

out vec4 FragColor;

void main()
{
    // 1. Setup vectors
    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_ViewPos - v_worldPos);
    vec3 L = normalize(vec3(1.0, 1.0, 0.5));
    vec3 H = normalize(L + V);

    // 2. Base color determination and conversion from sRGB to Linear space
    vec3 baseColor = u_Color;
    if (u_UseTexture == 1) {
        vec3 tex = texture(u_Texture, v_uv).rgb;
        if (length(tex) > 0.01) {
            baseColor = tex;
        }
    } else if (u_UseTexture == 2) {
        // Checkerboard pattern
        float checker = mod(floor(v_uv.x) + floor(v_uv.y), 2.0);
        baseColor = mix(vec3(0.25, 0.25, 0.28), vec3(0.45, 0.45, 0.48), checker);
    }
    vec3 baseLinear = pow(baseColor, vec3(2.2));

    // 3. Fresnel (Schlick's approximation)
    // Non-metal default F0 (0.04)
    float F0 = 0.04;
    float cosTheta = max(dot(N, V), 0.0);
    float F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

    // 4. Direct Illumination (Blinn-Phong)
    vec3 lightColor = vec3(1.8); // strong direct light
    float diff = max(dot(N, L), 0.0);
    vec3 directDiffuse = (1.0 - F) * baseLinear * diff * lightColor;

    float spec = pow(max(dot(N, H), 0.0), 64.0);
    vec3 directSpecular = vec3(spec * 0.5) * lightColor;

    // 5. Indirect Illumination (Environment Mapping)
    vec3 R = reflect(-V, N);
    vec3 envColor = pow(texture(u_Cubemap, R).rgb, vec3(2.2));
    // Very subtle reflection for non-metallic surfaces
    vec3 indirectSpecular = F * envColor * 0.05;

    // Ambient (Indirect Diffuse) — strong enough for solid appearance
    vec3 indirectDiffuse = baseLinear * 0.4;

    // 6. Combine all components (Linear Space)
    vec3 linearColor = directDiffuse + directSpecular + indirectSpecular + indirectDiffuse;

    // 7. Gamma correction (Linear to sRGB space)
    vec3 srgbColor = pow(linearColor, vec3(1.0 / 2.2));

    FragColor = vec4(srgbColor, 1.0);
}
