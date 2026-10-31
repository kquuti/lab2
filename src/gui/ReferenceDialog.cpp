#include "gui/ReferenceDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSpinBox>

#include "gui/Util.h"

ReferenceDialog::ReferenceDialog(const Collection& collection, QWidget* parent)
    : QDialog(parent), collection_(collection) {
    setWindowTitle("Reference");
    setMinimumWidth(480);

    typeCombo_ = new QComboBox;
    for (int i = 0; i < kReferenceTypeCount; ++i) {
        typeCombo_->addItem(toQ(typeName(typeFromInt(i))), i);
    }

    titleEdit_ = new QLineEdit;

    authorsEdit_ = new QPlainTextEdit;
    authorsEdit_->setPlaceholderText("One author per line: Surname Name Patronymic");
    authorsEdit_->setMaximumHeight(80);

    yearSpin_ = new QSpinBox;
    yearSpin_->setRange(0, 2100);
    yearSpin_->setSpecialValueText("unknown");

    publisherEdit_ = new QLineEdit;
    urlEdit_ = new QLineEdit;

    commentEdit_ = new QPlainTextEdit;
    commentEdit_->setMaximumHeight(80);

    contextList_ = new QListWidget;
    contextList_->setMaximumHeight(120);
    for (const Context& context : collection_.contexts()) {
        auto* item = new QListWidgetItem(toQ(collection_.contextPath(context.id)), contextList_);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setData(Qt::UserRole, context.id);
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* form = new QFormLayout(this);
    form->addRow("Type:", typeCombo_);
    form->addRow("Title:", titleEdit_);
    form->addRow("Authors:", authorsEdit_);
    form->addRow("Year:", yearSpin_);
    form->addRow("Publisher / site:", publisherEdit_);
    form->addRow("URL:", urlEdit_);
    form->addRow("Comment:", commentEdit_);
    form->addRow("Contexts:", contextList_);
    form->addRow(buttons);
}

void ReferenceDialog::setReference(const Reference& reference) {
    id_ = reference.id();
    typeCombo_->setCurrentIndex(typeCombo_->findData(static_cast<int>(reference.type())));
    titleEdit_->setText(toQ(reference.title()));

    QStringList authors;
    for (const std::string& author : reference.authors()) {
        authors << toQ(author);
    }
    authorsEdit_->setPlainText(authors.join("\n"));

    yearSpin_->setValue(reference.year());
    publisherEdit_->setText(toQ(reference.publisher()));
    urlEdit_->setText(toQ(reference.url()));
    commentEdit_->setPlainText(toQ(reference.comment()));

    for (int i = 0; i < contextList_->count(); ++i) {
        QListWidgetItem* item = contextList_->item(i);
        const bool member = reference.inContext(item->data(Qt::UserRole).toInt());
        item->setCheckState(member ? Qt::Checked : Qt::Unchecked);
    }
}

Reference ReferenceDialog::reference() const {
    Reference result(typeFromInt(typeCombo_->currentData().toInt()),
                     fromQ(titleEdit_->text().trimmed()));
    result.setId(id_);

    std::vector<std::string> authors;
    const QStringList lines = authorsEdit_->toPlainText().split(QChar('\n'));
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            authors.push_back(fromQ(trimmed));
        }
    }
    result.setAuthors(authors);

    result.setYear(yearSpin_->value());
    result.setPublisher(fromQ(publisherEdit_->text().trimmed()));
    result.setUrl(fromQ(urlEdit_->text().trimmed()));
    result.setComment(fromQ(commentEdit_->toPlainText().trimmed()));

    for (int i = 0; i < contextList_->count(); ++i) {
        const QListWidgetItem* item = contextList_->item(i);
        if (item->checkState() == Qt::Checked) {
            result.addContext(item->data(Qt::UserRole).toInt());
        }
    }
    return result;
}

void ReferenceDialog::accept() {
    if (titleEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Reference", "Title must not be empty.");
        titleEdit_->setFocus();
        return;
    }
    QDialog::accept();
}