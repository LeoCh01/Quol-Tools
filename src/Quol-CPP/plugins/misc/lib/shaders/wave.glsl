#version 330 core
in vec2 v_texCoord;
out vec4 fragColor;
uniform sampler2D u_texture;
uniform float u_time;
uniform vec2  u_resolution;

void main() {
    vec2 px = gl_FragCoord.xy;
    vec2 center = u_resolution * 0.5;
    float dist = length(px - center);
    float wave = sin(dist * 0.05 - u_time * 2.0) * 8.0;
    vec2 dir = normalize(px - center + 0.001);
    vec2 displaced = v_texCoord + dir * wave / u_resolution;
    fragColor = texture(u_texture, displaced);
}