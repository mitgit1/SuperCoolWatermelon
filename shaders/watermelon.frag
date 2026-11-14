#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform LightUBO {
    vec3 lightDir;
    vec3 lightColor;
    vec3 viewPos;
} light;


float hash3(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float valueNoise3(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(mix(hash3(i + vec3(0,0,0)), hash3(i + vec3(1,0,0)), u.x),
            mix(hash3(i + vec3(0,1,0)), hash3(i + vec3(1,1,0)), u.x), u.y),
        mix(mix(hash3(i + vec3(0,0,1)), hash3(i + vec3(1,0,1)), u.x),
            mix(hash3(i + vec3(0,1,1)), hash3(i + vec3(1,1,1)), u.x), u.y),
        u.z
    );
}


vec3 watermelonRind(vec3 N) {
    float longitude = atan(N.z, N.x);

    float wobble = valueNoise3(N * 3.0) * 0.8;

    float stripeFreq = 12.0;
    float stripes = sin(longitude * stripeFreq + wobble);

    float stripeMask = smoothstep(-0.15, 0.15, stripes);

    vec3 paleGreen = vec3(0.55, 0.75, 0.45); // Светло-зеленый (основа)
    vec3 darkGreen = vec3(0.08, 0.25, 0.12); // Темно-зеленый (полосы)

    vec3 color = mix(paleGreen, darkGreen, stripeMask);

    float mottle = valueNoise3(N * 15.0);
    color *= (0.85 + 0.15 * mottle);

    float poleDarken = smoothstep(0.8, 1.0, abs(N.y));
    color = mix(color, darkGreen, poleDarken * 0.7);

    return color;
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 baseColor = watermelonRind(N);

    vec3 L = normalize(-light.lightDir);
    vec3 V = normalize(light.viewPos - fragWorldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.4;

    vec3 ambient  = baseColor * 0.35; // Чуть больше света в тенях
    vec3 diffuse  = baseColor * light.lightColor * diff * 0.8;
    vec3 specular = light.lightColor * spec;

    vec3 finalColor = ambient + diffuse + specular;

    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    outColor = vec4(finalColor, 1.0);
}
