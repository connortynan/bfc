#include <stdio.h>

// Platform detection
#ifdef _WIN32
#include <conio.h> // Windows-specific
#else
#include <unistd.h>
#include <termios.h>
#endif

#ifdef _WIN32
assert(false && "iomode.h not implemented for Windows (yet)")
#else
// Linux/Unix-specific function to disable canonical mode and echo
void enableRawMode(struct termios *orig_termios)
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    *orig_termios = raw;                      // Save original terminal settings
    raw.c_lflag &= ~(ICANON);                 // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // Apply new settings
}

void disableRawMode(struct termios *orig_termios)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios); // Restore original settings
}
#endif
