#include "graphics_rendition.h"

#include "di/container/vector/vector.h"
#include "di/parser/integral.h"
#include "di/util/clamp.h"
#include "ttx/params.h"

namespace ttx {
// Select Graphics Rendition - https://vt100.net/docs/vt510-rm/SGR.html
//   Modern extensions like underline and true color can be found here:
//     https://wezfurlong.org/wezterm/escape-sequences.html#graphic-rendition-sgr
void GraphicsRendition::update_with_csi_params(Params const& params) {
    for (auto i = 0_usize; i == 0 || i < params.size(); i++) {
        switch (params.get(i, 0)) {
            case 0:
                *this = {};
                break;
            case 1:
                font_weight = FontWeight::Bold;
                break;
            case 2:
                font_weight = FontWeight::Dim;
                break;
            case 3:
                italic = true;
                break;
            case 4:
                underline_mode = UnderlineMode::Normal;
                break;
            case 5:
                blink_mode = BlinkMode::Normal;
                break;
            case 6:
                blink_mode = BlinkMode::Rapid;
                break;
            case 7:
                inverted = true;
                break;
            case 8:
                invisible = true;
                break;
            case 9:
                strike_through = true;
                break;
            case 21:
                underline_mode = UnderlineMode::Double;
                break;
            case 22:
                font_weight = FontWeight::None;
                break;
            case 23:
                italic = false;
                break;
            case 24:
                underline_mode = UnderlineMode::None;
                break;
            case 25:
                blink_mode = BlinkMode::None;
                break;
            case 27:
                inverted = false;
                break;
            case 28:
                invisible = false;
                break;
            case 29:
                strike_through = false;
                break;
            case 30:
                fg = { Color::Palette::Black };
                break;
            case 31:
                fg = { Color::Palette::Red };
                break;
            case 32:
                fg = { Color::Palette::Green };
                break;
            case 33:
                fg = { Color::Palette::Brown };
                break;
            case 34:
                fg = { Color::Palette::Blue };
                break;
            case 35:
                fg = { Color::Palette::Magenta };
                break;
            case 36:
                fg = { Color::Palette::Cyan };
                break;
            case 37:
                fg = { Color::Palette::LightGrey };
                break;
            case 38:
                // Truecolor Foreground (xterm-256color)
                if (params.get(i + 1, 0) != 2) {
                    break;
                }
                if (params.size() - i < 5) {
                    break;
                }
                fg = Color { (uint8_t) di::clamp(params.get(i + 2), 0u, 255u),
                             (uint8_t) di::clamp(params.get(i + 3), 0u, 255u),
                             (uint8_t) di::clamp(params.get(i + 4), 0u, 255u) };
                i += 4;
                break;
            case 39:
                fg = {};
                break;
            case 40:
                bg = { Color::Palette::Black };
                break;
            case 41:
                bg = { Color::Palette::Red };
                break;
            case 42:
                bg = { Color::Palette::Green };
                break;
            case 43:
                bg = { Color::Palette::Brown };
                break;
            case 44:
                bg = { Color::Palette::Blue };
                break;
            case 45:
                bg = { Color::Palette::Magenta };
                break;
            case 46:
                bg = { Color::Palette::Cyan };
                break;
            case 47:
                bg = { Color::Palette::LightGrey };
                break;
            case 48:
                // Truecolor Background (xterm-256color)
                if (params.get(i + 1, 0) != 2) {
                    break;
                }
                if (params.size() - i < 5) {
                    break;
                }
                bg = Color { (uint8_t) di::clamp(params.get(i + 2), 0u, 255u),
                             (uint8_t) di::clamp(params.get(i + 3), 0u, 255u),
                             (uint8_t) di::clamp(params.get(i + 4), 0u, 255u) };
                i += 4;
                break;
            case 49:
                bg = {};
                break;
            case 53:
                overline = true;
            case 55:
                overline = false;
                break;
            case 58:
                // Truecolor Underine Color (xterm-256color)
                if (params.get(i + 1, 0) != 2) {
                    break;
                }
                if (params.size() - i < 5) {
                    break;
                }
                underline_color = Color { (uint8_t) di::clamp(params.get(i + 2), 0u, 255u),
                                          (uint8_t) di::clamp(params.get(i + 3), 0u, 255u),
                                          (uint8_t) di::clamp(params.get(i + 4), 0u, 255u) };
                i += 4;
                break;
                break;
            case 59:
                underline_color = {};
                break;
            case 90:
                fg = { Color::Palette::DarkGrey };
                break;
            case 91:
                fg = { Color::Palette::LightRed };
                break;
            case 92:
                fg = { Color::Palette::LightGreen };
                break;
            case 93:
                fg = { Color::Palette::Yellow };
                break;
            case 94:
                fg = { Color::Palette::LightBlue };
                break;
            case 95:
                fg = { Color::Palette::LightMagenta };
                break;
            case 96:
                fg = { Color::Palette::LightCyan };
                break;
            case 97:
                fg = { Color::Palette::White };
                break;
            case 100:
                bg = { Color::Palette::DarkGrey };
                break;
            case 101:
                bg = { Color::Palette::LightRed };
                break;
            case 102:
                bg = { Color::Palette::LightGreen };
                break;
            case 103:
                bg = { Color::Palette::Yellow };
                break;
            case 104:
                bg = { Color::Palette::LightBlue };
                break;
            case 105:
                bg = { Color::Palette::LightMagenta };
                break;
            case 106:
                bg = { Color::Palette::LightCyan };
                break;
            case 107:
                bg = { Color::Palette::White };
                break;
            default:
                break;
        }
    }
}

enum class ColorType {
    Fg,
    Bg,
    Underine,
};

static auto color_to_subparams(Color c, ColorType type) -> di::Vector<u32> {
    if (c.c == Color::Palette::Custom) {
        auto code = type == ColorType::Fg ? 38u : type == ColorType::Bg ? 48u : 58u;
        return { code, 2, c.r, c.g, c.b };
    } else if (type == ColorType::Underine) {
        // Use palette index.
        return { 58, 8, u32(c.c - Color::Palette::Black) };
    } else if (c.c <= Color::Palette::LightGrey) {
        auto base_value = type == ColorType::Fg ? 30u : 40u;
        return { base_value + c.c - Color::Palette::Black };
    } else {
        auto base_value = type == ColorType::Fg ? 90u : 100u;
        return { base_value + c.c - Color::Palette::DarkGrey };
    }
}

auto GraphicsRendition::as_csi_params() const -> di::String {
    // Start by clearing all attributes.
    auto sgr = Params();
    sgr.add_param(0);

    switch (font_weight) {
        case FontWeight::Bold:
            sgr.add_param(1);
            break;
        case FontWeight::Dim:
            sgr.add_param(2);
            break;
        case FontWeight::None:
            break;
    }
    if (italic) {
        sgr.add_param(3);
    }
    switch (underline_mode) {
        case UnderlineMode::Normal:
            sgr.add_param(4);
            break;
        case UnderlineMode::Double:
            sgr.add_param(21);
            break;
        case UnderlineMode::Curly:
            sgr.add_subparams({ 4, 3 });
            break;
        case UnderlineMode::Dotted:
            sgr.add_subparams({ 4, 4 });
            break;
        case UnderlineMode::Dashed:
            sgr.add_subparams({ 4, 5 });
            break;
        case UnderlineMode::None:
            break;
    }
    switch (blink_mode) {
        case BlinkMode::Normal:
            sgr.add_param(5);
            break;
        case BlinkMode::Rapid:
            sgr.add_param(6);
            break;
        case BlinkMode::None:
            break;
    }
    if (inverted) {
        sgr.add_param(7);
    }
    if (invisible) {
        sgr.add_param(8);
    }
    if (strike_through) {
        sgr.add_param(9);
    }
    if (overline) {
        sgr.add_param(53);
    }
    if (fg.c != Color::None) {
        sgr.add_subparams(color_to_subparams(fg, ColorType::Fg));
    }
    if (bg.c != Color::None) {
        sgr.add_subparams(color_to_subparams(bg, ColorType::Bg));
    }
    if (underline_color.c != Color::None) {
        sgr.add_subparams(color_to_subparams(underline_color, ColorType::Underine));
    }
    return sgr.to_string();
}
}
