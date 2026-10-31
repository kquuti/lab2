#include "gui/MainWindow.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <sstream>

#include "core/Storage.h"
#include "gui/ContextDialog.h"
#include "gui/ExportDialog.h"
#include "gui/ReferenceDialog.h"
#include "gui/Util.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), formatters_(createAllFormatters()) {
    setWindowTitle("Reference Collection");
    resize(1050, 650);

    buildUi();
    buildMenu();
    connectSignals();

    loadFromDisk();
    rebuildContextTree();
}

// ---------- Побудова інтерфейсу ----------

void MainWindow::buildUi() {
    contextTree_ = new QTreeWidget;
    contextTree_->setHeaderLabel("Contexts");

    searchEdit_ = new QLineEdit;
    searchEdit_->setPlaceholderText("Search...");
    searchEdit_->setClearButtonEnabled(true);

    fieldCombo_ = new QComboBox;
    fieldCombo_->addItem("Everywhere", static_cast<int>(SearchField::All));
    fieldCombo_->addItem("Title", static_cast<int>(SearchField::Title));
    fieldCombo_->addItem("Author", static_cast<int>(SearchField::Author));
    fieldCombo_->addItem("Year", static_cast<int>(SearchField::Year));
    fieldCombo_->addItem("Comment", static_cast<int>(SearchField::Comment));

    typeCombo_ = new QComboBox;
    typeCombo_->addItem("All types", -1);
    for (int i = 0; i < kReferenceTypeCount; ++i) {
        typeCombo_->addItem(toQ(typeName(typeFromInt(i))), i);
    }

    subCheck_ = new QCheckBox("Include subcontexts");
    subCheck_->setChecked(true);

    table_ = new QTableWidget(0, 4);
    QStringList headers;
    headers << "Title" << "Authors" << "Year" << "Type";
    table_->setHorizontalHeaderLabels(headers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setColumnWidth(0, 300);
    table_->setColumnWidth(1, 220);
    table_->setColumnWidth(2, 60);

    addButton_ = new QPushButton("Add...");
    editButton_ = new QPushButton("Edit...");
    deleteButton_ = new QPushButton("Delete");
    contextsButton_ = new QPushButton("Contexts...");
    exportButton_ = new QPushButton("Export...");

    styleCombo_ = new QComboBox;
    for (const auto& formatter : formatters_) {
        styleCombo_->addItem(toQ(formatter->name()));
    }

    preview_ = new QPlainTextEdit;
    preview_->setReadOnly(true);
    preview_->setMaximumHeight(170);

    auto* searchRow = new QHBoxLayout;
    searchRow->addWidget(searchEdit_, 1);
    searchRow->addWidget(fieldCombo_);
    searchRow->addWidget(typeCombo_);
    searchRow->addWidget(subCheck_);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(addButton_);
    buttonRow->addWidget(editButton_);
    buttonRow->addWidget(deleteButton_);
    buttonRow->addWidget(contextsButton_);
    buttonRow->addStretch();
    buttonRow->addWidget(exportButton_);

    auto* styleRow = new QHBoxLayout;
    styleRow->addWidget(new QLabel("Citation style:"));
    styleRow->addWidget(styleCombo_, 1);

    auto* right = new QWidget;
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->addLayout(searchRow);
    rightLayout->addWidget(table_, 1);
    rightLayout->addLayout(buttonRow);
    rightLayout->addLayout(styleRow);
    rightLayout->addWidget(preview_);

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(contextTree_);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    setCentralWidget(splitter);
}

void MainWindow::buildMenu() {
    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction* saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    QAction* quitAction = fileMenu->addAction("&Quit");
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveToDisk);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::connectSignals() {
    connect(searchEdit_, &QLineEdit::textChanged, this, &MainWindow::refreshTable);
    connect(fieldCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::refreshTable);
    connect(typeCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::refreshTable);
    connect(subCheck_, &QCheckBox::toggled, this, &MainWindow::refreshTable);
    connect(contextTree_, &QTreeWidget::currentItemChanged, this, &MainWindow::refreshTable);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &MainWindow::updatePreview);
    connect(table_, &QTableWidget::itemDoubleClicked, this, &MainWindow::editReference);
    connect(styleCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::updatePreview);
    connect(addButton_, &QPushButton::clicked, this, &MainWindow::addReference);
    connect(editButton_, &QPushButton::clicked, this, &MainWindow::editReference);
    connect(deleteButton_, &QPushButton::clicked, this, &MainWindow::deleteReference);
    connect(contextsButton_, &QPushButton::clicked, this, &MainWindow::manageContexts);
    connect(exportButton_, &QPushButton::clicked, this, &MainWindow::exportReferences);
}

