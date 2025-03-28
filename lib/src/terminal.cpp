#include "ttx/terminal.h"

#include "di/container/algorithm/contains.h"
#include "di/container/algorithm/for_each.h"
#include "di/container/algorithm/rotate.h"
#include "di/io/vector_writer.h"
#include "di/io/writer_print.h"
#include "di/serialization/base64.h"
#include "di/util/construct.h"
#include "dius/tty.h"
#include "ttx/cursor_style.h"
#include "ttx/escape_sequence_parser.h"
#include "ttx/focus_event_io.h"
#include "ttx/graphics_rendition.h"
#include "ttx/key_event_io.h"
#include "ttx/mouse_event_io.h"
#include "ttx/params.h"
#include "ttx/paste_event_io.h"

namespace ttx {
void Terminal::on_parser_results(di::Span<ParserResult const> results) {
    for (auto const& result : results) {
        di::visit(
            [&](auto const& r) {
                this->on_parser_result(r);
            },
            result);
    }
}

void Terminal::on_parser_result(PrintableCharacter const& Printable_character) {
    if (Printable_character.code_point < 0x7F || Printable_character.code_point > 0x9F) {
        put_char(Printable_character.code_point);
    }
}

void Terminal::on_parser_result(DCS const& dcs) {
    if (dcs.intermediate == "$q"_sv) {
        dcs_decrqss(dcs.params, dcs.data);
        return;
    }
}

void Terminal::on_parser_result(OSC const& osc) {
    auto ps_end = osc.data.find(';');
    if (!ps_end) {
        return;
    }

    auto ps = osc.data.substr(osc.data.begin(), ps_end.begin());
    if (ps == "52"_sv) {
        osc_52(osc.data.substr(ps_end.end()));
        return;
    }
}

void Terminal::on_parser_result(APC const&) {}

void Terminal::on_parser_result(ControlCharacter const& control_character) {
    switch (control_character.code_point) {
        case 8: {
            c0_bs();
            return;
        }
        case '\a':
            return;
        case '\t': {
            c0_ht();
            return;
        }
        case '\n': {
            c0_lf();
            return;
        }
        case '\v': {
            c0_vt();
            return;
        }
        case '\f': {
            c0_ff();
            return;
        }
        case '\r': {
            c0_cr();
            return;
        }
        default:
            return;
    }
}

void Terminal::on_parser_result(CSI const& csi) {
    if (csi.intermediate == "?$"_sv) {
        switch (csi.terminator) {
            case 'p': {
                csi_decrqm(csi.params);
                return;
            }
            default:
                return;
        }
    }

    if (csi.intermediate == "="_sv) {
        switch (csi.terminator) {
            case 'c': {
                csi_da3(csi.params);
                return;
            }
            case 'u': {
                csi_set_key_reporting_flags(csi.params);
                return;
            }
            default:
                return;
        }
    }

    if (csi.intermediate == ">"_sv) {
        switch (csi.terminator) {
            case 'c': {
                csi_da2(csi.params);
                return;
            }
            case 'u': {
                csi_push_key_reporting_flags(csi.params);
                return;
            }
            default:
                return;
        }
    }

    if (csi.intermediate == "<"_sv) {
        switch (csi.terminator) {
            case 'u': {
                csi_pop_key_reporting_flags(csi.params);
                return;
            }
            default:
                return;
        }
    }

    if (csi.intermediate == "?"_sv) {
        switch (csi.terminator) {
            case 'h': {
                csi_decset(csi.params);
                return;
            }
            case 'l': {
                csi_decrst(csi.params);
                return;
            }
            case 'u': {
                csi_get_key_reporting_flags(csi.params);
                return;
            }
            default:
                return;
        }
    }

    if (csi.intermediate == " "_sv) {
        switch (csi.terminator) {
            case 'q': {
                csi_decscusr(csi.params);
                return;
            }
            default:
                return;
        }
    }

    if (!csi.intermediate.empty()) {
        return;
    }

    switch (csi.terminator) {
        case '@': {
            csi_ich(csi.params);
            return;
        }
        case 'A': {
            csi_cuu(csi.params);
            return;
        }
        case 'B': {
            csi_cud(csi.params);
            return;
        }
        case 'C': {
            csi_cuf(csi.params);
            return;
        }
        case 'D': {
            csi_cub(csi.params);
            return;
        }
        case 'G': {
            csi_cha(csi.params);
            return;
        }
        case 'H': {
            csi_cup(csi.params);
            return;
        }
        case 'J': {
            csi_ed(csi.params);
            return;
        }
        case 'K': {
            csi_el(csi.params);
            return;
        }
        case 'L': {
            csi_il(csi.params);
            return;
        }
        case 'M': {
            csi_dl(csi.params);
            return;
        }
        case 'P': {
            csi_dch(csi.params);
            return;
        }
        case 'S': {
            csi_su(csi.params);
            return;
        }
        case 'T': {
            csi_sd(csi.params);
            return;
        }
        case 'X': {
            csi_ech(csi.params);
            return;
        }
        case 'b': {
            csi_rep(csi.params);
            return;
        }
        case 'c': {
            csi_da1(csi.params);
            return;
        }
        case 'd': {
            csi_vpa(csi.params);
            return;
        }
        case 'f': {
            csi_hvp(csi.params);
            return;
        }
        case 'g': {
            csi_tbc(csi.params);
            return;
        }
        case 'm': {
            csi_sgr(csi.params);
            return;
        }
        case 'n': {
            csi_dsr(csi.params);
            return;
        }
        case 'r': {
            csi_decstbm(csi.params);
            return;
        }
        case 's': {
            csi_scosc(csi.params);
            return;
        }
        case 't': {
            csi_xtwinops(csi.params);
            return;
        }
        case 'u': {
            csi_scorc(csi.params);
            return;
        }
        default:
            return;
    }
}

void Terminal::on_parser_result(Escape const& escape) {
    if (escape.intermediate == "#"_sv) {
        switch (escape.terminator) {
            case '8': {
                esc_decaln();
                return;
            }
            default:
                return;
        }
    }

    if (!escape.intermediate.empty()) {
        return;
    }

    switch (escape.terminator) {
        case '7': {
            esc_decsc();
            return;
        }
        case '8': {
            esc_decrc();
            return;
        }
        // 8 bit control characters
        case 'D': {
            c1_ind();
            return;
        }
        case 'E': {
            c1_nel();
            return;
        }
        case 'H': {
            c1_hts();
            return;
        }
        case 'M': {
            c1_ri();
            return;
        }
        default:
            return;
    }
}

// Backspace - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_bs() {
    if (m_cursor_col) {
        m_cursor_col--;
    }
    m_x_overflow = false;
}

