#version 330 core
in vec2 v_texCoord;
out vec4 fragColor;
uniform sampler2D u_prevFrame;
uniform float u_time;
uniform vec2  u_resolution;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    vec4 prev = texture(u_prevFrame, v_texCoord);
    
    // on first frame, initialize with values
    if (u_time < 0.1) {
        float rnd = hash(v_texCoord * u_resolution);
        fragColor = vec4(vec3(rnd > 0.5 ? 1.0 : 0.0), 1.0);
        return;
    }


    // count live neighbours
    vec2 px = 1.0 / u_resolution;
    int n = 0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            if (x == 0 && y == 0) continue;
            vec2 uv = v_texCoord + vec2(x, y) * px;
            if (texture(u_prevFrame, uv).r > 0.5) {
                n++;
            }
		
        }
    }

    if (prev.r > 0.5) {
        // live cell
        if (n < 2 || n > 3) {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0); // dies
        } else {
            fragColor = vec4(1.0, 1.0, 1.0, 1.0); // lives
        }
    } else {
        // dead cell
        if (n == 3) {
            fragColor = vec4(1.0, 1.0, 1.0, 1.0); // becomes alive
        } else {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0); // stays dead
        }
    }
}