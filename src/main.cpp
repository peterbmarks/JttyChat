#include "HamlibLog.h"
#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("JttyChat");
    QApplication::setOrganizationName("JttyChat");

    // Capture Hamlib's debug/trace output (see File > Hamlib Log) before
    // any rig_* call runs, so nothing is missed.
    HamlibLog::instance().install();

    MainWindow window;
    window.show();

    return app.exec();
}
