// VT DOOM
// Copyright (c) 2024 James Holderness
// Distributed under the GPL-2.0 License

#include "input.h"

#include "PureDOOM.h"
#include "os.h"

#include <iostream>

namespace {
    template <typename T>
    void for_each_modifier(const int modifiers, T&& lambda)
    {
        if (modifiers >= 2) {
            const auto modifier_mask = modifiers - 1;
            if (modifier_mask & 1) lambda(DOOM_KEY_SHIFT);
            if (modifier_mask & 2) lambda(DOOM_KEY_ALT);
            if (modifier_mask & 4) lambda(DOOM_KEY_CTRL);
        }
    }
}  // namespace

input::input()
{
    // Hide the cursor.
    std::cout << "\033[?25l";
    // Enable win32-input mode.
    std::cout << "\033[?9001h";
    // State the keyboard thread.
    _thread = std::thread([&]() {
        while (!_exiting)
            _parse_char(os::getch());
    });
}

input::~input()
{
    // Disable win32-input mode.
    std::cout << "\033[?9001l";
    // Request a DSR-OS report to unblock the input thread.
    std::cout << "\033[5n";
    std::cout.flush();
    // Wait for the thread to exit.
    _thread.join();
    // Show the cursor again.
    std::cout << "\033[?25h";
}

input::operator bool() const
{
    return !_exit_requested;
}

std::span<int> input::get_device_attributes() const
{
    auto lock = std::unique_lock(_query_mutex);
    // Request primary device attributes.
    std::cout << "\033[c";
    std::cout.flush();
    _query_condition.wait(lock, [&] { return !_device_attributes.empty(); });
    return _device_attributes;
}

std::pair<int, int> input::get_screen_size() const
{
    auto lock = std::unique_lock(_query_mutex);
    // Move to the bottom right corner and request the cell size and cursor position.
    // The latter gives us the screen size in cells.
    std::cout << "\033[9999;9999H";
    std::cout << "\033[16t";
    std::cout << "\033[6n";
    std::cout.flush();
    _cell_size = {};
    _cursor_pos = {};
    _query_condition.wait(lock, [&] { return _cursor_pos.has_value(); });
    // If no cell size is reported, assume VT340-compatible 20x10.
    if (!_cell_size.has_value())
        _cell_size = { 20, 10 };
    const auto [cell_height, cell_width] = _cell_size.value();
    const auto [rows, columns] = _cursor_pos.value();
    // Return the screen size in pixels.
    return { rows * cell_height, columns * cell_width };
}

void input::_parse_char(const char ch)
{
    if (ch == 3) {
        _exit_requested = true;
    } else if (ch == 27) {
        // It's too much effort to detect an individual escape key, so this
        // just treats a double escape as a single key press.
        if (_state != states::esc)
            _state = states::esc;
        else {
            _simulate_press_release(DOOM_KEY_ESCAPE);
            _state = states::ground;
        }
    } else if (_state == states::esc) {
        if (ch == 'O')
            _state = states::ss3;
        else if (ch == '[')
            _state = states::csi;
        else
            _state = states::ground;
        _parm = 0;
        _parm_count = 0;
        _parm_prefix = 0;
    } else if (_state == states::ground) {
        _ascii_key(ch);
    } else if (_state == states::ss3) {
        _ss3_key(ch);
        _state = states::ground;
    } else if (_state == states::csi) {
        if (ch >= '0' && ch <= '9') {
            _parm = _parm * 10 + (ch - '0');
        } else if (ch >= '<' && ch <= '?') {
            _parm_prefix = ch;
        } else {
            if (_parm_count < _parms.size())
                _parms[_parm_count++] = _parm;
            _parm = 0;
            switch (ch) {
                case ';':
                    return;
                case 'n':
                    _exiting = true;
                    break;
                case 'c':
                    if (_parm_prefix == '?')
                        _device_attributes_report();
                    break;
                case 't':
                    if (_parm_count == 3 && _parms[0] == 6 && !_parm_prefix)
                        _cell_size_report(_parms[1], _parms[2]);
                    break;
                case 'R':
                    if (_parm_count == 2 && !_parm_prefix)
                        _position_report(_parms[0], _parms[1]);
                    break;
                case '_':
                    if (_parm_count >= 5 && !_parm_prefix)
                        _win32_key(_parms[0], _parms[3], _parms[4]);
                    break;
                default:
                    if (!_parm_prefix)
                        _csi_key(ch, _parm_count > 0 ? _parms[0] : 0, _parm_count > 1 ? _parms[1] : 0);
                    break;
            }
            _state = states::ground;
        }
    }
}

