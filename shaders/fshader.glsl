#version 330 core
out vec4 fColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 UV;

uniform vec4 objectColor;   
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D textureSampler;
uniform bool hasTexture;
uniform sampler2D toonSampler;

void main()
{
    vec4 textureColor = vec4(1.0);
    if (hasTexture) {
        textureColor = texture(textureSampler, UV);
    }
    // 只使用物体颜色和贴图，不做任何光照
    vec4 finalColor = objectColor * textureColor;
    fColor = vec4(finalColor.rgb, finalColor.a);
}