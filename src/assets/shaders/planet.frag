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
    vec4 lightAndStar;
    vec4 colorParams;
} pc;

void main() {
    /* Sample Texture */
    vec4 baseColor = texture(texSampler, fragUV);
    
    /* Lighting (Sun) */
    vec3 N = normalize(fragNormal);
    /* Extract Params */
    vec3 lightPos = pc.lightAndStar.xyz;
    float isStar = pc.lightAndStar.w;
    
    vec3 L = normalize(lightPos - fragPos); /* Light Direction */
    
    /* Lambert Diffuse */
    float diff = max(dot(N, L), 0.0);
    
    /* Ambient light (Space is dark, but starlight exists) */
    float ambient = 0.05;
    
    vec3 lightColor = vec3(1.0, 0.98, 0.9); /* Sun White/Yellow */
    
    
    vec3 finalColor;
    
    if (isStar > 0.5) {
        /* Star is emissive, ignore lighting. 
           If texture is present, use it. If it's the sun, we want it BRIGHT.
           Use colorParams to tint the white texture (if generic).
        */
        vec3 tint = pc.colorParams.rgb;
        /* Safety: avoid multiplying by 0 if color passed is 0 (should not happen given factory) */
        if (length(tint) < 0.01) tint = vec3(1.0);
        
        finalColor = baseColor.rgb * tint * 2.0; /* 2.0 brightness */
    } else {
        /* Rim Lighting (Fresnel) - Gives atmospheric volume feel */
        /* Since we use RTC, Camera is at (0,0,0) in World Space (Relative) */
        /* Therefore View Vector is simply -fragPos normalized */
        vec3 V = normalize(-fragPos); 
        float rim = 1.0 - max(dot(N, V), 0.0);
        rim = smoothstep(0.6, 1.0, rim);
        vec3 rimColor = vec3(0.4, 0.6, 1.0); /* Blueish atmosphere hint */
        
        vec3 litColor = baseColor.rgb * (ambient + diff * lightColor);
        finalColor = litColor + (rim * rimColor * 0.5);
    }
    
    outColor = vec4(finalColor, 1.0);
}
