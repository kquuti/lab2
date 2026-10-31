#include "core/Storage.h"

#include <istream>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string escape(const std::string& text) {
    std::string result;
    for (char c : text) {
        switch (c) {
            case '\\':
                result += "\\\\";
                break;
            case '\t':
                result += "\\t";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            default:
                result += c;
        }
    }
    return result;
}

std::string unescape(const std::string& text) {
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            ++i;
            switch (text[i]) {
                case 't':
                    result += '\t';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'r':
                    result += '\r';
                    break;
                default:
                    result += text[i];
            }
        } else {
            result += text[i];
        }
    }
    return result;
}

std::vector<std::string> splitTabs(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    for (char c : line) {
        if (c == '\t') {
            fields.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    fields.push_back(current);
    return fields;
}

std::string withoutCarriageReturn(std::string line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return line;
}

int toInt(const std::string& text) {
    std::size_t used = 0;
    int value = 0;
    try {
        value = std::stoi(text, &used);
    } catch (const std::exception&) {
        throw std::runtime_error("not a number: '" + text + "'");
    }
    if (used != text.size()) {
        throw std::runtime_error("not a number: '" + text + "'");
    }
    return value;
}

/// Читає кількість елементів і перевіряє, що стільки полів ще є в рядку.
std::size_t readCount(const std::string& text, std::size_t available) {
    const int count = toInt(text);
    if (count < 0 || static_cast<std::size_t>(count) > available) {
        throw std::runtime_error("bad item count: " + text);
    }
    return static_cast<std::size_t>(count);
}

Reference parseReference(const std::vector<std::string>& f) {
    if (f.size() < 10) {
        throw std::runtime_error("too few fields in reference record");
    }
    Reference reference(typeFromInt(toInt(f[1])), unescape(f[3]));
    reference.setId(toInt(f[2]));
    reference.setYear(toInt(f[4]));
    reference.setPublisher(unescape(f[5]));
    reference.setUrl(unescape(f[6]));
    reference.setComment(unescape(f[7]));

    std::size_t pos = 8;
    const std::size_t authorCount = readCount(f[pos], f.size() - pos - 1);
    ++pos;
    std::vector<std::string> authors;
    for (std::size_t i = 0; i < authorCount; ++i) {
        authors.push_back(unescape(f[pos++]));
    }

    if (pos >= f.size()) {
        throw std::runtime_error("missing context list");
    }
    const std::size_t contextCount = readCount(f[pos], f.size() - pos - 1);
    ++pos;
    std::set<int> contexts;
    for (std::size_t i = 0; i < contextCount; ++i) {
        contexts.insert(toInt(f[pos++]));
    }

    reference.setAuthors(authors);
    reference.setContextIds(contexts);
    return reference;
}

}  // namespace

void saveCollection(const Collection& collection, std::ostream& out) {
    out << "REFS\t1\n";
    for (const Context& context : collection.contexts()) {
        out << "C\t" << context.id << '\t' << escape(context.name) << '\t' << context.parentId << '\n';
    }
    for (const Reference& reference : collection.references()) {
        out << "R\t" << static_cast<int>(reference.type()) << '\t' << reference.id() << '\t'
            << escape(reference.title()) << '\t' << reference.year() << '\t'
            << escape(reference.publisher()) << '\t' << escape(reference.url()) << '\t'
            << escape(reference.comment()) << '\t' << reference.authors().size();
        for (const std::string& author : reference.authors()) {
            out << '\t' << escape(author);
        }
        out << '\t' << reference.contextIds().size();
        for (int contextId : reference.contextIds()) {
            out << '\t' << contextId;
        }
        out << '\n';
    }
}

Collection loadCollection(std::istream& in) {
    std::string line;
    if (!std::getline(in, line) || withoutCarriageReturn(line) != "REFS\t1") {
        throw std::runtime_error("Unknown file format");
    }

    Collection collection;
    int lineNumber = 1;
    while (std::getline(in, line)) {
        ++lineNumber;
        line = withoutCarriageReturn(line);
        if (line.empty()) {
            continue;
        }
        try {
            const std::vector<std::string> fields = splitTabs(line);
            if (fields[0] == "C") {
                if (fields.size() != 4) {
                    throw std::runtime_error("bad context record");
                }
                Context context;
                context.id = toInt(fields[1]);
                context.name = unescape(fields[2]);
                context.parentId = toInt(fields[3]);
                collection.restoreContext(context);
            } else if (fields[0] == "R") {
                collection.restoreReference(parseReference(fields));
            } else {
                throw std::runtime_error("unknown record type '" + fields[0] + "'");
            }
        } catch (const std::exception& error) {
            throw std::runtime_error("Line " + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    return collection;
}