// Horizontal Tab - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_ht() {
    for (auto tab_stop : m_tab_stops) {
        if (tab_stop > m_cursor_col) {
            set_cursor(m_cursor_row, tab_stop);
            return;
        }
    }

    set_cursor(m_cursor_row, max_col_inclusive());
}

// Line Feed - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_lf() {
    m_cursor_row++;
    scroll_down_if_needed();
    m_x_overflow = false;
}

// Vertical Tab - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_vt() {
    c0_lf();
}

// Form Feed - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_ff() {
    c0_lf();
}

// Carriage Return - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_cr() {
    m_cursor_col = 0;
    m_x_overflow = false;
}

// Index - https://vt100.net/docs/vt510-rm/IND.html
void Terminal::c1_ind() {
    m_cursor_row++;
    m_x_overflow = false;
    scroll_down_if_needed();
}

// Next Line - https://vt100.net/docs/vt510-rm/NEL.html
void Terminal::c1_nel() {
    m_cursor_row++;
    m_cursor_col = 0;
    m_x_overflow = false;
    scroll_down_if_needed();
}

// Horizontal Tab Set - https://vt100.net/docs/vt510-rm/HTS.html
void Terminal::c1_hts() {
    if (di::contains(m_tab_stops, m_cursor_col)) {
        return;
    }

    auto index = 0_usize;
    for (; index < m_tab_stops.size(); index++) {
        if (m_cursor_col < m_tab_stops[index]) {
            break;
        }
    }

    m_tab_stops.insert(m_tab_stops.begin() + index, m_cursor_col);
}

// Reverse Index - https://www.vt100.net/docs/vt100-ug/chapter3.html#RI
void Terminal::c1_ri() {
    m_cursor_row--;
    m_x_overflow = false;
    scroll_up_if_needed();
}

// Request Status String - https://vt100.net/docs/vt510-rm/DECRQSS.html
void Terminal::dcs_decrqss(Params const&, di::StringView data) {
    // Set graphics rendition
    if (data == "m"_sv) {
        auto sgr_string = m_current_graphics_rendition.as_csi_params() | di::transform(di::to_string) |
                          di::join_with(U';') | di::to<di::String>();
        (void) m_psuedo_terminal.write_exactly(di::as_bytes(di::present("\033P1$r{}m\033\\"_sv, sgr_string)->span()));
    } else {
        (void) m_psuedo_terminal.write_exactly(di::as_bytes("\033P0$r\033\\"_sv.span()));
    }
}

// OSC 52 - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-Operating-System-Commands
void Terminal::osc_52(di::StringView data) {
    // Data is of the form: Pc ; Pd
    auto pc_end = data.find(';');
    if (!pc_end) {
        return;
    }

    // For now, just ignore which selection is asked for (the Pc field).
    auto pd = data.substr(pc_end.end());
    if (pd == "?"_sv) {
        // TODO: respond with the actual clipboard contents.
        return;
    }

    auto clipboard_data = di::parse<di::Base64<>>(pd);
    if (!clipboard_data) {
        return;
    }

    m_outgoing_events.emplace_back(SetClipboard(di::move(clipboard_data).value().container()));
}

// DEC Screen Alignment Pattern - https://vt100.net/docs/vt510-rm/DECALN.html
void Terminal::esc_decaln() {
    clear('E');
    set_cursor(0, 0);
    m_x_overflow = false;
}

// DEC Save Cursor - https://vt100.net/docs/vt510-rm/DECSC.html
void Terminal::esc_decsc() {
    save_pos();
}

// DEC Restore Cursor - https://vt100.net/docs/vt510-rm/DECRC.html
void Terminal::esc_decrc() {
    restore_pos();
}

// Insert Character - https://vt100.net/docs/vt510-rm/ICH.html
void Terminal::csi_ich(Params const& params) {
    auto chars = di::max(1u, params.get(0, 1));

    auto& row = m_rows[m_cursor_row];
    for (u32 i = max_col_inclusive() - chars; i >= m_cursor_col; i--) {
        row[i + chars] = row[i];
    }

    for (u32 i = m_cursor_col; i <= max_col_inclusive() && i < m_cursor_col + u32(chars); i++) {
        row[i] = {};
    }
}

// Cursor Up - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUU
void Terminal::csi_cuu(Params const& params) {
    auto delta_row = di::max(1u, params.get(0, 1));
    set_cursor(m_cursor_row - delta_row, m_cursor_col);
}

// Cursor Down - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUD
void Terminal::csi_cud(Params const& params) {
    auto delta_row = di::max(1u, params.get(0, 1));
    set_cursor(m_cursor_row + delta_row, m_cursor_col);
}

// Cursor Forward - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUF
void Terminal::csi_cuf(Params const& params) {
    auto delta_col = di::max(1u, params.get(0, 1));
    set_cursor(m_cursor_row, m_cursor_col + delta_col);
}

// Cursor Backward - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUB
void Terminal::csi_cub(Params const& params) {
    auto delta_col = di::max(1u, params.get(0, 1));
    set_cursor(m_cursor_row, m_cursor_col - delta_col);
}

// Cursor Position - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUP
void Terminal::csi_cup(Params const& params) {
    auto row = translate_row(params.get(0, 1));
    auto col = translate_col(params.get(1, 1));
    set_cursor(row, col);
}

