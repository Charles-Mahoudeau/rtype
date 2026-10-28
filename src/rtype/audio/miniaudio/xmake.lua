-- miniaudio, header-only: one .cpp of this module compiles its implementation (MINIAUDIO_IMPLEMENTATION), after
-- stb_vorbis, the only way miniaudio decodes OGG Vorbis. Encoding is off: the engine only plays sounds.
add_requires("miniaudio 0.11.x", {alias = "miniaudio", configs = {encoding = false, extra_nodes = false}})

-- miniaudio audio backend: IAudioDevice on miniaudio's engine, registered as "miniaudio". The only module that includes
-- miniaudio.
target("audio-miniaudio")
    set_kind("static")
    set_basename("rtype-audio-miniaudio")
    add_deps("core", "plugins")
    -- Private: only this module includes miniaudio.h (MiniaudioDevice.hpp is internal: the executable sees
    -- Registration.hpp only), and only vendor/ includes stb_vorbis.c.
    add_packages("miniaudio", "stb")
    if is_plat("linux", "bsd") then
        -- miniaudio loads the system's audio libraries (PulseAudio, ALSA, JACK) at runtime, and does its maths in libm.
        add_syslinks("dl", "m")
    end
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    -- Headers are included as "audio/miniaudio/...".
    add_includedirs("../..", {public = true})
