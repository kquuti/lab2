#pragma once

#include <QDialog>

#include "core/Collection.h"

class QComboBox;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QSpinBox;

/// Форма додавання та редагування одного посилання.
class ReferenceDialog : public QDialog {
public:
    ReferenceDialog(const Collection& collection, QWidget* parent = nullptr);

    /// Заповнює форму даними посилання (для редагування або початкових значень).
    void setReference(const Reference& reference);

    /// Повертає посилання з даними, введеними у формі.
    Reference reference() const;

    /// Перевіряє форму перед закриттям.
    void accept() override;

private:
    const Collection& collection_;
    int id_ = 0;
    QComboBox* typeCombo_;
    QLineEdit* titleEdit_;
    QPlainTextEdit* authorsEdit_;
    QSpinBox* yearSpin_;
    QLineEdit* publisherEdit_;
    QLineEdit* urlEdit_;
    QPlainTextEdit* commentEdit_;
    QListWidget* contextList_;
};