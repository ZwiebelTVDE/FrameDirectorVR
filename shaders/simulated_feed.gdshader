shader_type canvas_item;

uniform vec2 resolution = vec2(1920.0, 1080.0);
uniform float time = 0.0;
uniform int channel_number = 1;
uniform int is_sdi = 0;
uniform int framerate = 60;

void fragment() {
	vec2 uv = UV;
	float bars = floor(uv.x * 7.0);
	vec3 bar_col = vec3(0.75, 0.75, 0.75);
	if (bars < 1.0) bar_col = vec3(0.75, 0.75, 0.75);
	else if (bars < 2.0) bar_col = vec3(0.75, 0.75, 0.0);
	else if (bars < 3.0) bar_col = vec3(0.0, 0.75, 0.75);
	else if (bars < 4.0) bar_col = vec3(0.0, 0.75, 0.0);
	else if (bars < 5.0) bar_col = vec3(0.75, 0.0, 0.75);
	else if (bars < 6.0) bar_col = vec3(0.75, 0.0, 0.0);
	else bar_col = vec3(0.0, 0.0, 0.75);

	float sweep = fract(time * 0.12);
	vec3 col = bar_col;
	if (uv.y > 0.72 && uv.y < 0.88) {
		col = vec3(0.08, 0.09, 0.12);
		if (abs(uv.x - sweep) < 0.012) {
			col = vec3(0.95);
		}
	}
	if (uv.y >= 0.88) {
		col = mix(vec3(0.12, 0.45, 0.55), vec3(0.55, 0.16, 0.12), float(is_sdi));
	}

	float cx = abs(uv.x - 0.5);
	float cy = abs(uv.y - 0.38);
	if (cx < 0.16 && cy < 0.16) {
		col = vec3(0.05, 0.06, 0.08);
	}

	COLOR = vec4(col, 1.0);
}
