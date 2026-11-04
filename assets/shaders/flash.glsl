// Mixes the pixels towards white by each drawing's own amount.
instance uniform float amount = 0.0;

void fragment() {
    COLOR.rgb = mix(COLOR.rgb, vec3(COLOR.a), amount);
}
