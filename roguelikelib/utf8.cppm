//////////////////////////////////////////////////////////////////////////
// UTF-8: glyphs to text for the terminal, and text or keyboard input back
// to glyphs
//////////////////////////////////////////////////////////////////////////

module;

export module rl.utf8;

import std;

export namespace RL
{

// What malformed input decodes to, and invalid code points encode as
constexpr char32_t replacement_character = U'�';

// Encode a glyph as UTF-8, for printing it to a terminal. Anything that
// is not a valid code point comes out as the replacement character.
std::string EncodeUTF8(char32_t glyph)
{
    if (glyph > 0x10FFFF || (glyph >= 0xD800 && glyph <= 0xDFFF)) {
        glyph = replacement_character;
    }

    std::string utf8;

    if (glyph < 0x80) {
        utf8 += static_cast<char>(glyph);
    } else if (glyph < 0x800) {
        utf8 += static_cast<char>(0xC0 | (glyph >> 6));
        utf8 += static_cast<char>(0x80 | (glyph & 0x3F));
    } else if (glyph < 0x10000) {
        utf8 += static_cast<char>(0xE0 | (glyph >> 12));
        utf8 += static_cast<char>(0x80 | ((glyph >> 6) & 0x3F));
        utf8 += static_cast<char>(0x80 | (glyph & 0x3F));
    } else {
        utf8 += static_cast<char>(0xF0 | (glyph >> 18));
        utf8 += static_cast<char>(0x80 | ((glyph >> 12) & 0x3F));
        utf8 += static_cast<char>(0x80 | ((glyph >> 6) & 0x3F));
        utf8 += static_cast<char>(0x80 | (glyph & 0x3F));
    }

    return utf8;
}

struct SDecodedUTF8 {
    char32_t code_point = replacement_character;

    // Bytes taken from the input
    std::size_t length = 0;

    // False when the input stops in the middle of a valid sequence: more
    // bytes may still be on their way, as they can be from a keyboard.
    bool complete = true;
};

// Decodes the code point at the start of a non-empty text. Malformed input
// becomes the replacement character, one byte at a time, so that decoding
// always makes progress and resynchronises on the next lead byte.
SDecodedUTF8 DecodeUTF8(std::string_view text)
{
    const auto byte = [&text](std::size_t index) {
        return static_cast<unsigned char>(text[index]);
    };
    const auto invalid = SDecodedUTF8{replacement_character, 1, true};
    const unsigned char lead = byte(0);

    if (lead < 0x80) {
        return {lead, 1, true};
    }

    std::size_t length = 0;
    char32_t code_point = 0;
    char32_t lowest = 0;

    if (lead >= 0xC2 && lead <= 0xDF) {
        length = 2;
        code_point = lead & 0x1F;
        lowest = 0x80;
    } else if (lead >= 0xE0 && lead <= 0xEF) {
        length = 3;
        code_point = lead & 0x0F;
        lowest = 0x800;
    } else if (lead >= 0xF0 && lead <= 0xF4) {
        length = 4;
        code_point = lead & 0x07;
        lowest = 0x10000;
    } else {
        // A continuation byte without a lead, or a lead no valid sequence
        // starts with
        return invalid;
    }

    const std::size_t available = std::min(length, text.size());

    for (std::size_t index = 1; index < available; ++index) {
        if ((byte(index) & 0xC0) != 0x80) {
            return invalid;
        }

        code_point = (code_point << 6) | (byte(index) & 0x3F);
    }

    if (available < length) {
        return {replacement_character, available, false};
    }

    // Overlong encodings, UTF-16 surrogates and anything past Unicode
    if (code_point < lowest || (code_point >= 0xD800 && code_point <= 0xDFFF) || code_point > 0x10FFFF) {
        return invalid;
    }

    return {code_point, length, true};
}

} // namespace RL