void input::_device_attributes_report()
{
    {
        auto lock = std::lock_guard(_query_mutex);
        _device_attributes.assign(_parms.begin(), _parms.begin() + _parm_count);
    }
    _query_condition.notify_one();
}

void input::_cell_size_report(const int height, const int width)
{
    auto lock = std::lock_guard(_query_mutex);
    _cell_size = std::make_pair(height, width);
}

void input::_position_report(const int row, const int col)
{
    {
        auto lock = std::lock_guard(_query_mutex);
        _cursor_pos = std::make_pair(row, col);
    }
    _query_condition.notify_one();
}

void input::_ascii_key(const char ch)
{
    switch (ch) {
        case '\0': return _simulate_press_release(DOOM_KEY_SPACE, 5);
        case '\x7F': return _simulate_press_release(DOOM_KEY_BACKSPACE);
        case '\b': return _simulate_press_release(DOOM_KEY_BACKSPACE);
        case '\t': return _simulate_press_release(DOOM_KEY_TAB);
        case '\n': return _simulate_press_release(DOOM_KEY_ENTER);
        case '\r': return _simulate_press_release(DOOM_KEY_ENTER);
        case ' ': return _simulate_press_release(DOOM_KEY_SPACE);
        case '\'': return _simulate_press_release(DOOM_KEY_APOSTROPHE);
        case '*': return _simulate_press_release(DOOM_KEY_MULTIPLY);
        case ',': return _simulate_press_release(DOOM_KEY_COMMA);
        case '-': return _simulate_press_release(DOOM_KEY_MINUS);
        case '.': return _simulate_press_release(DOOM_KEY_PERIOD);
        case '/': return _simulate_press_release(DOOM_KEY_SLASH);
        case ';': return _simulate_press_release(DOOM_KEY_SEMICOLON);
        case '=': return _simulate_press_release(DOOM_KEY_EQUALS);
        case '[': return _simulate_press_release(DOOM_KEY_LEFT_BRACKET);
        case ']': return _simulate_press_release(DOOM_KEY_RIGHT_BRACKET);
        default:
            if (ch >= '0' && ch <= '9')
                _simulate_press_release(DOOM_KEY_0 + (ch - '0'));
            else if (ch >= 'a' && ch <= 'z')
                _simulate_press_release(DOOM_KEY_A + (ch - 'a'));
            break;
    }
}

void input::_ss3_key(const char ch)
{
    switch (ch) {
        case 'P': return _simulate_press_release(DOOM_KEY_F1);
        case 'Q': return _simulate_press_release(DOOM_KEY_F2);
        case 'R': return _simulate_press_release(DOOM_KEY_F3);
        case 'S': return _simulate_press_release(DOOM_KEY_F4);
    }
}

void input::_csi_key(const char ch, const int parm1, const int parm2)
{
    switch (ch) {
        case 'A': return _simulate_press_release(DOOM_KEY_UP_ARROW, parm2);
        case 'B': return _simulate_press_release(DOOM_KEY_DOWN_ARROW, parm2);
        case 'C': return _simulate_press_release(DOOM_KEY_RIGHT_ARROW, parm2);
        case 'D': return _simulate_press_release(DOOM_KEY_LEFT_ARROW, parm2);
        case '~':
            switch (parm1) {
                case 15: return _simulate_press_release(DOOM_KEY_F5);
                case 17: return _simulate_press_release(DOOM_KEY_F6);
                case 18: return _simulate_press_release(DOOM_KEY_F7);
                case 19: return _simulate_press_release(DOOM_KEY_F8);
                case 20: return _simulate_press_release(DOOM_KEY_F9);
                case 21: return _simulate_press_release(DOOM_KEY_F10);
                case 23: return _simulate_press_release(DOOM_KEY_F11);
                case 24: return _simulate_press_release(DOOM_KEY_F12);
            }
            break;
    }
}