// Cursor Horizontal Absolute - https://vt100.net/docs/vt510-rm/CHA.html
void Terminal::csi_cha(Params const& params) {
    set_cursor(m_cursor_row, translate_col(params.get(0, 1)));
}

// Erase in Display - https://vt100.net/docs/vt510-rm/ED.html
void Terminal::csi_ed(Params const& params) {
    switch (params.get(0, 0)) {
        case 0: {
            clear_below_cursor();
            return;
        }
        case 1: {
            clear_above_cursor();
            return;
        }
        case 2: {
            clear();
            return;
        }
        case 3:
            // XTerm extension to clear scoll buffer
            m_rows_above.clear();
            m_rows_below.clear();
            m_rows.resize(m_row_count);
            clear();
            return;
        default:
            return;
    }
}

// Erase in Line - https://vt100.net/docs/vt510-rm/EL.html
void Terminal::csi_el(Params const& params) {
    switch (params.get(0, 0)) {
        case 0: {
            clear_row_to_end(m_cursor_row, m_cursor_col);
            return;
        }
        case 1: {
            clear_row_until(m_cursor_row, m_cursor_col);
            return;
        }
        case 2: {
            clear_row(m_cursor_row);
            return;
        }
        default:
            return;
    }
}

// Insert Line - https://vt100.net/docs/vt510-rm/IL.html
void Terminal::csi_il(Params const& params) {
    if (m_cursor_row < m_scroll_start || m_cursor_row > m_scroll_end) {
        return;
    }
    u32 lines_to_insert = di::max(1u, params.get(0, 1));
    for (u32 i = 0; i < lines_to_insert; i++) {
        di::rotate(m_rows.begin() + m_cursor_row, m_rows.begin() + m_scroll_end, m_rows.begin() + m_scroll_end + 1);
        m_rows[m_cursor_row] = Row();
        m_rows[m_cursor_row].resize(m_col_count);
    }
    invalidate_all();
}

// Delete Line - https://vt100.net/docs/vt510-rm/DL.html
void Terminal::csi_dl(Params const& params) {
    if (m_cursor_row < m_scroll_start || m_cursor_row > m_scroll_end) {
        return;
    }
    u32 lines_to_delete = di::clamp(params.get(0, 1), 1u, (m_scroll_end - m_cursor_row));
    for (u32 i = 0; i < lines_to_delete; i++) {
        di::rotate(m_rows.begin() + m_cursor_row, m_rows.begin() + m_cursor_row + 1, m_rows.begin() + m_scroll_end + 1);
        m_rows[m_scroll_end] = Row();
        m_rows[m_scroll_end].resize(m_col_count);
    }

    invalidate_all();
}

// Delete Character - https://vt100.net/docs/vt510-rm/DCH.html
void Terminal::csi_dch(Params const& params) {
    u32 chars_to_delete = di::clamp(params.get(0, 1), 1u, (m_col_count - m_cursor_col));
    for (u32 i = 0; i < chars_to_delete; i++) {
        m_rows[m_cursor_row].erase(m_rows[m_cursor_row].begin() + m_cursor_col);
    }
    m_rows[m_cursor_row].resize(m_col_count);
    for (u32 i = m_cursor_col; i < m_col_count; i++) {
        m_rows[m_cursor_row][i].dirty = true;
    }
}

// Pan Down - https://vt100.net/docs/vt510-rm/SU.html
void Terminal::csi_su(Params const& params) {
    u32 to_scroll = params.get(0, 1);
    u32 row_save = m_cursor_row;
    for (u32 i = 0; i < to_scroll; i++) {
        m_cursor_row = m_row_count;
        scroll_down_if_needed();
    }
    m_cursor_row = row_save;
}

// Pan Up - https://vt100.net/docs/vt510-rm/SD.html
void Terminal::csi_sd(Params const& params) {
    u32 to_scroll = params.get(0, 1);
    u32 row_save = m_cursor_row;
    for (u32 i = 0; i < to_scroll; i++) {
        m_cursor_row = -1;
        scroll_up_if_needed();
    }
    m_cursor_row = row_save;
}

// Erase Character - https://vt100.net/docs/vt510-rm/ECH.html
void Terminal::csi_ech(Params const& params) {
    u32 chars_to_erase = di::max(1u, params.get(0, 1));
    for (u32 i = m_cursor_col; i - m_cursor_col < chars_to_erase && i < m_col_count; i++) {
        m_rows[m_cursor_row][i] = {};
    }
}

// Repeat Preceding Graphic Character - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
void Terminal::csi_rep(Params const& params) {
    c32 preceding_character = ' ';
    if (m_cursor_col == 0) {
        if (m_cursor_row != 0) {
            preceding_character = m_rows[m_cursor_row - 1][m_col_count - 1].ch;
        }
    } else {
        preceding_character = m_rows[m_cursor_row][m_cursor_col - 1].ch;
    }
    for (auto i = 0_u32; i < params.get(0, 0); i++) {
        put_char(preceding_character);
    }
}

// Primary Device Attributes - https://vt100.net/docs/vt510-rm/DA1.html
void Terminal::csi_da1(Params const& params) {
    if (params.get(0, 0) != 0) {
        return;
    }
    (void) m_psuedo_terminal.write_exactly(di::as_bytes("\033[?1;0c"_sv.span()));
}

// Secondary Device Attributes - https://vt100.net/docs/vt510-rm/DA2.html
void Terminal::csi_da2(Params const& params) {
    if (params.get(0, 0) != 0) {
        return;
    }
    (void) m_psuedo_terminal.write_exactly(di::as_bytes("\033[>010;0c"_sv.span()));
}

// Tertiary Device Attributes - https://vt100.net/docs/vt510-rm/DA3.html
void Terminal::csi_da3(Params const& params) {
    if (params.get(0, 0) != 0) {
        return;
    }
    (void) m_psuedo_terminal.write_exactly(di::as_bytes("\033P!|00000000\033\\"_sv.span()));
}

// Vertical Line Position Absolute - https://vt100.net/docs/vt510-rm/VPA.html
void Terminal::csi_vpa(Params const& params) {
    set_cursor(translate_row(params.get(0, 1)), m_cursor_col);
}

