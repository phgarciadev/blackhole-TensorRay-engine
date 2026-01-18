#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos; /* Sun position */
    float pad;
} pc;

void main() {
    /* World Position */
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    gl_Position = pc.proj * pc.view * worldPos;
    
    fragUV = inUV;
    
    /* Normal in World Space (assuming uniform scale, otherwise use inverse transpose) */
    fragNormal = normalize(mat3(pc.model) * inNormal);
    
    fragPos = worldPos.xyz;
}
