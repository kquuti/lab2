#pragma once

#include <iosfwd>

#include "core/Collection.h"

/// Записує колекцію в текстовий потік (власний простий формат, поля розділені табуляцією).
void saveCollection(const Collection& collection, std::ostream& out);

/// Читає колекцію з потоку. Кидає std::runtime_error з номером рядка, якщо формат хибний.
Collection loadCollection(std::istream& in);