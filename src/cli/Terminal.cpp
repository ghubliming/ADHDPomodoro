#include "cli/Terminal.h"

#include <csignal>
#include <cstdio>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace adhd::ui {

namespace {

volatile std::sig_atomic_t g_interrupted = 0;

// Alternate screen, hide cursor, save window title.
constexpr const char* kEnter = "\x1b[?1049h\x1b[?25l\x1b[22;0t";
constexpr const char* kLeave = "\x1b[23;0t\x1b[?25h\x1b[?1049l";

#ifdef _WIN32

DWORD g_origOutMode = 0;

BOOL WINAPI onCtrl(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT || type == CTRL_CLOSE_EVENT) {
        g_interrupted = 1;
        return TRUE;
    }
    return FALSE;
}

void appendUtf8(std::string& out, unsigned cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

#else

termios g_orig{};
bool g_rawActive = false;

void onSignal(int) { g_interrupted = 1; }

bool waitReadable(int timeoutMs) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
}

bool readByte(unsigned char& c) { return ::read(STDIN_FILENO, &c, 1) == 1; }

void restoreTerminal() {
    if (!g_rawActive) return;
    const std::string leave = kLeave;
    (void)!::write(STDOUT_FILENO, leave.data(), leave.size());
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig);
    g_rawActive = false;
}

#endif

} // namespace

void Terminal::enableUtf8AndColors() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode)) SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

Terminal::Terminal() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(out, &g_origOutMode);
    enableUtf8AndColors();
    SetConsoleCtrlHandler(onCtrl, TRUE);
#else
    tcgetattr(STDIN_FILENO, &g_orig);
    termios raw = g_orig;
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    g_rawActive = true;

    struct sigaction sa {};
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP, &sa, nullptr);
#endif
    write(kEnter);
}

Terminal::~Terminal() {
#ifdef _WIN32
    write(kLeave);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), g_origOutMode);
    SetConsoleCtrlHandler(onCtrl, FALSE);
#else
    restoreTerminal();
#endif
}

void Terminal::write(const std::string& s) {
#ifdef _WIN32
    std::fwrite(s.data(), 1, s.size(), stdout);
    std::fflush(stdout);
#else
    std::size_t done = 0;
    while (done < s.size()) {
        const ssize_t n = ::write(STDOUT_FILENO, s.data() + done, s.size() - done);
        if (n <= 0) break;
        done += static_cast<std::size_t>(n);
    }
#endif
}

Key Terminal::readKey(int timeoutMs) {
    Key key;
#ifdef _WIN32
    for (int waited = 0; !_kbhit(); waited += 10) {
        if (waited >= timeoutMs || g_interrupted) return key;
        Sleep(10);
    }
    const wint_t c = _getwch();
    if (c == 0 || c == 0xE0) {
        switch (_getwch()) {
            case 72: key.type = KeyType::Up; break;
            case 80: key.type = KeyType::Down; break;
            case 75: key.type = KeyType::Left; break;
            case 77: key.type = KeyType::Right; break;
            default: break;
        }
        return key;
    }
    if (c == '\r' || c == '\n') key.type = KeyType::Enter;
    else if (c == 8) key.type = KeyType::Backspace;
    else if (c == 27) key.type = KeyType::Escape;
    else if (c >= 32) {
        key.type = KeyType::Char;
        appendUtf8(key.text, static_cast<unsigned>(c));
    }
    return key;
#else
    if (!waitReadable(timeoutMs)) return key;
    unsigned char c = 0;
    if (!readByte(c)) return key;

    if (c == 27) {
        unsigned char c2 = 0, c3 = 0;
        if (!waitReadable(25) || !readByte(c2)) {
            key.type = KeyType::Escape;
            return key;
        }
        if ((c2 == '[' || c2 == 'O') && waitReadable(25) && readByte(c3)) {
            switch (c3) {
                case 'A': key.type = KeyType::Up; break;
                case 'B': key.type = KeyType::Down; break;
                case 'C': key.type = KeyType::Right; break;
                case 'D': key.type = KeyType::Left; break;
                default:
                    // Swallow the rest of longer sequences (e.g. "\x1b[3~").
                    while (waitReadable(5) && readByte(c3) && !(c3 >= 0x40 && c3 <= 0x7E)) {}
                    break;
            }
        }
        return key;
    }
    if (c == '\r' || c == '\n') key.type = KeyType::Enter;
    else if (c == 127 || c == 8) key.type = KeyType::Backspace;
    else if (c >= 32) {
        key.type = KeyType::Char;
        key.text.assign(1, static_cast<char>(c));
        // Pull in the rest of a UTF-8 sequence.
        const int extra = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
        for (int i = 0; i < extra && waitReadable(5) && readByte(c); ++i) key.text += static_cast<char>(c);
    }
    return key;
#endif
}

int Terminal::width() const {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
        return info.srWindow.Right - info.srWindow.Left + 1;
#else
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
#endif
    return 80;
}

int Terminal::height() const {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
        return info.srWindow.Bottom - info.srWindow.Top + 1;
#else
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) return ws.ws_row;
#endif
    return 24;
}

bool Terminal::interrupted() { return g_interrupted != 0; }

bool Terminal::stdinIsTty() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

bool Terminal::stdoutIsTty() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

} // namespace adhd::ui
