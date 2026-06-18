#version 330 core
in vec2 v_uv;
out vec4 FragColor;

uniform sampler2D u_ScreenTexture;
uniform int u_EffectMode;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(u_ScreenTexture, 0));

    if (u_EffectMode == 0) {
        // Mode 0: Normal
        FragColor = texture(u_ScreenTexture, v_uv);
    } 
    else if (u_EffectMode == 1) {
        // Mode 1: Grayscale
        vec4 color = texture(u_ScreenTexture, v_uv);
        float average = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
        FragColor = vec4(vec3(average), color.a);
    } 
    else if (u_EffectMode == 2) {
        // Mode 2: Color Inversion
        vec4 color = texture(u_ScreenTexture, v_uv);
        FragColor = vec4(1.0 - color.rgb, color.a);
    } 
    else if (u_EffectMode == 3) {
        // Mode 3: Edge Detection (Sobel Kernel)
        float offsets[18] = float[](
            -texelSize.x,  texelSize.y,  // top-left
             0.0,          texelSize.y,  // top-center
             texelSize.x,  texelSize.y,  // top-right
            -texelSize.x,  0.0,          // center-left
             0.0,          0.0,          // center-center
             texelSize.x,  0.0,          // center-right
            -texelSize.x, -texelSize.y,  // bottom-left
             0.0,         -texelSize.y,  // bottom-center
             texelSize.x, -texelSize.y   // bottom-right
        );

        float kernelX[9] = float[](
            -1,  0,  1,
            -2,  0,  2,
            -1,  0,  1
        );

        float kernelY[9] = float[](
            -1, -2, -1,
             0,  0,  0,
             1,  2,  1
        );

        vec3 sampleTex[9];
        for (int i = 0; i < 9; i++) {
            vec2 offset = vec2(offsets[2*i], offsets[2*i+1]);
            sampleTex[i] = texture(u_ScreenTexture, v_uv + offset).rgb;
        }

        vec3 colX = vec3(0.0);
        vec3 colY = vec3(0.0);
        for (int i = 0; i < 9; i++) {
            colX += sampleTex[i] * kernelX[i];
            colY += sampleTex[i] * kernelY[i];
        }

        vec3 col = sqrt(colX * colX + colY * colY);
        // Combine edges with original to make it stand out beautifully
        vec3 orig = texture(u_ScreenTexture, v_uv).rgb;
        FragColor = vec4(col, 1.0);
    }
    else if (u_EffectMode == 4) {
        // Mode 4: Chromatic Aberration
        float rOffset = 0.003;
        float gOffset = 0.0015;
        float bOffset = 0.0;

        float r = texture(u_ScreenTexture, v_uv + vec2(rOffset, 0.0)).r;
        float g = texture(u_ScreenTexture, v_uv + vec2(gOffset, 0.0)).g;
        float b = texture(u_ScreenTexture, v_uv + vec2(bOffset, 0.0)).b;

        FragColor = vec4(r, g, b, 1.0);
    }
    else {
    FragColor = texture(u_ScreenTexture, v_uv);
}
}
