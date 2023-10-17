/*
 * Conduit - a project highlighting CLAP-first development
 *           and exercising the surge synth team libraries.
 *
 * Copyright 2023 Paul Walker and authors in github
 *
 * This file you are viewing now is released under the
 * MIT license as described in LICENSE.md
 *
 * The assembled program which results from compiling this
 * project has GPL3 dependencies, so if you distribute
 * a binary, the combined work would be a GPL3 product.
 *
 * Roughly, that means you are welcome to copy the code and
 * ideas in the src/ directory, but perhaps not code from elsewhere
 * if you are closed source or non-GPL3. And if you do copy this code
 * you will need to replace some of the dependencies. Please see
 * the discussion in README.md for further information on what this may
 * mean for you.
 */

#ifndef CONDUIT_SRC_MIDI2_SAWSYNTH_NI_MIDI_PATCHES_H
#define CONDUIT_SRC_MIDI2_SAWSYNTH_NI_MIDI_PATCHES_H

/*
 * These are files which I've submitted as a PR to ni-midi2 but until they merge I stage it here
 * and track the submodule against main
 */

#include <midi/types.h>
#include <midi/universal_packet.h>

#include <cassert>
#include <optional>

namespace midi
{
constexpr bool is_midi2_registered_controller_message(const universal_packet &);
constexpr bool is_midi2_assignable_controller_message(const universal_packet &);
constexpr bool is_midi2_registered_per_note_controller_message(const universal_packet &);
constexpr bool is_midi2_assignable_per_note_controller_message(const universal_packet &);
constexpr bool is_midi2_per_note_pitch_bend_message(const universal_packet &);
constexpr uint8_t get_midi2_per_note_controller_index(const universal_packet &);

constexpr bool is_midi2_registered_controller_message(const universal_packet &p)
{
    return is_midi2_channel_voice_message(p) &&
           (p.status() & 0xF0) == channel_voice_status::registered_controller;
}
constexpr bool is_midi2_assignable_controller_message(const universal_packet &p)
{
    return is_midi2_channel_voice_message(p) &&
           (p.status() & 0xF0) == channel_voice_status::assignable_controller;
}
constexpr bool is_midi2_registered_per_note_controller_message(const universal_packet &p)
{
    return is_midi2_channel_voice_message(p) &&
           (p.status() & 0xF0) == channel_voice_status::registered_per_note_controller;
}
constexpr bool is_midi2_assignable_per_note_controller_message(const universal_packet &p)
{
    return is_midi2_channel_voice_message(p) &&
           (p.status() & 0xF0) == channel_voice_status::assignable_per_note_controller;
}
constexpr bool is_midi2_per_note_pitch_bend_message(const universal_packet &p)
{
    return is_midi2_channel_voice_message(p) &&
           (p.status() & 0xF0) == channel_voice_status::per_note_pitch_bend;
}
constexpr uint8_t get_midi2_per_note_controller_index(const universal_packet &p)
{
    return p.byte4();
}
} // namespace midi
#endif // CONDUIT_NI_MIDI_PATCHES_H
