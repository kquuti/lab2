#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/Reference.h"

/// Контекст (проєкт, каталог). parentId == 0 означає контекст верхнього рівня.
struct Context {
    int id = 0;
    std::string name;
    int parentId = 0;
};

/// Параметри пошуку.
struct SearchQuery {
    std::string text;
    SearchField field = SearchField::All;
    std::optional<ReferenceType> type;  // порожнє означає «будь-який тип»
    int contextId = 0;                  // 0 означає «будь-який контекст»
    bool includeSubcontexts = true;
};

/// Колекція посилань і ієрархія контекстів.
/// Одне посилання може належати до кількох контекстів.
/// Усі помилки використання повідомляються через std::invalid_argument.
class Collection {
public:
    // ---------- Посилання ----------

    /// Додає посилання, призначає йому новий id і повертає його.
    int addReference(Reference reference);

    /// Замінює дані посилання з заданим id.
    void updateReference(int id, Reference reference);

    void removeReference(int id);

    /// Повертає посилання за id або nullptr.
    const Reference* findReference(int id) const;

    const std::vector<Reference>& references() const { return references_; }

    // ---------- Контексти ----------

    /// Додає контекст (parentId == 0 для верхнього рівня) і повертає його id.
    int addContext(const std::string& name, int parentId = 0);

    void renameContext(int id, const std::string& name);

    /// Видаляє контекст разом з усіма підконтекстами. Самі посилання лишаються,
    /// але перестають належати до видалених контекстів.
    void removeContext(int id);

    const Context* findContext(int id) const;

    const std::vector<Context>& contexts() const { return contexts_; }

    /// Прямі нащадки контексту (0 дає контексти верхнього рівня).
    std::vector<Context> childrenOf(int parentId) const;

    /// id контексту та всіх його нащадків (порожній вектор, якщо id невідомий).
    std::vector<int> contextWithDescendants(int id) const;

    /// Повний шлях контексту, наприклад "Study / C++".
    std::string contextPath(int id) const;

    // ---------- Пошук ----------

    /// Повертає id знайдених посилань у порядку додавання.
    std::vector<int> search(const SearchQuery& query) const;

    // ---------- Для завантаження з файлу ----------

    /// Додає контекст із заданим id (використовується при завантаженні).
    void restoreContext(const Context& context);

    /// Додає посилання із заданим id (використовується при завантаженні).
    /// Невідомі контексти в посиланні відкидаються.
    void restoreReference(const Reference& reference);

private:
    Reference* mutableReference(int id);
    Context* mutableContext(int id);
    void validate(const Reference& reference) const;

    std::vector<Reference> references_;
    std::vector<Context> contexts_;
    int nextReferenceId_ = 1;
    int nextContextId_ = 1;
};