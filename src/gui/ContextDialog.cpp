#include "gui/ContextDialog.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "gui/Util.h"

ContextDialog::ContextDialog(Collection& collection, QWidget* parent)
    : QDialog(parent), collection_(collection) {
    setWindowTitle("Contexts");
    resize(380, 420);

    tree_ = new QTreeWidget;
    tree_->setHeaderLabel("Contexts");

    auto* addRootButton = new QPushButton("Add top-level");
    auto* addChildButton = new QPushButton("Add child");
    auto* renameButton = new QPushButton("Rename");
    auto* deleteButton = new QPushButton("Delete");
    auto* closeButton = new QPushButton("Close");

    connect(addRootButton, &QPushButton::clicked, this, &ContextDialog::addRoot);
    connect(addChildButton, &QPushButton::clicked, this, &ContextDialog::addChild);
    connect(renameButton, &QPushButton::clicked, this, &ContextDialog::renameSelected);
    connect(deleteButton, &QPushButton::clicked, this, &ContextDialog::removeSelected);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(addRootButton);
    buttonRow->addWidget(addChildButton);
    buttonRow->addWidget(renameButton);
    buttonRow->addWidget(deleteButton);
    buttonRow->addStretch();
    buttonRow->addWidget(closeButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(tree_, 1);
    layout->addLayout(buttonRow);

    rebuild();
}

void ContextDialog::rebuild() {
    tree_->clear();
    addContextItems(collection_, tree_, nullptr, 0);
    tree_->expandAll();
}

int ContextDialog::selectedId() const {
    const QTreeWidgetItem* item = tree_->currentItem();
    return item ? item->data(0, Qt::UserRole).toInt() : 0;
}

void ContextDialog::run(const std::function<void()>& action) {
    try {
        action();
    } catch (const std::exception& error) {
        QMessageBox::warning(this, "Contexts", toQ(error.what()));
    }
    rebuild();
}

void ContextDialog::createContext(int parentId) {
    bool ok = false;
    const QString name =
        QInputDialog::getText(this, "New context", "Name:", QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) {
        return;
    }
    run([&]() { collection_.addContext(fromQ(name.trimmed()), parentId); });
}

void ContextDialog::addRoot() {
    createContext(0);
}

void ContextDialog::addChild() {
    const int parentId = selectedId();
    if (parentId == 0) {
        QMessageBox::information(this, "Contexts", "Select a parent context first.");
        return;
    }
    createContext(parentId);
}

void ContextDialog::renameSelected() {
    const int id = selectedId();
    if (id == 0) {
        QMessageBox::information(this, "Contexts", "Select a context first.");
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Rename context", "Name:", QLineEdit::Normal,
                                               tree_->currentItem()->text(0), &ok);
    if (!ok || name.trimmed().isEmpty()) {
        return;
    }
    run([&]() { collection_.renameContext(id, fromQ(name.trimmed())); });
}

void ContextDialog::removeSelected() {
    const int id = selectedId();
    if (id == 0) {
        QMessageBox::information(this, "Contexts", "Select a context first.");
        return;
    }
    const auto answer = QMessageBox::question(
        this, "Delete context",
        "Delete this context and all its subcontexts?\nThe references themselves will stay.",
        QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    run([&]() { collection_.removeContext(id); });
}