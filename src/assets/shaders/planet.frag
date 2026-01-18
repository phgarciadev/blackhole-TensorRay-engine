#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    float pad;
} pc;

void main() {
    /* Sample Texture */
    vec4 baseColor = texture(texSampler, fragUV);
    
    /* Lighting (Sun) */
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(pc.lightPos - fragPos); /* Light Direction */
    
    /* Lambert Diffuse */
    float diff = max(dot(N, L), 0.0);
    
    /* Ambient light (Space is dark, but starlight exists) */
    float ambient = 0.05;
    
    vec3 lightColor = vec3(1.0, 0.98, 0.9); /* Sun White/Yellow */
    
    vec3 finalColor = baseColor.rgb * (ambient + diff * lightColor);
    
    outColor = vec4(finalColor, 1.0);
}
