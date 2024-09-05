// VT DOOM
// Copyright (c) 2024 James Holderness
// Distributed under the GPL-2.0 License

#include "PureDOOM.h"
#include "input.h"
#include "os.h"
#include "renderer.h"

#include <algorithm>
#include <iostream>

int main(int argc, char** argv)
{
    os os;
    input input;

    // If the first parameter of the DA report is 60 or more, then the remaining
    // parameters indicate the supported extensions, and sixel is extension 4.
    const auto da = input.get_device_attributes();
    const auto has_sixel = std::find(da.begin(), da.end(), 4) != da.end();
    if (da[0] < 60 || !has_sixel) {
        std::cout << "VT DOOM requires a terminal supporting Sixel graphics.\n";
        return 1;
    }

    static const char* last_print_string = nullptr;
    doom_set_print([](const char* s) {
        // We track the last print string to display as an error message when
        // we receive an exit call. But this can sometimes be followed by a
        // linefeed which we need to ignore.
        if (*s != '\n') last_print_string = s;
    });

    try {
        doom_set_exit([](int exit_code) { throw exit_code; });
        doom_init(argc, argv, 0);

        const auto [height, width] = input.get_screen_size();
        auto r = renderer{height, width};

        while (input) {
            doom_update();
            r.render_frame(doom_get_framebuffer(1));
        }

        return 0;
    } catch (int exit_code) {
        // If the exit code is non-zero, this is an error event, and the
        // error message is likely recorded in the last print string.
        if (exit_code)
            std::cout << last_print_string << "\n";
        return exit_code ? 1 : 0;
    }
}
