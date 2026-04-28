#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;

out vec4 finalColor;

// Barrel distortion para simular curvatura de tubo CRT
vec2 crtCurve(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs(uv.yx) / vec2(7.0, 5.0);
    uv = uv + uv * offset * offset;
    return uv * 0.5 + 0.5;
}

void main() {
    vec2 curved = crtCurve(fragTexCoord);

    // Las esquinas fuera de la curva → negro
    if (curved.x < 0.0 || curved.x > 1.0 || curved.y < 0.0 || curved.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Curvatura sutil en el muestreo
    vec2 sampleUv = mix(fragTexCoord, curved, 0.25);
    vec4 color = texture(texture0, sampleUv);

    // Scanlines usando la UV curva para dar sensación CRT
    float scanline = sin(curved.y * resolution.y * 3.14159) * 0.03;
    color.rgb -= scanline;

    // Viñeta usando la UV curva → oscurece los bordes como pantalla curva
    vec2 vig = curved * (1.0 - curved.yx);
    float vignette = pow(vig.x * vig.y * 12.0, 0.3);
    // Reducir oscurecimiento en el borde inferior
    vignette = mix(vignette, 0.3, curved.y * 0.30);
    color.rgb *= vignette;

    finalColor = color * fragColor;
}
