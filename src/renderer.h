// VT DOOM
// Copyright (c) 2024 James Holderness
// Distributed under the GPL-2.0 License

#pragma once

#include <string>
#include <string_view>
#include <vector>

class renderer {
public:
    renderer(const int screen_height, const int screen_width);
    ~renderer();
    void render_frame(const unsigned char* frame);

private:
    void _reset();
    void _flush();
    void _append(const char c);
    void _append(const std::string_view s);
    void _append(const int n);
    void _append_palette();
    void _append_sixel(const int sixel, const int repeat = 1);

    int _scale = 1;
    int _xindent = 0;
    int _yindent = 0;
    std::string _ypadding;
    std::vector<char> _buffer;
    char* _buffer_ptr = nullptr;
    bool _palette_initialized = false;
};
