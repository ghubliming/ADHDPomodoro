#pragma once
#include <string>

namespace adhd::ui {

enum class KeyType { None, Char, Enter, Escape, Backspace, Up, Down, Left, Right };

struct Key {
    KeyType type = KeyType::None;
    std::string text; // UTF-8 for KeyType::Char

    explicit operator bool() const { return type != KeyType::None; }
    char ch() const { return type == KeyType::Char && text.size() == 1 ? text[0] : '\0'; }
};

// RAII terminal session: raw keyboard input, alternate screen, hidden cursor.
// Everything is restored on destruction, Ctrl+C included.
class Terminal {
public:
    Terminal();
    ~Terminal();
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    Key readKey(int timeoutMs);
    void write(const std::string& s);
    int width() const;
    int height() const;

    static bool interrupted();             // Ctrl+C / SIGTERM received
    static bool stdinIsTty();
    static bool stdoutIsTty();
    static void enableUtf8AndColors();     // no-op outside Windows
};

} // namespace adhd::ui
