// A reload sweep over a round sprite: `progress` goes from 0 (just used) to 1 (ready). The part not reloaded yet is
// dark, and a bright edge turns clockwise from the top; once ready, the sprite is drawn as it is.
uniform float progress = 1.0;

void fragment() {
    vec2 p = UV - vec2(0.5);
    // 0 at the top, growing clockwise to 1.
    float turn = fract(atan(p.x, -p.y) / 6.2831853);
    float waiting = step(progress, turn) * (1.0 - step(0.9999, progress));
    float edge = (1.0 - smoothstep(0.0, 0.04, abs(turn - progress))) * (1.0 - step(0.9999, progress));
    COLOR.rgb = COLOR.rgb * mix(1.0, 0.3, waiting) + vec3(COLOR.a) * 0.6 * edge;
}
