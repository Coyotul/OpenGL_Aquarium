#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D diffuseTexture;

void main()
{
    vec4 texColor = texture(diffuseTexture, TexCoords);
    
    if (texColor.a < 0.5) {
        discard;
    } else {
        FragColor = texColor;
    }
}
