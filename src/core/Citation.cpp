#include "core/Citation.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

std::size_t codePointLength(unsigned char lead) {
    if (lead < 0x80) {
        return 1;
    }
    if ((lead >> 5) == 0x6) {
        return 2;
    }
    if ((lead >> 4) == 0xE) {
        return 3;
    }
    if ((lead >> 3) == 0x1E) {
        return 4;
    }
    return 1;
}

std::vector<std::string> splitWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream stream(text);
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}

std::string joinStrings(const std::vector<std::string>& items, const std::string& separator) {
    std::string result;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            result += separator;
        }
        result += items[i];
    }
    return result;
}

std::string stripTrailingDots(std::string text) {
    while (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text;
}

/// "Видавець, Рік" (відсутні частини пропускаються).
std::string publisherAndYear(const Reference& reference) {
    std::vector<std::string> parts;
    if (!reference.publisher().empty()) {
        parts.push_back(reference.publisher());
    }
    if (reference.year() > 0) {
        parts.push_back(std::to_string(reference.year()));
    }
    return joinStrings(parts, ", ");
}

/// "Прізвище І. П."
std::string shortAuthor(const std::string& author) {
    const std::string surname = authorSurname(author);
    const std::string initials = authorInitials(author, " ");
    return initials.empty() ? surname : surname + " " + initials;
}

/// Ключ запису BibTeX: прізвище першого автора латиницею + рік, або ref<id>.
std::string bibKey(const Reference& reference) {
    std::string key;
    if (!reference.authors().empty()) {
        for (char c : authorSurname(reference.authors().front())) {
            const unsigned char u = static_cast<unsigned char>(c);
            if (u < 128 && std::isalnum(u)) {
                key += c;
            }
        }
    }
    if (key.empty()) {
        return "ref" + std::to_string(reference.id());
    }
    if (reference.year() > 0) {
        key += std::to_string(reference.year());
    }
    return key;
}

}  // namespace

std::string authorSurname(const std::string& author) {
    const std::vector<std::string> words = splitWords(author);
    if (words.empty()) {
        return "";
    }
    std::string surname = words.front();
    while (!surname.empty() && surname.back() == ',') {
        surname.pop_back();
    }
    return surname;
}

std::string authorInitials(const std::string& author, const std::string& separator) {
    const std::vector<std::string> words = splitWords(author);
    std::vector<std::string> initials;
    for (std::size_t i = 1; i < words.size(); ++i) {
        const std::string& word = words[i];
        const std::size_t length =
            std::min(codePointLength(static_cast<unsigned char>(word[0])), word.size());
        initials.push_back(word.substr(0, length) + ".");
    }
    return joinStrings(initials, separator);
}

// ---------- ДСТУ ГОСТ 7.1:2006 ----------

std::string GostFormatter::name() const {
    return "ДСТУ ГОСТ 7.1:2006";
}

std::string GostFormatter::format(const Reference& reference) const {
    std::vector<std::string> authors;
    for (const std::string& author : reference.authors()) {
        authors.push_back(shortAuthor(author));
    }
    std::string head;
    if (!authors.empty()) {
        head = joinStrings(authors, ", ") + " ";
    }
    head += stripTrailingDots(reference.title());
    if (reference.type() != ReferenceType::Book) {
        head += " [Електронний ресурс]";
    }

    std::string result = head;
    const std::string publication = publisherAndYear(reference);
    if (!publication.empty()) {
        result += ". – " + publication;
    }
    if (!reference.url().empty()) {
        result += ". – Режим доступу: " + reference.url();
    } else {
        result += ".";
    }
    return result;
}

// ---------- ДСТУ 8302:2015 ----------

std::string Dstu8302Formatter::name() const {
    return "ДСТУ 8302:2015";
}

