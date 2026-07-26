#version 330 core
in vec2 v_texCoord;
out vec4 fragColor;
uniform sampler2D u_texture;
uniform float u_time;
uniform vec2  u_resolution;

void main() {
    vec2 uv = v_texCoord;
    vec2 px = gl_FragCoord.xy;
    float bar = sin(px.y * 0.1 + u_time * 3.0) * 0.5 + 0.5;
    float glitch = step(0.92, bar);
    float offset = sin(u_time * 7.0 + px.y * 0.5) * 0.04;
    float rgbSplit = glitch * offset;
    float r = texture(u_texture, uv + vec2(rgbSplit + 0.01, 0.0)).r;
    float g = texture(u_texture, uv).g;
    float b = texture(u_texture, uv - vec2(rgbSplit - 0.01, 0.0)).b;
    float flicker = 1.0 - step(0.98, sin(u_time * 11.0 + px.y)) * 0.3;
    fragColor = vec4(r, g, b, 1.0) * flicker;
}