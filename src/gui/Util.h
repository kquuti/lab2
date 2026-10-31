#pragma once

#include <QByteArray>
#include <QFile>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <string>

#include "core/Collection.h"

/// std::string (UTF-8) -> QString.
inline QString toQ(const std::string& text) {
    return QString::fromUtf8(text.c_str(), static_cast<qsizetype>(text.size()));
}

/// QString -> std::string (UTF-8).
inline std::string fromQ(const QString& text) {
    return text.toUtf8().toStdString();
}

inline bool writeTextFile(const QString& path, const std::string& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const QByteArray bytes(content.data(), static_cast<qsizetype>(content.size()));
    return file.write(bytes) == bytes.size();
}

inline bool readTextFile(const QString& path, std::string& content) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray bytes = file.readAll();
    content.assign(bytes.constData(), static_cast<std::size_t>(bytes.size()));
    return true;
}

/// Рекурсивно додає в дерево контексти, які є нащадками parentId.
/// id контексту зберігається в Qt::UserRole першого стовпця.
inline void addContextItems(const Collection& collection, QTreeWidget* tree,
                            QTreeWidgetItem* parentItem, int parentId) {
    for (const Context& context : collection.childrenOf(parentId)) {
        QTreeWidgetItem* item =
            parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(tree);
        item->setText(0, toQ(context.name));
        item->setData(0, Qt::UserRole, context.id);
        addContextItems(collection, tree, item, context.id);
    }
}