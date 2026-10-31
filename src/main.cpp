#include <QApplication>
#include <QCoreApplication>

#include "gui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("StudyProjects");
    QCoreApplication::setApplicationName("ReferenceCollection");

    MainWindow window;
    window.show();
    return app.exec();
}