std::string Dstu8302Formatter::format(const Reference& reference) const {
    std::vector<std::string> authors;
    for (const std::string& author : reference.authors()) {
        authors.push_back(shortAuthor(author));
    }
    std::string result;
    if (!authors.empty()) {
        result = joinStrings(authors, ", ") + " ";
    }
    result += stripTrailingDots(reference.title()) + ".";
    const std::string publication = publisherAndYear(reference);
    if (!publication.empty()) {
        result += " " + publication + ".";
    }
    if (!reference.url().empty()) {
        result += " URL: " + reference.url();
    }
    return result;
}

// ---------- Harvard ----------

std::string HarvardFormatter::name() const {
    return "Harvard";
}

std::string HarvardFormatter::format(const Reference& reference) const {
    std::vector<std::string> authors;
    for (const std::string& author : reference.authors()) {
        const std::string surname = authorSurname(author);
        const std::string initials = authorInitials(author, "");
        authors.push_back(initials.empty() ? surname : surname + ", " + initials);
    }
    std::string names;
    if (authors.size() == 1) {
        names = authors.front();
    } else if (authors.size() > 1) {
        const std::vector<std::string> allButLast(authors.begin(), authors.end() - 1);
        names = joinStrings(allButLast, ", ") + " and " + authors.back();
    }

    const std::string year = reference.year() > 0 ? std::to_string(reference.year()) : "n.d.";
    const std::string title = stripTrailingDots(reference.title());
    std::string result;
    if (!names.empty()) {
        result = names + " (" + year + ") " + title + ".";
    } else {
        result = title + " (" + year + ").";
    }
    if (!reference.publisher().empty()) {
        result += " " + reference.publisher() + ".";
    }
    if (!reference.url().empty()) {
        result += " Available at: " + reference.url();
    }
    return result;
}

// ---------- BibTeX ----------

std::string BibTexFormatter::name() const {
    return "BibTeX";
}

std::string BibTexFormatter::format(const Reference& reference) const {
    std::vector<std::string> fields;
    if (!reference.authors().empty()) {
        std::vector<std::string> authors;
        for (const std::string& author : reference.authors()) {
            const std::string surname = authorSurname(author);
            const std::string initials = authorInitials(author, " ");
            authors.push_back(initials.empty() ? surname : surname + ", " + initials);
        }
        fields.push_back("  author = {" + joinStrings(authors, " and ") + "}");
    }
    fields.push_back("  title = {" + reference.title() + "}");
    if (reference.year() > 0) {
        fields.push_back("  year = {" + std::to_string(reference.year()) + "}");
    }
    if (!reference.publisher().empty()) {
        fields.push_back("  publisher = {" + reference.publisher() + "}");
    }
    if (!reference.url().empty()) {
        fields.push_back("  url = {" + reference.url() + "}");
    }
    if (!reference.comment().empty()) {
        fields.push_back("  note = {" + reference.comment() + "}");
    }
    const std::string entryType = reference.type() == ReferenceType::Book ? "book" : "misc";
    return "@" + entryType + "{" + bibKey(reference) + ",\n" + joinStrings(fields, ",\n") + "\n}";
}

// ---------- Загальне ----------

std::vector<std::unique_ptr<ICitationFormatter>> createAllFormatters() {
    std::vector<std::unique_ptr<ICitationFormatter>> formatters;
    formatters.push_back(std::make_unique<GostFormatter>());
    formatters.push_back(std::make_unique<Dstu8302Formatter>());
    formatters.push_back(std::make_unique<HarvardFormatter>());
    formatters.push_back(std::make_unique<BibTexFormatter>());
    return formatters;
}

std::string formatList(const ICitationFormatter& formatter,
                       const std::vector<const Reference*>& references) {
    std::string result;
    int number = 1;
    for (const Reference* reference : references) {
        if (formatter.isNumbered()) {
            result += std::to_string(number++) + ". " + formatter.format(*reference) + "\n";
        } else {
            result += formatter.format(*reference) + "\n\n";
        }
    }
    return result;
}