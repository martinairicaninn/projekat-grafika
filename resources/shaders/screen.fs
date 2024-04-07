#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int effect;// 0 - none, 1 - grayscale, 2 - edge detection, 3 - sharpen

const float offset = 1.0 / 300.0;

void main() {

    if (effect == 0) {
        FragColor = texture(screenTexture, TexCoords);
        return;
    }

    if (effect == 1) {
        vec3 color = texture(screenTexture, TexCoords).rgb;
        float avg = (dot(color, vec3(0.2126, 0.7152, 0.0722))) / 3;
        FragColor = vec4(avg, avg, avg, 1.0f);
        return;
    }

    vec2 offsets[9] = vec2[](
    vec2(-offset, offset),
    vec2(0.0, offset),
    vec2(offset, offset),
    vec2(-offset, 0.0),
    vec2(0.0, 0.0),
    vec2(0.0, offset),
    vec2(-offset, -offset),
    vec2(0.0, -offset),
    vec2(offset, -offset)
    );

    float kernel[9];
    if (effect == 2) {
        kernel = float[](
        -1, -1, -1,
        -1, 8, -1,
        -1, -1, -1
        );
    }

    if (effect == 3) {
        kernel = float[](
        0, -1, 0,
        -1, 5, -1,
        0, -1, 0
        );
    }


    vec3 sampleTex[9];
    for (int i = 0; i < 9; ++i) {
        sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
    }

    vec3 color = vec3(0.0);
    for (int i = 0; i < 9; ++i) {
        color += sampleTex[i] * kernel[i];
    }

    FragColor = vec4(color, 1.0f);

    //    FragColor = vec4(average, average, average, 1.0f);
}