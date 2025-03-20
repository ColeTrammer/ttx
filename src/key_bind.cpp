#include "key_bind.h"

#include "actions.h"
#include "input_mode.h"
#include "tab.h"
#include "ttx/modifiers.h"

namespace ttx {
auto make_switch_tab_binds(di::Vector<KeyBind>& result) {
    for (auto i : di::range(9zu)) {
        auto key = Key(di::to_underlying(Key::_1) + i);
        result.push_back({
            .key = key,
            .mode = InputMode::Normal,
            .action = switch_tab(i + 1),
        });
    }
}

auto make_navigate_binds(di::Vector<KeyBind>& result, InputMode mode, InputMode next_mode) {
    auto keys = di::Array {
        di::Tuple { Key::J, NavigateDirection::Down },
        di::Tuple { Key::K, NavigateDirection::Up },
        di::Tuple { Key::L, NavigateDirection::Right },
        di::Tuple { Key::H, NavigateDirection::Left },
    };
    for (auto [key, direction] : keys) {
        result.push_back({
            .key = key,
            .modifiers = Modifiers::Control,
            .mode = mode,
            .next_mode = next_mode,
            .action = navigate(direction),
        });
    }
}

auto make_resize_binds(di::Vector<KeyBind>& result, InputMode mode) {
    auto keys = di::Array {
        di::Tuple { Key::J, ResizeDirection::Bottom },
        di::Tuple { Key::K, ResizeDirection::Top },
        di::Tuple { Key::L, ResizeDirection::Right },
        di::Tuple { Key::H, ResizeDirection::Left },
    };
    for (auto [key, direction] : keys) {
        result.push_back({
            .key = key,
            .mode = mode,
            .next_mode = InputMode::Resize,
            .action = resize(direction, 2),
        });
        result.push_back({
            .key = key,
            .modifiers = Modifiers::Shift,
            .mode = mode,
            .next_mode = InputMode::Resize,
            .action = resize(direction, -2),
        });
    }
}

auto make_key_binds(Key prefix) -> di::Vector<KeyBind> {
    auto result = di::Vector<KeyBind> {};

    // Insert mode
    {
        result.push_back({
            .key = prefix,
            .modifiers = Modifiers::Control,
            .mode = InputMode::Insert,
            .next_mode = InputMode::Normal,
            .action = enter_normal_mode(),
        });
        result.push_back({
            .key = Key::None,
            .mode = InputMode::Insert,
            .action = send_to_pane(),
        });
    }

    // Normal Mode
    {
        result.push_back({
            .key = prefix,
            .modifiers = Modifiers::Control,
            .mode = InputMode::Normal,
            .action = send_to_pane(),
        });
        make_resize_binds(result, InputMode::Normal);
        make_navigate_binds(result, InputMode::Normal, InputMode::Switch);
        result.push_back({
            .key = Key::C,
            .mode = InputMode::Normal,
            .action = create_tab(),
        });
        make_switch_tab_binds(result);
        result.push_back({
            .key = Key::D,
            .mode = InputMode::Normal,
            .action = quit(),
        });
        result.push_back({
            .key = Key::I,
            .modifiers = Modifiers::Shift,
            .mode = InputMode::Normal,
            .action = stop_capture(),
        });
        result.push_back({
            .key = Key::X,
            .mode = InputMode::Normal,
            .action = exit_pane(),
        });
        result.push_back({
            .key = Key::BackSlash,
            .modifiers = Modifiers::Shift,
            .mode = InputMode::Normal,
            .action = add_pane(Direction::Horizontal),
        });
        result.push_back({
            .key = Key::Minus,
            .mode = InputMode::Normal,
            .action = add_pane(Direction::Vertical),
        });
        result.push_back({
            .key = Key::None,
            .mode = InputMode::Normal,
            .action = reset_mode(),
        });
    }

    // Switch Mode
    {
        make_navigate_binds(result, InputMode::Switch, InputMode::Switch);
        result.push_back({
            .key = Key::None,
            .mode = InputMode::Switch,
            .action = reset_mode(),
        });
    }

    // Resize Mode
    {
        make_resize_binds(result, InputMode::Resize);
        make_navigate_binds(result, InputMode::Resize, InputMode::Resize);
        result.push_back({
            .key = Key::None,
            .mode = InputMode::Resize,
            .action = reset_mode(),
        });
    }

    return result;
}
}
