// The beams' glow: a faint white core along the middle of the beam's width, fading to its edges, pulsing like a laser.
// UV runs across the square whatever the way it is turned, so the core follows the beam.
uniform float core = 0.25;   // how bright the white core gets, at the top of a pulse
uniform float pulses = 25.0; // radians per second of the pulse

void fragment() {
    float pulse = 0.75 + (0.25 * sin(TIME * pulses));
    float across = 1.0 - (abs(UV.y - 0.5) * 2.0); // 1 in the middle of the beam's width, 0 at its edges
    COLOR.rgb += vec3(1.0) * (across * across) * core * pulse * COLOR.a; // COLOR is premultiplied
}
