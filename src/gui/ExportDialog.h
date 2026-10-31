#pragma once

#include <QDialog>
#include <memory>
#include <string>
#include <vector>

#include "core/Citation.h"
#include "core/Collection.h"

class QComboBox;
class QPlainTextEdit;

/// Форма експорту списку посилань: вибір стандарту, попередній перегляд, файл чи буфер обміну.
class ExportDialog : public QDialog {
public:
    ExportDialog(const Collection& collection, const std::vector<int>& shownIds,
                 QWidget* parent = nullptr);

private:
    std::string currentText() const;
    const ICitationFormatter& currentFormatter() const;
    void updatePreview();
    void saveToFile();
    void copyToClipboard();

    const Collection& collection_;
    std::vector<int> shownIds_;
    std::vector<std::unique_ptr<ICitationFormatter>> formatters_;
    QComboBox* formatCombo_;
    QComboBox* scopeCombo_;
    QPlainTextEdit* preview_;
};