// VT DOOM
// Copyright (c) 2024 James Holderness
// Distributed under the GPL-2.0 License

#pragma once

#include <array>
#include <condition_variable>
#include <optional>
#include <span>
#include <thread>
#include <tuple>
#include <vector>

class input {
public:
    input();
    ~input();
    operator bool() const;
    std::span<int> get_device_attributes() const;
    std::pair<int, int> get_screen_size() const;

private:
    void _parse_char(const char ch);
    void _device_attributes_report();
    void _cell_size_report(const int height, const int width);
    void _position_report(const int row, const int col);
    static void _ascii_key(const char ch);
    static void _ss3_key(const char ch);
    static void _csi_key(const char ch, const int parm1, const int parm2);
    void _win32_key(const int vkey, const bool pressed, const int modifiers);
    static int _map_vkey(const int vkey);
    static void _simulate_press_release(const int doom_key, const int modifiers = 0);

    std::thread _thread;
    volatile bool _exiting = false;

    enum class states {
        ground,
        esc,
        ss3,
        csi
    };

    states _state = states::ground;
    std::array<int, 32> _parms = {};
    int _parm = 0;
    int _parm_count = 0;
    char _parm_prefix = 0;
    mutable std::condition_variable _query_condition;
    mutable std::mutex _query_mutex;
    mutable std::vector<int> _device_attributes;
    mutable std::optional<std::pair<int, int>> _cell_size;
    mutable std::optional<std::pair<int, int>> _cursor_pos;
    bool _exit_requested = false;
};
