#pragma once
#include <string>
#include <vector>

#include "core/Phase.h"

namespace adhd::ui {

namespace ansi {
constexpr const char* reset = "\x1b[0m";
constexpr const char* bold = "\x1b[1m";
constexpr const char* dim = "\x1b[2m";
constexpr const char* red = "\x1b[31m";
constexpr const char* green = "\x1b[32m";
constexpr const char* yellow = "\x1b[33m";
constexpr const char* blue = "\x1b[34m";
constexpr const char* magenta = "\x1b[35m";
constexpr const char* cyan = "\x1b[36m";
constexpr const char* gray = "\x1b[90m";
} // namespace ansi

const char* phaseColor(Phase p);

// 5-row block-letter rendering of digits and ':'.
std::vector<std::string> bigText(const std::string& text);

std::string progressBar(double fraction, int width, const char* fillColor);

// Display width, ignoring ANSI escapes and counting each UTF-8 code point as one cell.
int visibleWidth(const std::string& s);
std::string padRight(const std::string& s, int width);
std::string center(const std::string& s, int width);
// Left part and right part on one line, `width` cells wide.
std::string spread(const std::string& left, const std::string& right, int width);
// Word wrap plain (escape-free) text.
std::vector<std::string> wrap(const std::string& text, int width);
// Wrap "a · b · c" lists without splitting an item across lines.
std::vector<std::string> wrapItems(const std::string& text, int width, const std::string& sep = " · ");
// Cut plain text to `width` cells, adding an ellipsis.
std::string truncate(const std::string& text, int width);

} // namespace adhd::ui
