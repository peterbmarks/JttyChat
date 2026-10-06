#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("JttyChat");
    QApplication::setOrganizationName("JttyChat");

    MainWindow window;
    window.show();

    return app.exec();
}
