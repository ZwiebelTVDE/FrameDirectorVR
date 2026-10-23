shader_type canvas_item;

// Mirrors CameraFeed.FeedDataType: 1 = RGB, 2 = YCbCr interleaved, 3 = YCbCr separate.
uniform int mode = 1;
uniform bool flip_v = false;
uniform sampler2D rgba_tex : filter_linear;
uniform sampler2D y_tex : filter_linear;
uniform sampler2D cbcr_tex : filter_linear;

vec3 ycbcr_to_rgb(float y, vec2 cbcr) {
	// BT.709 limited range, which is what HDMI/SDI video sources send.
	y = (y - 16.0 / 255.0) * (255.0 / 219.0);
	vec2 c = (cbcr - 128.0 / 255.0) * (255.0 / 224.0);
	return clamp(vec3(
		y + 1.5748 * c.y,
		y - 0.1873 * c.x - 0.4681 * c.y,
		y + 1.8556 * c.x
	), 0.0, 1.0);
}

void fragment() {
	vec2 uv = UV;
	if (flip_v) {
		uv.y = 1.0 - uv.y;
	}
	vec3 col;
	if (mode == 3) {
		col = ycbcr_to_rgb(texture(y_tex, uv).r, texture(cbcr_tex, uv).rg);
	} else if (mode == 2) {
		// Packed 4:2:2 (YUYV): each texel holds Y0 Cb Y1 Cr for two pixels.
		vec4 s = texture(y_tex, uv);
		float px = uv.x * float(textureSize(y_tex, 0).x) * 2.0;
		float y = mod(floor(px), 2.0) < 1.0 ? s.r : s.b;
		col = ycbcr_to_rgb(y, s.ga);
	} else if (mode == 1) {
		col = texture(rgba_tex, uv).rgb;
	} else {
		col = vec3(0.06, 0.06, 0.07); // no frames yet
	}
	COLOR = vec4(col, 1.0);
}
