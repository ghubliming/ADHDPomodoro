#include "cli/Render.h"

#include <algorithm>
#include <sstream>

namespace adhd::ui {

const char* phaseColor(Phase p) {
    switch (p) {
        case Phase::Ready: return ansi::blue;
        case Phase::Focus: return ansi::cyan;
        case Phase::ExtendedFocus: return ansi::yellow;
        case Phase::Hyperfocus: return ansi::magenta;
        case Phase::Rest: return ansi::green;
        case Phase::Idle: return ansi::red;
    }
    return ansi::reset;
}

namespace {

const char* const kDigits[10][5] = {
    {" ███ ", "█   █", "█   █", "█   █", " ███ "},
    {"  █  ", " ██  ", "  █  ", "  █  ", " ███ "},
    {" ███ ", "█   █", "  ██ ", " █   ", "█████"},
    {"████ ", "    █", " ███ ", "    █", "████ "},
    {"█   █", "█   █", "█████", "    █", "    █"},
    {"█████", "█    ", "████ ", "    █", "████ "},
    {" ███ ", "█    ", "████ ", "█   █", " ███ "},
    {"█████", "    █", "   █ ", "  █  ", "  █  "},
    {" ███ ", "█   █", " ███ ", "█   █", " ███ "},
    {" ███ ", "█   █", " ████", "    █", " ███ "},
};
const char* const kColon[5] = {"   ", " ▪ ", "   ", " ▪ ", "   "};

} // namespace

std::vector<std::string> bigText(const std::string& text) {
    std::vector<std::string> rows(5);
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        for (int r = 0; r < 5; ++r) {
            if (c >= '0' && c <= '9') rows[r] += kDigits[c - '0'][r];
            else if (c == ':') rows[r] += kColon[r];
            else rows[r] += "     ";
            if (i + 1 < text.size() && c != ':' && text[i + 1] != ':') rows[r] += " ";
        }
    }
    return rows;
}

std::string progressBar(double fraction, int width, const char* fillColor) {
    fraction = std::clamp(fraction, 0.0, 1.0);
    // Eighth-blocks give a smooth bar even in narrow terminals.
    static const char* partial[] = {"", "▏", "▎", "▍", "▌", "▋", "▊", "▉"};
    const int eighths = static_cast<int>(fraction * width * 8 + 0.5);
    const int full = std::min(eighths / 8, width);
    std::string out = fillColor;
    for (int i = 0; i < full; ++i) out += "█";
    int used = full;
    if (full < width && eighths % 8) {
        out += partial[eighths % 8];
        ++used;
    }
    out += ansi::gray;
    for (int i = used; i < width; ++i) out += "─";
    return out + ansi::reset;
}

int visibleWidth(const std::string& s) {
    int w = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == 0x1b) {
            if (i + 1 < s.size() && s[i + 1] == ']') { // OSC: skip to BEL
                while (i < s.size() && s[i] != '\a') ++i;
                continue;
            }
            ++i; // CSI: skip to the final byte
            while (i + 1 < s.size() && !(s[i + 1] >= 0x40 && s[i + 1] <= 0x7E)) ++i;
            ++i;
            continue;
        }
        if ((c & 0xC0) != 0x80) ++w; // count lead bytes only
    }
    return w;
}

std::string padRight(const std::string& s, int width) {
    const int w = visibleWidth(s);
    return w >= width ? s : s + std::string(static_cast<std::size_t>(width - w), ' ');
}

std::string center(const std::string& s, int width) {
    const int w = visibleWidth(s);
    if (w >= width) return s;
    const int left = (width - w) / 2;
    return std::string(static_cast<std::size_t>(left), ' ') + s;
}

std::string spread(const std::string& left, const std::string& right, int width) {
    const int gap = std::max(1, width - visibleWidth(left) - visibleWidth(right));
    return left + std::string(static_cast<std::size_t>(gap), ' ') + right;
}

std::vector<std::string> wrap(const std::string& text, int width) {
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word, line;
    while (words >> word) {
        if (!line.empty() && visibleWidth(line) + 1 + visibleWidth(word) > width) {
            lines.push_back(line);
            line.clear();
        }
        line += (line.empty() ? "" : " ") + word;
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

std::vector<std::string> wrapItems(const std::string& text, int width, const std::string& sep) {
    std::vector<std::string> lines;
    std::string line;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        const std::size_t next = std::min(text.find(sep, pos), text.size());
        const std::string item = text.substr(pos, next - pos);
        if (!line.empty() && visibleWidth(line) + visibleWidth(sep) + visibleWidth(item) > width) {
            lines.push_back(line);
            line.clear();
        }
        line += (line.empty() ? "" : sep) + item;
        pos = next + sep.size();
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

std::string truncate(const std::string& text, int width) {
    if (visibleWidth(text) <= width) return text;
    std::string out;
    int w = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if ((c & 0xC0) != 0x80) {
            if (w == width - 1) break;
            ++w;
        }
        out += text[i];
    }
    return out + "…";
}

} // namespace adhd::ui
