#pragma once

#include "di/reflect/prelude.h"
#include "input_mode.h"
#include "key_bind.h"
#include "layout_state.h"
#include "ttx/focus_event.h"
#include "ttx/key_event.h"
#include "ttx/paste_event.h"

namespace ttx {
class RenderThread;

class InputThread {
public:
    static auto create(di::Vector<di::TransparentStringView> command, di::Vector<KeyBind> key_binds,
                       di::Synchronized<LayoutState>& layout_state, RenderThread& render_thread)
        -> di::Result<di::Box<InputThread>>;

    explicit InputThread(di::Vector<di::TransparentStringView> command, di::Vector<KeyBind> key_binds,
                         di::Synchronized<LayoutState>& layout_state, RenderThread& render_thread);
    ~InputThread();

    void request_exit();

private:
    void input_thread();

    void set_input_mode(InputMode mode);

    void handle_event(KeyEvent const& event);
    void handle_event(MouseEvent const& event);
    void handle_event(FocusEvent const& event);
    void handle_event(PasteEvent const& event);

    InputMode m_mode { InputMode::Insert };
    di::Vector<KeyBind> m_key_binds;
    di::Vector<di::TransparentStringView> m_command;
    di::Atomic<bool> m_done { false };
    di::Synchronized<LayoutState>& m_layout_state;
    RenderThread& m_render_thread;
    dius::Thread m_thread;
};
}
