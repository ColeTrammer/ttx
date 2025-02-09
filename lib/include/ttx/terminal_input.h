#pragma once

#include "di/container/string/string_view.h"
#include "di/container/vector/vector.h"
#include "di/vocab/variant/variant.h"
#include "ttx/escape_sequence_parser.h"
#include "ttx/key_event.h"
#include "ttx/mouse_event.h"

namespace ttx {
using Event = di::Variant<KeyEvent, MouseEvent>;

class TerminalInputParser {
public:
    auto parse(di::StringView input) -> di::Vector<Event>;

private:
    void handle(PrintableCharacter const& printable_character);
    void handle(DCS const& dcs);
    void handle(CSI const& csi);
    void handle(Escape const& escape);
    void handle(ControlCharacter const& control_character);

    EscapeSequenceParser m_parser;
    di::Vector<Event> m_events;
};
}
