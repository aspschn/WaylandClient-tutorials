#version 330 core

precision mediump float;

out vec4 fragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    vec4 texColor = texture(ourTexture, TexCoord);
    if (texColor.a < 0.1) {
        discard;
    }
    fragColor = texColor;
}

