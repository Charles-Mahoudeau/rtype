/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** sprite.frag
*/

// The fragment stage of sprites and particles: the texture times the colour, both premultiplied. Without a shader, the
// default shader of sprites and particles.
#pragma rtype_shader

void main() {
    UV = rtypeUvIn;
    COLOR = texture(TEXTURE, UV) * rtypeColorIn;
#pragma rtype_main
    rtypeFragColor = rtypeFinishBlend(COLOR, rtypeBlendIn & rtypeBlendMask);
}
