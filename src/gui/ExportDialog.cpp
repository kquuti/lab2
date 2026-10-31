#include "gui/ExportDialog.h"

#include <QClipboard>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "gui/Util.h"

ExportDialog::ExportDialog(const Collection& collection, const std::vector<int>& shownIds,
                           QWidget* parent)
    : QDialog(parent),
      collection_(collection),
      shownIds_(shownIds),
      formatters_(createAllFormatters()) {
    setWindowTitle("Export references");
    resize(720, 520);

    formatCombo_ = new QComboBox;
    for (const auto& formatter : formatters_) {
        formatCombo_->addItem(toQ(formatter->name()));
    }

    scopeCombo_ = new QComboBox;
    scopeCombo_->addItem("Currently shown references");
    scopeCombo_->addItem("All references");

    preview_ = new QPlainTextEdit;
    preview_->setReadOnly(true);

    auto* saveButton = new QPushButton("Save to file...");
    auto* copyButton = new QPushButton("Copy to clipboard");
    auto* closeButton = new QPushButton("Close");

    connect(formatCombo_, &QComboBox::currentIndexChanged, this, &ExportDialog::updatePreview);
    connect(scopeCombo_, &QComboBox::currentIndexChanged, this, &ExportDialog::updatePreview);
    connect(saveButton, &QPushButton::clicked, this, &ExportDialog::saveToFile);
    connect(copyButton, &QPushButton::clicked, this, &ExportDialog::copyToClipboard);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto* form = new QFormLayout;
    form->addRow("Style:", formatCombo_);
    form->addRow("Scope:", scopeCombo_);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(saveButton);
    buttonRow->addWidget(copyButton);
    buttonRow->addStretch();
    buttonRow->addWidget(closeButton);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(preview_, 1);
    layout->addLayout(buttonRow);

    updatePreview();
}

const ICitationFormatter& ExportDialog::currentFormatter() const {
    return *formatters_[static_cast<std::size_t>(formatCombo_->currentIndex())];
}

std::string ExportDialog::currentText() const {
    std::vector<const Reference*> references;
    if (scopeCombo_->currentIndex() == 0) {
        for (int id : shownIds_) {
            if (const Reference* reference = collection_.findReference(id)) {
                references.push_back(reference);
            }
        }
    } else {
        for (const Reference& reference : collection_.references()) {
            references.push_back(&reference);
        }
    }
    return formatList(currentFormatter(), references);
}

void ExportDialog::updatePreview() {
    preview_->setPlainText(toQ(currentText()));
}

void ExportDialog::saveToFile() {
    const QString extension = toQ(currentFormatter().fileExtension());
    const QString path = QFileDialog::getSaveFileName(
        this, "Save references", QString("references.") + extension, QString("*.") + extension);
    if (path.isEmpty()) {
        return;
    }
    if (!writeTextFile(path, currentText())) {
        QMessageBox::warning(this, "Export", "Could not write the file.");
    }
}

void ExportDialog::copyToClipboard() {
    QGuiApplication::clipboard()->setText(toQ(currentText()));
}