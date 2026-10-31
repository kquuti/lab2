#include "core/Reference.h"

#include <cctype>
#include <stdexcept>

std::string typeName(ReferenceType type) {
    switch (type) {
        case ReferenceType::Book:
            return "Book";
        case ReferenceType::Web:
            return "Web page";
        case ReferenceType::Video:
            return "Video";
        case ReferenceType::Audio:
            return "Audio";
    }
    return "Unknown";
}

ReferenceType typeFromInt(int value) {
    if (value < 0 || value >= kReferenceTypeCount) {
        throw std::invalid_argument("Unknown reference type: " + std::to_string(value));
    }
    return static_cast<ReferenceType>(value);
}

namespace {

char lowerAscii(char c) {
    const unsigned char u = static_cast<unsigned char>(c);
    return u < 128 ? static_cast<char>(std::tolower(u)) : c;
}

std::string lowered(const std::string& text) {
    std::string result = text;
    for (char& c : result) {
        c = lowerAscii(c);
    }
    return result;
}

}  // namespace

bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    return lowered(haystack).find(lowered(needle)) != std::string::npos;
}

bool Reference::matches(const std::string& text, SearchField field) const {
    if (text.empty()) {
        return true;
    }
    const bool all = field == SearchField::All;
    if ((all || field == SearchField::Title) && containsIgnoreCase(title_, text)) {
        return true;
    }
    if (all || field == SearchField::Author) {
        for (const std::string& author : authors_) {
            if (containsIgnoreCase(author, text)) {
                return true;
            }
        }
    }
    if ((all || field == SearchField::Year) && year_ != 0 &&
        containsIgnoreCase(std::to_string(year_), text)) {
        return true;
    }
    if ((all || field == SearchField::Comment) && containsIgnoreCase(comment_, text)) {
        return true;
    }
    if (all && (containsIgnoreCase(publisher_, text) || containsIgnoreCase(url_, text))) {
        return true;
    }
    return false;
}