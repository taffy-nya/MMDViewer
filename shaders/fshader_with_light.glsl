#version 330 core
out vec4 fColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 UV;

uniform vec4 objectColor;   
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float brightness;

uniform sampler2D textureSampler;
uniform bool hasTexture;
uniform sampler2D toonSampler;

void main()
{
    vec4 textureColor = vec4(1.0);
    if (hasTexture) {
        textureColor = texture(textureSampler, UV);
    }
    
    vec3 finalObjectColor = objectColor.rgb * textureColor.rgb;

    // --- Toon Shading Light Calculation ---
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    float lightIntensity = dot(norm, lightDir) * 0.5 + 0.5;
    vec3 toonColor = texture(toonSampler, vec2(0.5, lightIntensity)).rgb;

    float ambientStrength = 0.4; // MMD ambient is usually stronger
    vec3 ambient = ambientStrength * lightColor;

    vec3 diffuse = toonColor * lightColor;
    
    // Specular can be kept for highlights
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor * objectColor.rgb;

    // --- Final Color ---
    vec3 result = (ambient + diffuse) * finalObjectColor + specular;
    
    fColor = vec4(result * brightness, textureColor.a * objectColor.a);
}