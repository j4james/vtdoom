// VT DOOM
// Copyright (c) 2024 James Holderness
// Distributed under the GPL-2.0 License

#include "renderer.h"

#include <array>
#include <iostream>

static constexpr auto width = 320;
static constexpr auto height = 200;
static constexpr auto palette_size = 256;

extern "C" unsigned char screen_palette[palette_size * 3];
static unsigned char last_screen_palette[palette_size * 3];

renderer::renderer(const int screen_height, const int screen_width)
    : _scale(std::max(std::min(screen_height / height, screen_width / width), 1)),
      _xindent(std::max((screen_width - width * _scale) / 2, 0)),
      _yindent(std::max((screen_height - height * _scale) / 2, 0)),
      _ypadding(_yindent / (6 * _scale), '-')
{
    // Set the window title.
    std::cout << "\033]21;VT DOOM\033\\";
    // Enable sixel display mode to prevent the page from scrolling if the
    // sixel image happens to extend beyond the bottom of the window.
    std::cout << "\033[?80h";
    std::cout << "\033[H\033[2J";
    // This is just a loose estimate of the required size.
    // It's much more than we're likely to need.
    const auto buffer_size = 500000 * _scale;
    _buffer.resize(buffer_size);
}

renderer::~renderer()
{
    // Reset the window title.
    std::cout << "\033]21\033\\";
    // Clear the screen.
    std::cout << "\033[H\033[J";
    // Reenable sixel scrolling.
    std::cout << "\033[?80l";
}

void renderer::render_frame(const unsigned char* frame)
{
    _reset();
    _append("\033P;1q");

    // We set the sixel aspect ratio here to apply a vertical scaling factor,
    // and scale the repeat counts when outputting the individual sixels below
    // to apply the horizontal scaling factor.
    _append('"');
    _append(_scale);
    _append(";1");

    // The y padding just adds some graphic new lines to the top of the image.
    _append(_ypadding);
    _append_palette();

    const unsigned char* src = frame;
    for (auto y = 0; y < height; y += 6, src += width * 6) {
        if (y > 0) _append('-');
        // Setting the last x position to a negative value forces the offset
        // calculation for the first sixel to be greater than it would otherwise
        // have been, thereby indenting the image by the required amount.
        auto last_x = -_xindent;
        for (auto c = 0; c < palette_size; c++) {
            auto used_color = false;
            for (auto x = 0; x < width; x++, src++) {
                auto sixel = 0;
                for (auto i = 0, bit = 1; i < 6; i++, bit <<= 1) {
                    if (y + i < height)
                        sixel += (*src == c ? bit : 0);
                    src += width;
                }
                if (sixel) {
                    const auto scaled_x = x * _scale;
                    if (!used_color) {
                        used_color = true;
                        if (scaled_x < last_x) {
                            _append('$');
                            last_x = -_xindent;
                        }
                        _append('#');
                        _append(c);
                    }
                    _append_sixel(0, scaled_x - last_x);
                    _append_sixel(sixel, _scale);
                    last_x = scaled_x + _scale;
                }
                src -= width * 6;
            }
            src -= width;
        }
    }

    _append("\033\\");
    _flush();
}

void renderer::_reset()
{
    _buffer_ptr = &_buffer[0];
}

void renderer::_flush()
{
    const auto len = _buffer_ptr - &_buffer[0];
    std::cout.write(&_buffer[0], len);
    std::cout.flush();
}

void renderer::_append(const char ch)
{
    *(_buffer_ptr++) = ch;
}

void renderer::_append(const std::string_view s)
{
    for (auto ch : s)
        _append(ch);
}

void renderer::_append(const int n)
{
    if (n > 999)
        _append(char('0' + (n / 1000)));
    if (n > 99)
        _append(char('0' + ((n / 100) % 10)));
    if (n > 9)
        _append(char('0' + ((n / 10) % 10)));
    _append(char('0' + (n % 10)));
}

void renderer::_append_palette()
{
    // Our palette components are in the range 0 to 255, while sixel requires
    // percent values. This is a precomputed table to handle that mapping.
    static constexpr auto component_map = [] {
        auto map = std::array<int8_t, 256>{};
        for (auto i = 0; i < map.size(); i++)
            map[i] = (i * 100 + 128) / 255;
        return map;
    }();

    // We only include the palette in the sixel sequence if it has changed
    // from the last frame, since much of the time it will be the same.

    const auto same_palette = std::equal(
        std::begin(screen_palette), std::end(screen_palette),
        std::begin(last_screen_palette), std::end(last_screen_palette));

    if (!_palette_initialized || !same_palette) {
        for (auto i = 0; i < palette_size; i++) {
            const auto color_number = (i + 1) % palette_size;
            const auto palette_index = color_number * 3;
            const auto r = screen_palette[palette_index + 0];
            const auto g = screen_palette[palette_index + 1];
            const auto b = screen_palette[palette_index + 2];
            _append('#');
            _append(color_number);
            _append(";2;");
            _append(component_map[r]);
            _append(';');
            _append(component_map[g]);
            _append(';');
            _append(component_map[b]);
        }
    }

    std::copy(std::begin(screen_palette), std::end(screen_palette), std::begin(last_screen_palette));
    _palette_initialized = true;
}

void renderer::_append_sixel(const int sixel, const int repeat)
{
    const auto sixel_char = char('?' + sixel);
    if (repeat <= 3) {
        for (auto i = 0; i < repeat; i++)
            _append(sixel_char);
    } else {
        _append('!');
        _append(repeat);
        _append(sixel_char);
    }
}
