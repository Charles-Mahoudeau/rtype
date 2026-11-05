/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** instance_input
*/

// The instance values of sprites, glyphs and shapes in their fragment stage, carried from the vertex stage. Only
// declared when the effect has some (RTYPE_INSTANCE_VALUES, defined by the assembler), so the others have no input to
// fill.
#ifdef RTYPE_INSTANCE_VALUES
layout(location = 3) flat in vec4 rtypeInstance[4];
#endif
