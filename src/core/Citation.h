#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/Reference.h"

/// Прізвище автора: перше слово рядка (автори вводяться як «Прізвище Ім'я По-батькові»).
std::string authorSurname(const std::string& author);

/// Ініціали автора (перші літери всіх слів, крім першого), розділені separator.
/// Наприклад "Іваненко Іван Петрович" з роздільником " " дає "І. П.".
std::string authorInitials(const std::string& author, const std::string& separator);

/// Інтерфейс форматування посилання за певним стандартом оформлення.
class ICitationFormatter {
public:
    virtual ~ICitationFormatter() = default;

    virtual std::string name() const = 0;
    virtual std::string format(const Reference& reference) const = 0;

    /// Чи нумерувати записи у списку.
    virtual bool isNumbered() const { return true; }

    /// Розширення файлу для експорту (без крапки).
    virtual std::string fileExtension() const { return "txt"; }
};

/// ДСТУ ГОСТ 7.1:2006 (спрощена форма).
class GostFormatter : public ICitationFormatter {
public:
    std::string name() const override;
    std::string format(const Reference& reference) const override;
};

/// ДСТУ 8302:2015 (спрощена форма).
class Dstu8302Formatter : public ICitationFormatter {
public:
    std::string name() const override;
    std::string format(const Reference& reference) const override;
};

/// Harvard.
class HarvardFormatter : public ICitationFormatter {
public:
    std::string name() const override;
    std::string format(const Reference& reference) const override;
};

/// BibTeX.
class BibTexFormatter : public ICitationFormatter {
public:
    std::string name() const override;
    std::string format(const Reference& reference) const override;
    bool isNumbered() const override { return false; }
    std::string fileExtension() const override { return "bib"; }
};

/// Створює по одному екземпляру кожного форматувальника.
std::vector<std::unique_ptr<ICitationFormatter>> createAllFormatters();

/// Форматує список посилань: кожен запис з нового рядка (BibTeX розділяється порожніми рядками).
std::string formatList(const ICitationFormatter& formatter,
                       const std::vector<const Reference*>& references);