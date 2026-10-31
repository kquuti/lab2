#pragma once

#include <QMainWindow>
#include <memory>
#include <vector>

#include "core/Citation.h"
#include "core/Collection.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTreeWidget;

/// Головне вікно: дерево контекстів, таблиця посилань, пошук, попередній перегляд цитування.
class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void buildUi();
    void buildMenu();
    void connectSignals();

    QString dataFilePath() const;
    void loadFromDisk();
    void saveToDisk();

    void rebuildContextTree();
    void selectContext(int id);
    int selectedContextId() const;

    void refreshTable();
    void selectReference(int id);
    int selectedReferenceId() const;
    void updatePreview();

    void addReference();
    void editReference();
    void deleteReference();
    void manageContexts();
    void exportReferences();
    void showAbout();

    Collection collection_;
    std::vector<std::unique_ptr<ICitationFormatter>> formatters_;
    std::vector<int> shownIds_;

    QTreeWidget* contextTree_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QComboBox* fieldCombo_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QCheckBox* subCheck_ = nullptr;
    QTableWidget* table_ = nullptr;
    QPushButton* addButton_ = nullptr;
    QPushButton* editButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    QPushButton* contextsButton_ = nullptr;
    QPushButton* exportButton_ = nullptr;
    QComboBox* styleCombo_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
};