// Horizontal and Vertical Position - https://vt100.net/docs/vt510-rm/HVP.html
void Terminal::csi_hvp(Params const& params) {
    csi_cup(params);
}

// Tab Clear - https://vt100.net/docs/vt510-rm/TBC.html
void Terminal::csi_tbc(Params const& params) {
    switch (params.get(0, 0)) {
        case 0:
            di::erase_if(m_tab_stops, [this](auto x) {
                return x == m_cursor_col;
            });
            return;
        case 3:
            m_tab_stops.clear();
            return;
        default:
            return;
    }
}

// DEC Private Mode Set - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
void Terminal::csi_decset(Params const& params) {
    switch (params.get(0, 0)) {
        case 1:
            // Cursor Keys Mode - https://vt100.net/docs/vt510-rm/DECCKM.html
            m_application_cursor_keys_mode = ApplicationCursorKeysMode::Enabled;
            break;
        case 3:
            // Select 80 or 132 Columns per Page - https://vt100.net/docs/vt510-rm/DECCOLM.html
            if (m_allow_80_132_col_mode) {
                m_80_col_mode = false;
                m_132_col_mode = true;
                resize({ m_row_count, 132, m_available_xpixels_in_display * 132 / m_available_cols_in_display,
                         m_ypixels });
                clear();
                csi_decstbm({});
            }
            break;
        case 6:
            // Origin Mode - https://vt100.net/docs/vt510-rm/DECOM.html
            m_origin_mode = true;
            set_cursor(m_cursor_row, m_cursor_col);
            break;
        case 7:
            // Autowrap mode - https://vt100.net/docs/vt510-rm/DECAWM.html
            m_autowrap_mode = true;
            break;
        case 9:
            m_mouse_protocol = MouseProtocol::X10;
            break;
        case 25:
            // Text Cursor Enable Mode - https://vt100.net/docs/vt510-rm/DECTCEM.html
            m_cursor_hidden = false;
            break;
        case 40:
            m_allow_80_132_col_mode = true;
            break;
        case 1000:
            m_mouse_protocol = MouseProtocol::VT200;
            break;
        case 1002:
            m_mouse_protocol = MouseProtocol::BtnEvent;
            break;
        case 1003:
            m_mouse_protocol = MouseProtocol::AnyEvent;
            break;
        case 1004:
            m_focus_event_mode = FocusEventMode::Enabled;
            break;
        case 1005:
            m_mouse_encoding = MouseEncoding::UTF8;
            break;
        case 1006:
            m_mouse_encoding = MouseEncoding::SGR;
            break;
        case 1015:
            m_mouse_encoding = MouseEncoding::URXVT;
            break;
        case 1016:
            m_mouse_encoding = MouseEncoding::SGRPixels;
            break;
        case 1049:
            set_use_alternate_screen_buffer(true);
            break;
        case 2004:
            m_bracketed_paste_mode = BracketedPasteMode::Enabled;
            break;
        case 2026:
            m_disable_drawing = true;
            break;
        default:
            break;
    }
}

// DEC Private Mode Reset - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
void Terminal::csi_decrst(Params const& params) {
    switch (params.get(0, 0)) {
        case 1:
            // Cursor Keys Mode - https://vt100.net/docs/vt510-rm/DECCKM.html
            m_application_cursor_keys_mode = ApplicationCursorKeysMode::Disabled;
            break;
        case 3:
            // Select 80 or 132 Columns per Page - https://vt100.net/docs/vt510-rm/DECCOLM.html
            if (m_allow_80_132_col_mode) {
                m_80_col_mode = true;
                m_132_col_mode = false;
                resize(
                    { m_row_count, 80, m_available_xpixels_in_display * 80 / m_available_cols_in_display, m_ypixels });
                clear();
                csi_decstbm({});
            }
            break;
        case 6:
            // Origin Mode - https://vt100.net/docs/vt510-rm/DECOM.html
            m_origin_mode = false;
            break;
        case 7:
            // Autowrap mode - https://vt100.net/docs/vt510-rm/DECAWM.html
            m_autowrap_mode = false;
            break;
        case 9:
            m_mouse_protocol = MouseProtocol::None;
            break;
        case 25:
            // Text Cursor Enable Mode - https://vt100.net/docs/vt510-rm/DECTCEM.html
            m_cursor_hidden = true;
            break;
        case 40:
            m_allow_80_132_col_mode = false;
            if (m_80_col_mode || m_132_col_mode) {
                m_80_col_mode = false;
                m_132_col_mode = false;
                resize(visible_size());
            }
            break;
        case 1000:
        case 1002:
        case 1003:
            m_mouse_protocol = MouseProtocol::None;
            break;
        case 1004:
            m_focus_event_mode = FocusEventMode::Disabled;
            break;
        case 1005:
        case 1006:
        case 1015:
        case 1016:
            m_mouse_encoding = MouseEncoding::X10;
            break;
        case 1049:
            set_use_alternate_screen_buffer(false);
            break;
        case 2004:
            m_bracketed_paste_mode = BracketedPasteMode::Disabled;
            break;
        case 2026:
            m_disable_drawing = false;
            break;
        default:
            break;
    }
}

// Request Mode - Host to Terminal - https://vt100.net/docs/vt510-rm/DECRQM.html
void Terminal::csi_decrqm(Params const& params) {
    auto param = params.get(0, 0);
    switch (param) {
        // Synchronized output - https://gist.github.com/christianparpart/d8a62cc1ab659194337d73e399004036
        case 2026:
            (void) m_psuedo_terminal.write_exactly(
                di::as_bytes(di::present("\033[?{};{}$y"_sv, param, m_disable_drawing ? 1 : 2)->span()));
            break;
        default:
            (void) m_psuedo_terminal.write_exactly(di::as_bytes(di::present("\033[?{};0$y"_sv, param)->span()));
    }
}

