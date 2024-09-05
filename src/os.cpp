// VT DOOM
// Copyright (c) 2024 James Holderness
// Distributed under the GPL-2.0 License

#include "os.h"

#ifdef _WIN32

#include <Windows.h>

DWORD output_mode;
DWORD input_mode;

os::os()
{
    HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(output_handle, &output_mode);
    SetConsoleMode(output_handle, output_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
    HANDLE input_handle = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(input_handle, &input_mode);
    SetConsoleMode(input_handle, input_mode & ~ENABLE_LINE_INPUT & ~ENABLE_ECHO_INPUT & ~ENABLE_PROCESSED_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT);
}

os::~os()
{
    HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleMode(output_handle, output_mode);
    HANDLE input_handle = GetStdHandle(STD_INPUT_HANDLE);
    SetConsoleMode(input_handle, input_mode);
}

int os::getch()
{
    char ch;
    DWORD chars_read = 0;
    HANDLE input_handle = GetStdHandle(STD_INPUT_HANDLE);
    ReadConsoleA(input_handle, &ch, 1, &chars_read, NULL);
    return chars_read == 1 ? static_cast<int>(ch) : -1;
}
#endif

#ifdef __linux__

#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include <cstdio>

struct termios term_attributes;

os::os()
{
    tcgetattr(STDIN_FILENO, &term_attributes);
    auto new_term_attributes = term_attributes;
    new_term_attributes.c_lflag &= ~(ICANON | ISIG | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term_attributes);
}

os::~os()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &term_attributes);
}

int os::getch()
{
    return getchar();
}

#endif
