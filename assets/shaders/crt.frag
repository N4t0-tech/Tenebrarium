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
    vec2 uv = crtCurve(fragTexCoord);

    // Fuera de la pantalla curva → negro
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 color = texture(texture0, uv);

    // Scanlines: líneas horizontales sutiles
    float scanline = sin(uv.y * resolution.y * 3.14159) * 0.03;
    color.rgb -= scanline;

    // Viñeta: oscurecer los bordes
    vec2 vig = uv * (1.0 - uv.yx);
    float vignette = pow(vig.x * vig.y * 12.0, 0.3);
    color.rgb *= vignette;

    finalColor = color * fragColor;
}