// ---------- Збереження ----------

QString MainWindow::dataFilePath() const {
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(directory);
    return directory + "/collection.refs";
}

void MainWindow::loadFromDisk() {
    const QString path = dataFilePath();
    if (!QFile::exists(path)) {
        return;
    }
    std::string content;
    if (!readTextFile(path, content)) {
        QMessageBox::warning(this, "Load", QString("Could not read ") + path);
        return;
    }
    try {
        std::istringstream in(content);
        collection_ = loadCollection(in);
    } catch (const std::exception& error) {
        QFile::copy(path, path + QString(".bak"));
        QMessageBox::warning(
            this, "Load",
            QString("The data file is damaged (%1).\nA backup copy was saved as %2.bak")
                .arg(toQ(error.what()), path));
    }
}

void MainWindow::saveToDisk() {
    std::ostringstream out;
    saveCollection(collection_, out);
    if (writeTextFile(dataFilePath(), out.str())) {
        statusBar()->showMessage("Saved", 2000);
    } else {
        QMessageBox::warning(this, "Save", "Could not write the data file.");
    }
}

// ---------- Дерево контекстів ----------

int MainWindow::selectedContextId() const {
    const QTreeWidgetItem* item = contextTree_->currentItem();
    return item ? item->data(0, Qt::UserRole).toInt() : 0;
}

void MainWindow::selectContext(int id) {
    QTreeWidgetItemIterator it(contextTree_);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            contextTree_->setCurrentItem(*it);
            return;
        }
        ++it;
    }
    contextTree_->setCurrentItem(contextTree_->topLevelItem(0));
}

void MainWindow::rebuildContextTree() {
    const int keep = selectedContextId();
    {
        QSignalBlocker blocker(contextTree_);
        contextTree_->clear();
        auto* root = new QTreeWidgetItem(contextTree_);
        root->setText(0, "All references");
        root->setData(0, Qt::UserRole, 0);
        addContextItems(collection_, contextTree_, root, 0);
        contextTree_->expandAll();
        selectContext(keep);
    }
    refreshTable();
}

// ---------- Таблиця ----------

int MainWindow::selectedReferenceId() const {
    const int row = table_->currentRow();
    if (row < 0) {
        return 0;
    }
    const QTableWidgetItem* item = table_->item(row, 0);
    return item ? item->data(Qt::UserRole).toInt() : 0;
}

void MainWindow::selectReference(int id) {
    for (int row = 0; row < table_->rowCount(); ++row) {
        if (table_->item(row, 0)->data(Qt::UserRole).toInt() == id) {
            table_->selectRow(row);
            return;
        }
    }
}