// Set Cursor Style - https://vt100.net/docs/vt510-rm/DECSCUSR.html
void Terminal::csi_decscusr(Params const& params) {
    auto param = params.get(0, 0);
    // 0 and 1 indicate the same style.
    if (param == 0) {
        param = 1;
    }
    if (param >= u32(CursorStyle::Max)) {
        return;
    }
    m_cursor_style = CursorStyle(param);
}

// Select Graphics Rendition - https://vt100.net/docs/vt510-rm/SGR.html
void Terminal::csi_sgr(Params const& params) {
    // Delegate to graphics rendition class.
    m_current_graphics_rendition.update_with_csi_params(params);
}

// Device Status Report - https://vt100.net/docs/vt510-rm/DSR.html
void Terminal::csi_dsr(Params const& params) {
    switch (params.get(0, 0)) {
        case 5:
            // Operating Status - https://vt100.net/docs/vt510-rm/DSR-OS.html
            (void) m_psuedo_terminal.write_exactly(di::as_bytes("\033[0n"_sv.span()));
            break;
        case 6:
            // Cursor Position Report - https://vt100.net/docs/vt510-rm/DSR-CPR.html
            (void) m_psuedo_terminal.write_exactly(
                di::as_bytes(di::present("\033[{};{}R"_sv, m_cursor_row + 1, m_cursor_col + 1).value().span()));
            break;
        default:
            break;
    }
}

// DEC Set Top and Bottom Margins - https://www.vt100.net/docs/vt100-ug/chapter3.html#DECSTBM
void Terminal::csi_decstbm(Params const& params) {
    u32 new_scroll_start = di::min(params.get(0, 1) - 1, m_row_count - 1);
    u32 new_scroll_end = di::min(params.get(1, m_row_count) - 1, m_row_count - 1);
    if (new_scroll_end - new_scroll_start < 1) {
        return;
    }
    m_scroll_start = new_scroll_start;
    m_scroll_end = new_scroll_end;
    set_cursor(0, 0);
}

// Save Current Cursor Position - https://vt100.net/docs/vt510-rm/SCOSC.html
void Terminal::csi_scosc(Params const&) {
    save_pos();
}

// Restore Saved Cursor Position - https://vt100.net/docs/vt510-rm/SCORC.html
void Terminal::csi_scorc(Params const&) {
    restore_pos();
}

// Window manipulation -
// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h4-Functions-using-CSI-_-ordered-by-the-final-character-lparen-s-rparen:CSI-Ps;Ps;Ps-t.1EB0
void Terminal::csi_xtwinops(Params const& params) {
    auto command = params.get(0);
    switch (command) {
        case 4: {
            if (!m_allow_force_terminal_size) {
                break;
            }

            // This could also set the width and height as based on the ratio of pixels to cells,
            // but we skip this for now. This command is used for testing ttx (forcing a specific
            // size), but does not change the visible size of the terminal itself, which is already
            // constrained by the layout.
            auto height = di::min(params.get(1, m_ypixels), 100000u);
            auto width = di::min(params.get(2, m_xpixels), 100000u);
            if (height == 0) {
                height = m_available_ypixels_in_display;
            }
            if (width == 0) {
                width = m_available_xpixels_in_display;
            }
            m_ypixels = height;
            m_xpixels = width;
            break;
        }
        case 8: {
            if (!m_allow_force_terminal_size) {
                break;
            }

            // This logic is similar to DECSET 3 - 80/132 column mode, in that we don't actually resize the terminal's
            // visible area. This only resizes the terminal's internal size, which is useful for facilitating testing
            // or if the application requires the terminal to be a certain size.
            auto rows = di::min(params.get(1, m_row_count), 1000u);
            auto cols = di::min(params.get(2, m_col_count), 1000u);
            m_force_terminal_size = rows != 0 || cols != 0;
            if (rows == 0) {
                rows = m_available_rows_in_display;
            }
            if (cols == 0) {
                cols = m_available_cols_in_display;
            }
            resize({ rows, cols, m_xpixels, m_ypixels });
            clear();
            csi_decstbm({});
            break;
        }
        default:
            break;
    }
}

// https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement
void Terminal::csi_set_key_reporting_flags(Params const& params) {
    auto flags_u32 = params.get(0);
    auto mode = params.get(1, 1);

    auto flags = KeyReportingFlags(flags_u32) & KeyReportingFlags::All;
    switch (mode) {
        case 1:
            m_key_reporting_flags = flags;
            break;
        case 2:
            m_key_reporting_flags |= flags;
            break;
        case 3:
            m_key_reporting_flags &= ~flags;
            break;
        default:
            break;
    }
}

// https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement
void Terminal::csi_get_key_reporting_flags(Params const&) {
    (void) m_psuedo_terminal.write_exactly(
        di::as_bytes(di::present("\033[?{}u"_sv, u32(m_key_reporting_flags)).value().span()));
}

// https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement
void Terminal::csi_push_key_reporting_flags(Params const& params) {
    auto flags_u32 = params.get(0);
    auto flags = KeyReportingFlags(flags_u32) & KeyReportingFlags::All;

    if (m_key_reporting_flags_stack.size() >= 100) {
        m_key_reporting_flags_stack.pop_front();
    }
    m_key_reporting_flags_stack.push_back(m_key_reporting_flags);
    m_key_reporting_flags = flags;
}

// https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement
void Terminal::csi_pop_key_reporting_flags(Params const& params) {
    auto n = params.get(0, 1);
    if (n == 0) {
        return;
    }
    if (n >= m_key_reporting_flags_stack.size()) {
        m_key_reporting_flags_stack.clear();
        m_key_reporting_flags = KeyReportingFlags::None;
        return;
    }

    auto new_stack_size = m_key_reporting_flags_stack.size() - n;
    m_key_reporting_flags = m_key_reporting_flags_stack[new_stack_size];
    m_key_reporting_flags_stack.erase(m_key_reporting_flags_stack.begin() + isize(new_stack_size));
}

void Terminal::set_cursor(u32 row, u32 col) {
    m_cursor_row = di::clamp(row, min_row_inclusive(), max_row_inclusive());
    m_cursor_col = di::clamp(col, min_col_inclusive(), max_col_inclusive());
    m_x_overflow = false;
}

