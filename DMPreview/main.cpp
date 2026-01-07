#include "mainwindow.h"
#include <QApplication>
#include <QMainWindow>
#include "eSPDI_version.h"
#include "CPointCloudViewerWidget.h"
#include "ColorPaletteConfig.h"

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    // Load color palette configuration at startup
    ColorPaletteConfig::GetInstance()->LoadConfig();

    MainWindow mainWindow;
    //mainWindow.setWindowIcon(QIcon(":/image/eys3d.png"));
    QString title = "DMPreview ";
    title += APC_VERSION;
    mainWindow.setWindowTitle(title);
    mainWindow.show();

    return app.exec();
}
