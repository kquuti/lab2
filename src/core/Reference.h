#pragma once

#include <set>
#include <string>
#include <vector>

/// Тип ресурсу.
enum class ReferenceType { Book = 0, Web = 1, Video = 2, Audio = 3 };

constexpr int kReferenceTypeCount = 4;

/// Де шукати текст.
enum class SearchField { All = 0, Title = 1, Author = 2, Year = 3, Comment = 4 };

/// Назва типу для інтерфейсу.
std::string typeName(ReferenceType type);

/// Перетворює число 0..3 на тип. Кидає std::invalid_argument для іншого значення.
ReferenceType typeFromInt(int value);

/// Чи містить haystack підрядок needle без урахування регістру (лише для ASCII-літер).
bool containsIgnoreCase(const std::string& haystack, const std::string& needle);

/// Одне посилання на ресурс (книга, веб-сторінка, відео, аудіо).
class Reference {
public:
    Reference() = default;
    Reference(ReferenceType type, const std::string& title) : type_(type), title_(title) {}

    int id() const { return id_; }
    ReferenceType type() const { return type_; }
    const std::string& title() const { return title_; }
    const std::vector<std::string>& authors() const { return authors_; }
    int year() const { return year_; }  // 0 означає «невідомо»
    const std::string& publisher() const { return publisher_; }
    const std::string& url() const { return url_; }
    const std::string& comment() const { return comment_; }
    const std::set<int>& contextIds() const { return contextIds_; }

    void setId(int id) { id_ = id; }
    void setType(ReferenceType type) { type_ = type; }
    void setTitle(const std::string& title) { title_ = title; }
    void setAuthors(const std::vector<std::string>& authors) { authors_ = authors; }
    void setYear(int year) { year_ = year; }
    void setPublisher(const std::string& publisher) { publisher_ = publisher; }
    void setUrl(const std::string& url) { url_ = url; }
    void setComment(const std::string& comment) { comment_ = comment; }
    void setContextIds(const std::set<int>& ids) { contextIds_ = ids; }

    void addContext(int contextId) { contextIds_.insert(contextId); }
    void removeContext(int contextId) { contextIds_.erase(contextId); }
    bool inContext(int contextId) const { return contextIds_.count(contextId) > 0; }

    /// Чи містить посилання text у вибраному полі. Порожній text підходить завжди.
    bool matches(const std::string& text, SearchField field) const;

private:
    int id_ = 0;
    ReferenceType type_ = ReferenceType::Book;
    std::string title_;
    std::vector<std::string> authors_;
    int year_ = 0;
    std::string publisher_;
    std::string url_;
    std::string comment_;
    std::set<int> contextIds_;
};