void MainWindow::refreshTable() {
    SearchQuery query;
    query.text = fromQ(searchEdit_->text().trimmed());
    query.field = static_cast<SearchField>(fieldCombo_->currentData().toInt());
    const int typeValue = typeCombo_->currentData().toInt();
    if (typeValue >= 0) {
        query.type = typeFromInt(typeValue);
    }
    query.contextId = selectedContextId();
    query.includeSubcontexts = subCheck_->isChecked();

    shownIds_ = collection_.search(query);

    table_->setSortingEnabled(false);
    table_->setRowCount(0);
    for (int id : shownIds_) {
        const Reference* reference = collection_.findReference(id);
        if (reference == nullptr) {
            continue;
        }
        const int row = table_->rowCount();
        table_->insertRow(row);

        auto* titleItem = new QTableWidgetItem(toQ(reference->title()));
        titleItem->setData(Qt::UserRole, reference->id());
        table_->setItem(row, 0, titleItem);

        QStringList authors;
        for (const std::string& author : reference->authors()) {
            authors << toQ(author);
        }
        table_->setItem(row, 1, new QTableWidgetItem(authors.join(", ")));

        auto* yearItem = new QTableWidgetItem;
        if (reference->year() > 0) {
            yearItem->setData(Qt::DisplayRole, reference->year());
        }
        table_->setItem(row, 2, yearItem);

        table_->setItem(row, 3, new QTableWidgetItem(toQ(typeName(reference->type()))));
    }
    table_->setSortingEnabled(true);

    statusBar()->showMessage(QString("Shown %1 of %2 references")
                                 .arg(shownIds_.size())
                                 .arg(collection_.references().size()));
    updatePreview();
}

void MainWindow::updatePreview() {
    const Reference* reference = collection_.findReference(selectedReferenceId());
    editButton_->setEnabled(reference != nullptr);
    deleteButton_->setEnabled(reference != nullptr);
    if (reference == nullptr) {
        preview_->clear();
        return;
    }

    const ICitationFormatter& formatter =
        *formatters_[static_cast<std::size_t>(styleCombo_->currentIndex())];
    QString text = toQ(formatter.format(*reference));
    if (!reference->comment().empty()) {
        text += QString("\n\nComment: ") + toQ(reference->comment());
    }
    QStringList contexts;
    for (int contextId : reference->contextIds()) {
        contexts << toQ(collection_.contextPath(contextId));
    }
    if (!contexts.isEmpty()) {
        text += QString("\n\nContexts: ") + contexts.join("; ");
    }
    preview_->setPlainText(text);
}

// ---------- Дії користувача ----------

void MainWindow::addReference() {
    ReferenceDialog dialog(collection_, this);
    Reference preset;
    if (selectedContextId() != 0) {
        preset.addContext(selectedContextId());
    }
    dialog.setReference(preset);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    try {
        const int id = collection_.addReference(dialog.reference());
        saveToDisk();
        refreshTable();
        selectReference(id);
    } catch (const std::exception& error) {
        QMessageBox::warning(this, "Error", toQ(error.what()));
    }
}

void MainWindow::editReference() {
    const Reference* current = collection_.findReference(selectedReferenceId());
    if (current == nullptr) {
        return;
    }
    const int id = current->id();
    ReferenceDialog dialog(collection_, this);
    dialog.setReference(*current);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    try {
        collection_.updateReference(id, dialog.reference());
        saveToDisk();
        refreshTable();
        selectReference(id);
    } catch (const std::exception& error) {
        QMessageBox::warning(this, "Error", toQ(error.what()));
    }
}

void MainWindow::deleteReference() {
    const Reference* current = collection_.findReference(selectedReferenceId());
    if (current == nullptr) {
        return;
    }
    const int id = current->id();
    const auto answer = QMessageBox::question(
        this, "Delete reference", QString("Delete \"%1\"?").arg(toQ(current->title())),
        QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    try {
        collection_.removeReference(id);
        saveToDisk();
        refreshTable();
    } catch (const std::exception& error) {
        QMessageBox::warning(this, "Error", toQ(error.what()));
    }
}

void MainWindow::manageContexts() {
    ContextDialog dialog(collection_, this);
    dialog.exec();
    saveToDisk();
    rebuildContextTree();
}

void MainWindow::exportReferences() {
    ExportDialog dialog(collection_, shownIds_, this);
    dialog.exec();
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About",
                       QString("Reference Collection\nLab work 2, 2025-2026.\n\nData file:\n") +
                           dataFilePath());
}