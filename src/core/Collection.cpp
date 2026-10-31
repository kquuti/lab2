#include "core/Collection.h"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <utility>

// ---------- Посилання ----------

void Collection::validate(const Reference& reference) const {
    if (reference.title().empty()) {
        throw std::invalid_argument("Reference: title must not be empty");
    }
    for (int contextId : reference.contextIds()) {
        if (findContext(contextId) == nullptr) {
            throw std::invalid_argument("Reference: unknown context " + std::to_string(contextId));
        }
    }
}

int Collection::addReference(Reference reference) {
    validate(reference);
    reference.setId(nextReferenceId_++);
    references_.push_back(std::move(reference));
    return references_.back().id();
}

void Collection::updateReference(int id, Reference reference) {
    validate(reference);
    Reference* existing = mutableReference(id);
    if (existing == nullptr) {
        throw std::invalid_argument("Collection: unknown reference " + std::to_string(id));
    }
    reference.setId(id);
    *existing = std::move(reference);
}

void Collection::removeReference(int id) {
    const auto it = std::find_if(references_.begin(), references_.end(),
                                 [id](const Reference& r) { return r.id() == id; });
    if (it == references_.end()) {
        throw std::invalid_argument("Collection: unknown reference " + std::to_string(id));
    }
    references_.erase(it);
}

const Reference* Collection::findReference(int id) const {
    for (const Reference& reference : references_) {
        if (reference.id() == id) {
            return &reference;
        }
    }
    return nullptr;
}

Reference* Collection::mutableReference(int id) {
    for (Reference& reference : references_) {
        if (reference.id() == id) {
            return &reference;
        }
    }
    return nullptr;
}

// ---------- Контексти ----------

int Collection::addContext(const std::string& name, int parentId) {
    if (name.empty()) {
        throw std::invalid_argument("Context: name must not be empty");
    }
    if (parentId != 0 && findContext(parentId) == nullptr) {
        throw std::invalid_argument("Context: unknown parent " + std::to_string(parentId));
    }
    Context context;
    context.id = nextContextId_++;
    context.name = name;
    context.parentId = parentId;
    contexts_.push_back(context);
    return context.id;
}

void Collection::renameContext(int id, const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("Context: name must not be empty");
    }
    Context* context = mutableContext(id);
    if (context == nullptr) {
        throw std::invalid_argument("Collection: unknown context " + std::to_string(id));
    }
    context->name = name;
}

void Collection::removeContext(int id) {
    const std::vector<int> doomed = contextWithDescendants(id);
    if (doomed.empty()) {
        throw std::invalid_argument("Collection: unknown context " + std::to_string(id));
    }
    const std::set<int> doomedSet(doomed.begin(), doomed.end());
    contexts_.erase(std::remove_if(contexts_.begin(), contexts_.end(),
                                   [&doomedSet](const Context& c) { return doomedSet.count(c.id) > 0; }),
                    contexts_.end());
    for (Reference& reference : references_) {
        for (int contextId : doomed) {
            reference.removeContext(contextId);
        }
    }
}

const Context* Collection::findContext(int id) const {
    for (const Context& context : contexts_) {
        if (context.id == id) {
            return &context;
        }
    }
    return nullptr;
}

Context* Collection::mutableContext(int id) {
    for (Context& context : contexts_) {
        if (context.id == id) {
            return &context;
        }
    }
    return nullptr;
}

std::vector<Context> Collection::childrenOf(int parentId) const {
    std::vector<Context> result;
    for (const Context& context : contexts_) {
        if (context.parentId == parentId) {
            result.push_back(context);
        }
    }
    return result;
}

std::vector<int> Collection::contextWithDescendants(int id) const {
    std::vector<int> result;
    if (findContext(id) == nullptr) {
        return result;
    }
    result.push_back(id);
    // Список росте під час обходу: нащадки нащадків додаються в кінець.
    for (std::size_t i = 0; i < result.size(); ++i) {
        for (const Context& context : contexts_) {
            if (context.parentId == result[i]) {
                result.push_back(context.id);
            }
        }
    }
    return result;
}

std::string Collection::contextPath(int id) const {
    std::vector<std::string> names;
    const Context* context = findContext(id);
    int guard = 0;  // захист від нескінченного циклу в пошкоджених даних
    while (context != nullptr && guard++ < 1000) {
        names.push_back(context->name);
        context = context->parentId == 0 ? nullptr : findContext(context->parentId);
    }
    std::reverse(names.begin(), names.end());
    std::string result;
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) {
            result += " / ";
        }
        result += names[i];
    }
    return result;
}

// ---------- Пошук ----------

std::vector<int> Collection::search(const SearchQuery& query) const {
    const bool filterByContext = query.contextId != 0;
    std::set<int> allowedContexts;
    if (filterByContext) {
        const std::vector<int> ids = query.includeSubcontexts
                                         ? contextWithDescendants(query.contextId)
                                         : std::vector<int>{query.contextId};
        allowedContexts.insert(ids.begin(), ids.end());
    }

    std::vector<int> result;
    for (const Reference& reference : references_) {
        if (query.type && reference.type() != *query.type) {
            continue;
        }
        if (filterByContext) {
            bool inside = false;
            for (int contextId : reference.contextIds()) {
                if (allowedContexts.count(contextId) > 0) {
                    inside = true;
                    break;
                }
            }
            if (!inside) {
                continue;
            }
        }
        if (!reference.matches(query.text, query.field)) {
            continue;
        }
        result.push_back(reference.id());
    }
    return result;
}

// ---------- Для завантаження з файлу ----------

void Collection::restoreContext(const Context& context) {
    if (context.id <= 0) {
        throw std::invalid_argument("Context: bad id");
    }
    if (context.name.empty()) {
        throw std::invalid_argument("Context: name must not be empty");
    }
    if (findContext(context.id) != nullptr) {
        throw std::invalid_argument("Context: duplicate id " + std::to_string(context.id));
    }
    if (context.parentId != 0 && findContext(context.parentId) == nullptr) {
        throw std::invalid_argument("Context: unknown parent " + std::to_string(context.parentId));
    }
    contexts_.push_back(context);
    nextContextId_ = std::max(nextContextId_, context.id + 1);
}

void Collection::restoreReference(const Reference& reference) {
    if (reference.id() <= 0) {
        throw std::invalid_argument("Reference: bad id");
    }
    if (reference.title().empty()) {
        throw std::invalid_argument("Reference: title must not be empty");
    }
    if (findReference(reference.id()) != nullptr) {
        throw std::invalid_argument("Reference: duplicate id " + std::to_string(reference.id()));
    }
    std::set<int> knownContexts;
    for (int contextId : reference.contextIds()) {
        if (findContext(contextId) != nullptr) {
            knownContexts.insert(contextId);
        }
    }
    Reference copy = reference;
    copy.setContextIds(knownContexts);
    references_.push_back(std::move(copy));
    nextReferenceId_ = std::max(nextReferenceId_, reference.id() + 1);
}