void Terminal::set_visible_size(dius::tty::WindowSize const& window_size) {
    if (m_available_rows_in_display == window_size.rows && m_available_cols_in_display == window_size.cols &&
        m_available_xpixels_in_display == window_size.pixel_width &&
        m_available_ypixels_in_display == window_size.pixel_height) {
        return;
    }

    m_available_rows_in_display = window_size.rows;
    m_available_cols_in_display = window_size.cols;
    m_available_xpixels_in_display = window_size.pixel_width;
    m_available_ypixels_in_display = window_size.pixel_height;
    if (!m_80_col_mode && !m_132_col_mode && !m_force_terminal_size) {
        resize(window_size);
    }
}

void Terminal::resize(dius::tty::WindowSize const& window_size) {
    m_row_count = window_size.rows;
    m_col_count = window_size.cols;
    m_xpixels = window_size.pixel_width;
    m_ypixels = window_size.pixel_height;

    m_scroll_start = 0;
    m_scroll_end = m_row_count - 1;

    m_rows.resize(window_size.rows);
    for (auto& row : m_rows) {
        row.resize(window_size.cols);
    }

    for (auto& row : m_rows_above) {
        row.resize(window_size.cols);
    }

    for (auto& row : m_rows_below) {
        row.resize(window_size.cols);
    }

    // Reset the margins - invalid margins make us crash on overflow...
    csi_decstbm({});

    set_cursor(m_cursor_row, m_cursor_col);

    invalidate_all();

    // Send size update to client.
    // TODO: support in-band resize notifications: https://gist.github.com/rockorager/e695fb2924d36b2bcf1fff4a3704bd83
    (void) m_psuedo_terminal.set_tty_window_size(window_size);
}

void Terminal::invalidate_all() {
    for (auto& row : m_rows) {
        for (auto& cell : row) {
            cell.dirty = true;
        }
    }
}

void Terminal::clear_below_cursor(char ch) {
    clear_row_to_end(m_cursor_row, m_cursor_col, ch);
    for (auto r = m_cursor_row + 1; r < m_row_count; r++) {
        clear_row(r, ch);
    }
}

void Terminal::clear_above_cursor(char ch) {
    for (auto r = 0u; r < m_cursor_row; r++) {
        clear_row(r, ch);
    }
    clear_row_until(m_cursor_row, m_cursor_col, ch);
}

void Terminal::clear(char ch) {
    for (auto r = 0u; r < m_row_count; r++) {
        clear_row(r, ch);
    }
}

void Terminal::clear_row(u32 r, char ch) {
    clear_row_to_end(r, 0, ch);
}

void Terminal::clear_row_until(u32 r, u32 end_col, char ch) {
    for (auto c = 0u; c <= end_col; c++) {
        put_char(r, c, ch);
    }
}

void Terminal::clear_row_to_end(u32 r, u32 start_col, char ch) {
    for (auto c = start_col; c < m_col_count; c++) {
        put_char(r, c, ch);
    }
}

void Terminal::put_char(u32 row, u32 col, c32 c) {
    auto& cell = m_rows[row][col];
    cell.ch = c;
    cell.graphics_rendition = m_current_graphics_rendition;
    cell.dirty = true;
}

void Terminal::put_char(c32 c) {
    if (c <= 31 || c == 127) {
        put_char('^');
        put_char(c | 0x40);
        return;
    }

    if (m_x_overflow) {
        m_cursor_row++;
        scroll_down_if_needed();
        m_cursor_col = 0;
        m_x_overflow = false;
    }

    put_char(m_cursor_row, m_cursor_col, c);

    m_cursor_col++;
    if (m_cursor_col >= m_col_count) {
        m_x_overflow = m_autowrap_mode;
        m_cursor_col--;
    }
}

auto Terminal::should_display_cursor_at_position(u32 r, u32 c) const -> bool {
    if (m_cursor_hidden) {
        return false;
    }

    if (c != m_cursor_col) {
        return false;
    }

    if (m_cursor_row < m_scroll_start || m_cursor_row > m_scroll_end || r < m_scroll_start || r > m_scroll_end) {
        return r == m_cursor_row;
    }

    return row_offset() + r == cursor_row() + total_rows() - row_count();
}

auto Terminal::scroll_relative_offset(u32 display_row) const -> u32 {
    if (display_row < m_scroll_start) {
        return display_row;
    }
    if (display_row > m_scroll_end) {
        return display_row + total_rows() - row_count();
    }
    return display_row + row_offset();
}

auto Terminal::row_at_scroll_relative_offset(u32 offset) const -> Terminal::Row const& {
    if (offset < m_scroll_start) {
        return m_rows[offset];
    }
    if (offset < m_scroll_start + m_rows_above.size()) {
        return m_rows_above[offset - m_scroll_start];
    }
    if (offset < m_scroll_start + m_rows_above.size() + (m_scroll_end - m_scroll_start)) {
        return m_rows[offset - m_rows_above.size()];
    }
    if (offset < m_scroll_start + m_rows_above.size() + (m_scroll_end - m_scroll_start) + m_rows_below.size()) {
        return m_rows_below[offset - m_scroll_start - m_rows_above.size() - (m_scroll_end - m_scroll_start)];
    }
    return m_rows[offset - m_rows_above.size() - m_rows_below.size()];
}

