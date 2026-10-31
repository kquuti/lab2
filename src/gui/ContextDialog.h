#pragma once

#include <QDialog>
#include <functional>

#include "core/Collection.h"

class QTreeWidget;

/// Форма керування деревом контекстів: додати, перейменувати, видалити.
class ContextDialog : public QDialog {
public:
    ContextDialog(Collection& collection, QWidget* parent = nullptr);

private:
    void rebuild();
    int selectedId() const;
    void createContext(int parentId);
    void addRoot();
    void addChild();
    void renameSelected();
    void removeSelected();

    /// Виконує зміну колекції, показує помилку (якщо є) і перебудовує дерево.
    void run(const std::function<void()>& action);

    Collection& collection_;
    QTreeWidget* tree_;
};