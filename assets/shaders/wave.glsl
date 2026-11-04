// Sways the vertices with the time, tints the pixels and blends in a second texture.
uniform vec4 tint = vec4(0.4, 0.8, 1.0, 1.0);
uniform float speed = 3.0;
uniform sampler2D overlay;

void vertex() {
    VERTEX.y += sin(TIME * speed + VERTEX.x * 0.05) * 12.0;
}

void fragment() {
    vec4 other = texture(overlay, UV);
    COLOR = mix(COLOR * tint, other * COLOR.a, 0.25);
}