void Terminal::set_use_alternate_screen_buffer(bool b) {
    if ((!b && !m_save_state) || (b && m_save_state)) {
        return;
    }

    if (b) {
        m_save_state = di::make_box<Terminal>(di::clone(*this));
        m_current_graphics_rendition = {};
        m_x_overflow = false;
        m_cursor_hidden = false;
        m_cursor_row = m_cursor_col = m_saved_cursor_row = m_saved_cursor_col = 0;
        m_rows.resize(m_row_count);
        m_rows_above.clear();
        m_rows_below.clear();
        clear();
    } else {
        ASSERT(m_save_state);
        m_cursor_row = m_save_state->m_cursor_row;
        m_cursor_col = m_save_state->m_cursor_col;
        m_saved_cursor_row = m_save_state->m_saved_cursor_row;
        m_saved_cursor_col = m_save_state->m_saved_cursor_col;
        m_current_graphics_rendition = m_save_state->m_current_graphics_rendition;
        m_x_overflow = m_save_state->m_x_overflow;
        m_cursor_hidden = m_save_state->m_cursor_hidden;
        m_rows = di::move(m_save_state->m_rows);
        m_rows_above = di::move(m_save_state->m_rows_above);
        m_rows_below = di::move(m_save_state->m_rows_below);

        if (m_row_count != m_save_state->m_row_count || m_col_count != m_save_state->m_col_count ||
            m_xpixels != m_save_state->m_xpixels || m_ypixels != m_save_state->m_ypixels) {
            resize({ m_row_count, m_col_count, m_xpixels, m_ypixels });
        } else {
            invalidate_all();
        }

        m_save_state = nullptr;
    }
}

void Terminal::scroll_up() {
    if (m_rows_above.empty()) {
        return;
    }

    di::rotate(m_rows.begin() + m_scroll_start, m_rows.begin() + m_scroll_end, m_rows.begin() + m_scroll_end + 1);
    m_rows_below.push_back(di::move(m_rows[m_scroll_start]));
    m_rows[m_scroll_start] = m_rows_above.pop_back().value();
    invalidate_all();
}

void Terminal::scroll_down() {
    if (m_rows_below.empty()) {
        return;
    }

    di::rotate(m_rows.begin() + m_scroll_start, m_rows.begin() + m_scroll_start + 1, m_rows.begin() + m_scroll_end + 1);
    m_rows_above.push_back(di::move(m_rows[m_scroll_end]));
    m_rows[m_scroll_end] = m_rows_below.pop_back().value();
    invalidate_all();
}

void Terminal::scroll_up_if_needed() {
    if (m_cursor_row == m_scroll_start - 1) {
        m_cursor_row = u32(di::clamp(i32(m_cursor_row), i32(m_scroll_start), i32(m_scroll_end)));

        if (!m_rows_above.empty()) {
            scroll_up();
            return;
        }

        di::rotate(m_rows.begin() + m_scroll_start, m_rows.begin() + m_scroll_end, m_rows.begin() + m_scroll_end + 1);
        m_rows_below.push_back(di::move(m_rows[m_scroll_start]));
        m_rows[m_scroll_start] = Row();
        m_rows[m_scroll_start].resize(m_col_count);
        invalidate_all();

        if (total_rows() - m_rows.size() > m_row_count + 1000) {
            m_rows_below.erase(m_rows_below.begin());
        }
    }
}

void Terminal::scroll_down_if_needed() {
    if (m_cursor_row == m_scroll_end + 1) {
        m_cursor_row = di::clamp(m_cursor_row, m_scroll_start, m_scroll_end);

        if (!m_rows_below.empty()) {
            scroll_down();
            return;
        }

        di::rotate(m_rows.begin() + m_scroll_start, m_rows.begin() + m_scroll_start + 1,
                   m_rows.begin() + m_scroll_end + 1);
        m_rows_above.push_back(di::move(m_rows[m_scroll_end]));
        m_rows[m_scroll_end] = Row();
        m_rows[m_scroll_end].resize(m_col_count);
        invalidate_all();

        if (total_rows() - m_rows.size() > m_row_count + 1000) {
            m_rows_above.erase(m_rows_above.begin());
        }
    }
}

void Terminal::scroll_to_bottom() {
    while (!m_rows_below.empty()) {
        scroll_down();
    }
}