void input::_win32_key(const int vkey, const bool pressed, const int modifiers)
{
    if (vkey == 'C' && (modifiers & 8) != 0)
        _exit_requested = true;
    else {
        const auto key = doom_key_t(_map_vkey(vkey));
        if (key != DOOM_KEY_UNKNOWN) {
            if (pressed)
                doom_key_down(key);
            else
                doom_key_up(key);
        }
    }
}

int input::_map_vkey(const int vkey)
{
    switch (vkey) {
        case 8: return DOOM_KEY_BACKSPACE;
        case 9: return DOOM_KEY_TAB;
        case 13: return DOOM_KEY_ENTER;
        case 16: return DOOM_KEY_SHIFT;
        case 17: return DOOM_KEY_CTRL;
        case 18: return DOOM_KEY_ALT;
        case 27: return DOOM_KEY_ESCAPE;
        case 32: return DOOM_KEY_SPACE;
        case 37: return DOOM_KEY_LEFT_ARROW;
        case 38: return DOOM_KEY_UP_ARROW;
        case 39: return DOOM_KEY_RIGHT_ARROW;
        case 40: return DOOM_KEY_DOWN_ARROW;
        case 112: return DOOM_KEY_F1;
        case 113: return DOOM_KEY_F2;
        case 114: return DOOM_KEY_F3;
        case 115: return DOOM_KEY_F4;
        case 116: return DOOM_KEY_F5;
        case 117: return DOOM_KEY_F6;
        case 118: return DOOM_KEY_F7;
        case 119: return DOOM_KEY_F8;
        case 120: return DOOM_KEY_F9;
        case 121: return DOOM_KEY_F10;
        case 122: return DOOM_KEY_F11;
        case 123: return DOOM_KEY_F12;
        default:
            if (vkey >= '0' && vkey <= '9')
                return DOOM_KEY_0 + (vkey - '0');
            else if (vkey >= 'A' && vkey <= 'Z')
                return DOOM_KEY_A + (vkey - 'A');
            else
                return DOOM_KEY_UNKNOWN;
    }
}

void input::_simulate_press_release(const int doom_key, const int modifiers)
{
    // Standard VT key sequences don't track key-up events, so we have to try
    // and simulate that. The way this works is we start a thread whenever a
    // key is pressed, and that thread generates the key-up event 10ms later.
    // But if the key is being held down, so we detect another press of the
    // same key before the timer has elapsed, we don't generate a new key-down
    // event, but instead extend the timer for another 10ms.

    static auto active_counter = 0;
    static auto last_key = -1;
    static auto last_modifiers = 0;
    static auto mutex = std::mutex{};
    auto lock = std::lock_guard(mutex);

    if (doom_key != last_key) {
        if (last_key != -1) {
            doom_key_up(doom_key_t(last_key));
            for_each_modifier(last_modifiers, [](const auto modifier_key) {
                doom_key_up(modifier_key);
            });
        }
        for_each_modifier(modifiers, [](const auto modifier_key) {
            doom_key_down(modifier_key);
        });
        doom_key_down(doom_key_t(doom_key));
        last_key = doom_key;
        last_modifiers = modifiers;
    }

    const auto this_counter = ++active_counter;
    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto lock = std::lock_guard(mutex);
        if (this_counter == active_counter) {
            doom_key_up(doom_key_t(doom_key));
            for_each_modifier(modifiers, [](const auto modifier_key) {
                doom_key_up(modifier_key);
            });
            last_key = -1;
            last_modifiers = 0;
        }
    }).detach();
}
