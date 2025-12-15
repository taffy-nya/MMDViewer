#version 330 core
out vec4 fColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec4 edge_color;

// Light structure
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
uniform vec3 ambientColor;
uniform float ambientStrength;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lighting = vec3(0.0);
    
    // Simplified lighting for edges: just diffuse contribution
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

        float diff = max(dot(norm, lightDir), 0.0);
        lighting += diff * lights[i].color * lights[i].intensity * attenuation;
    }

    vec3 ambient = ambientStrength * ambientColor;
    vec3 result = (ambient + lighting) * edge_color.rgb;
    
    // Fallback if dark
    if (length(result) < 0.01 && length(edge_color.rgb) > 0.01) {
        // Don't let it be completely black if the edge color is not black
        // But we want it to be dark in shadow.
        // Let's just clamp it slightly so it's not invisible?
        // Actually, MMD edges DO disappear in pitch black.
    }

    fColor = vec4(result, edge_color.a);
}