auto Terminal::state_as_escape_sequences_internal(di::VectorWriter<>& writer) const {
    // 1. Terminal size. (note that the visibile size is not reported in any way).
    di::writer_print<di::String::Encoding>(writer, "\033[4;{};{}t"_sv, m_ypixels, m_xpixels);
    di::writer_print<di::String::Encoding>(writer, "\033[8;{};{}t"_sv, m_row_count, m_col_count);
    if (m_80_col_mode || m_132_col_mode) {
        // When writing the mode, first ensure we enable setting the mode.
        di::writer_print<di::String::Encoding>(writer, "\033[?40h"_sv);
        auto _ = di::ScopeExit([&] {
            di::writer_print<di::String::Encoding>(writer, "\033[?40l"_sv);
        });

        if (m_80_col_mode) {
            di::writer_print<di::String::Encoding>(writer, "\033[?3l"_sv);
        } else {
            di::writer_print<di::String::Encoding>(writer, "\033[?3h"_sv);
        }
    }

    // 2. Terminal cell contents.
    {
        // When printing terminal cell contents, ensure auto-wrap is disabled, to prevent accidently scrolling the
        // screen.
        di::writer_print<di::String::Encoding>(writer, "\033[?7l"_sv);
        auto _ = di::ScopeExit([&] {
            di::writer_print<di::String::Encoding>(writer, "\033[?7h"_sv);
        });

        auto first = true;
        auto last_sgr = GraphicsRendition {};
        auto output_row = [&](Row const& row) {
            // Move to the next line (for any row other than the first).
            if (!first) {
                di::writer_print<di::String::Encoding>(writer, "\r\n"_sv);
            }
            first = false;

            for (auto const& cell : row) {
                // Write graphics rendition if needed.
                if (cell.graphics_rendition != last_sgr) {
                    for (auto& params : cell.graphics_rendition.as_csi_params()) {
                        di::writer_print<di::String::Encoding>(writer, "\033[{}m"_sv, params);
                    }
                    last_sgr = cell.graphics_rendition;
                }

                // Write cell text.
                di::writer_print<di::String::Encoding>(writer, "{}"_sv, cell.ch);
            }
        };

        // Write out all cell contents.
        auto all_rows = di::concat(m_rows_above, m_rows, di::reverse(m_rows_below));
        di::container::for_each(all_rows, output_row);

        // Pan up so that the active region is correct.
        if (!m_rows_below.empty()) {
            di::writer_print<di::String::Encoding>(writer, "\033[H\033[{}T"_sv, m_rows_below.size());
        }
    }

    // 3. Tab stops (this is done before setting the cursor position, as it requires moving the cursor)
    for (auto col : m_tab_stops) {
        di::writer_print<di::String::Encoding>(writer, "\033[0;{}H\033H"_sv, col + 1);
    }

    // 4. Internal state.
    {
        // NOTE: Disable drawing (DECSET 2026) is ignored as saving its state is not useful.

        // Scroll margin.
        di::writer_print<di::String::Encoding>(writer, "\033[{};{}r"_sv, m_scroll_start + 1, m_scroll_end + 1);

        // Auto wrap.
        if (m_autowrap_mode) {
            di::writer_print<di::String::Encoding>(writer, "\033[?7h"_sv);
        }

        // Origin mode.
        if (m_origin_mode) {
            di::writer_print<di::String::Encoding>(writer, "\033[?6h"_sv);
        }
    }

    // 5. Application state
    {
        // Cursor keys mode
        if (m_application_cursor_keys_mode == ApplicationCursorKeysMode::Enabled) {
            di::writer_print<di::String::Encoding>(writer, "\033[?1h"_sv);
        }

        // Kitty key flags
        auto first = true;
        auto set_kitty_key_flags = [&](KeyReportingFlags flags) {
            if (first) {
                di::writer_print<di::String::Encoding>(writer, "\033[=1;{}u"_sv, i32(flags));
                first = false;
            } else {
                di::writer_print<di::String::Encoding>(writer, "\033[>{}u"_sv, i32(flags));
            }
        };

        for (auto flags : m_key_reporting_flags_stack) {
            set_kitty_key_flags(flags);
        }
        set_kitty_key_flags(m_key_reporting_flags);

        // Alternate scroll mode
        if (m_alternate_scroll_mode == AlternateScrollMode::Enabled) {
            di::writer_print<di::String::Encoding>(writer, "\033[?1007h"_sv);
        }

        // Mouse protocol
        switch (m_mouse_protocol) {
            case MouseProtocol::None:
                break;
            case MouseProtocol::X10:
                di::writer_print<di::String::Encoding>(writer, "\033[?9h"_sv);
                break;
            case MouseProtocol::VT200:
                di::writer_print<di::String::Encoding>(writer, "\033[?1000h"_sv);
                break;
            case MouseProtocol::BtnEvent:
                di::writer_print<di::String::Encoding>(writer, "\033[?1002h"_sv);
                break;
            case MouseProtocol::AnyEvent:
                di::writer_print<di::String::Encoding>(writer, "\033[?1003h"_sv);
                break;
        }

        // Mouse encoding
        switch (m_mouse_encoding) {
            case MouseEncoding::X10:
                break;
            case MouseEncoding::UTF8:
                di::writer_print<di::String::Encoding>(writer, "\033[?1005h"_sv);
                break;
            case MouseEncoding::SGR:
                di::writer_print<di::String::Encoding>(writer, "\033[?1006h"_sv);
                break;
            case MouseEncoding::URXVT:
                di::writer_print<di::String::Encoding>(writer, "\033[?1015h"_sv);
                break;
            case MouseEncoding::SGRPixels:
                di::writer_print<di::String::Encoding>(writer, "\033[?1016h"_sv);
                break;
        }

        // Focus event mode
        if (m_focus_event_mode == FocusEventMode::Enabled) {
            di::writer_print<di::String::Encoding>(writer, "\033[?1004h"_sv);
        }

        // Bracketed paste
        if (m_bracketed_paste_mode == BracketedPasteMode::Enabled) {
            di::writer_print<di::String::Encoding>(writer, "\033[?2004h"_sv);
        }
    }

    // 6. Cursor
    {
        // Cursor style
        di::writer_print<di::String::Encoding>(writer, "\033[{} q"_sv, i32(m_cursor_style));

        // Cursor position - when in origin mode we need to adjust our coordinates based on the scroll region's
        // start.
        if (m_origin_mode) {
            di::writer_print<di::String::Encoding>(writer, "\033[{};{}H"_sv, m_cursor_row - m_scroll_start + 1,
                                                   m_cursor_col - m_scroll_start + 1);
        } else {
            di::writer_print<di::String::Encoding>(writer, "\033[{};{}H"_sv, m_cursor_row + 1, m_cursor_col + 1);
        }

        // Cursor visible
        if (m_cursor_hidden) {
            di::writer_print<di::String::Encoding>(writer, "\033[?25l"_sv);
        }
    }

    // 7. X-overflow
    {
        // If we're pending overflow, we need to emit the last visibile cell again.
        if (m_x_overflow) {
            auto const& cell = *di::back(m_rows[m_cursor_row]);
            for (auto& params : cell.graphics_rendition.as_csi_params()) {
                di::writer_print<di::String::Encoding>(writer, "\033[{}m"_sv, params);
            }
            di::writer_print<di::String::Encoding>(writer, "{}"_sv, cell.ch);
        }
    }

    // 8. Current sgr
    for (auto& params : m_current_graphics_rendition.as_csi_params()) {
        di::writer_print<di::String::Encoding>(writer, "\033[{}m"_sv, params);
    }
}

auto Terminal::state_as_escape_sequences() const -> di::String {
    auto writer = di::VectorWriter<> {};

    // 1. Reset terminal
    di::writer_print<di::String::Encoding>(writer, "\033c"_sv);

    if (m_save_state) {
        // 2. If in alternate screen buffer, write the main buffer first.
        m_save_state->state_as_escape_sequences_internal(writer);

        // 3. Enter alternate screen buffer, if necssary.
        di::writer_print<di::String::Encoding>(writer, "\033[?1049h"_sv);
    }

    // 4. Write current contents.
    state_as_escape_sequences_internal(writer);

    // Return the resulting string.
    return writer.vector() | di::transform(di::construct<c8>) | di::to<di::String>(di::encoding::assume_valid);
}
}
