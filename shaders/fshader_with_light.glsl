#version 330 core
out vec4 fColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 UV;
in vec4 FragPosLightSpace;

struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    int type; // 0 = Directional, 1 = Point
    float constant;
    float linear;
    float quadratic;
    bool enabled;
};

#define MAX_LIGHTS 16
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

uniform vec4 objectColor;   
uniform vec3 viewPos;
uniform float brightness;
uniform float shininess;
uniform vec3 specularColor;

uniform vec3 ambientColor;
uniform float ambientStrength;

uniform sampler2D textureSampler;
uniform bool hasTexture;
uniform sampler2D toonSampler;
uniform sampler2D shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main()
{
    vec4 textureColor = vec4(1.0);
    if (hasTexture) {
        textureColor = texture(textureSampler, UV);
    }
    
    vec3 finalObjectColor = objectColor.rgb * textureColor.rgb;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Ambient
    vec3 ambient = ambientStrength * ambientColor * finalObjectColor;

    vec3 lighting = vec3(0.0);

    for(int i = 0; i < numLights; i++) {
        if(!lights[i].enabled) continue;
        
        vec3 lightDir;
        float attenuation = 1.0;

        if (lights[i].type == 0) { // Directional
             lightDir = normalize(-lights[i].direction);
        } else { // Point
             lightDir = normalize(lights[i].position - FragPos);
             float distance = length(lights[i].position - FragPos);
             attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
        }

        // Diffuse (Toon)
        float dotProd = dot(norm, lightDir);
        float lightIntensity = dotProd * 0.5 + 0.5;
        lightIntensity = clamp(lightIntensity, 0.01, 0.99); 
        vec3 toonColor = texture(toonSampler, vec2(0.5, lightIntensity)).rgb;
        
        float shadow = 0.0;
        if (i == 0 && lights[i].type == 0) { // Only first directional light casts shadow
             shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
        }

        vec3 diffuse = toonColor * lights[i].color * lights[i].intensity * (1.0 - shadow);

        // Specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
        vec3 specular = spec * lights[i].color * lights[i].intensity * specularColor * (1.0 - shadow); 

        lighting += (diffuse * finalObjectColor + specular) * attenuation;
    }
    
    vec3 result = ambient + lighting;
    
    // Fallback if no lights or ambient is too low, just to see something
    if (numLights == 0 && ambientStrength < 0.01) {
         result = finalObjectColor;
    }

    fColor = vec4(result, textureColor.a * objectColor.a);
}