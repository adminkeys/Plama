#include "gui/windowmain.h"
#include "util.h"
#include <Python.h>
#include <QApplication>
#include <QDebug>
#include <QFont>
#include <QSurfaceFormat>
#include <QSysInfo>
#include <data/project.h>

// soft code file name
// actual model size

int main(int argc, char *argv[]) {

    Py_Initialize();

    int ret;
    {
        QSurfaceFormat format;

        format.setSamples(2);
        format.setSwapInterval(0);
        format.setDepthBufferSize(16);
        format.setRenderableType(QSurfaceFormat::OpenGL);
        QSurfaceFormat::setDefaultFormat(format);

        QApplication a(argc, argv);
        if (QSysInfo::productType() == "windows") {
            QFont font("Segoe UI");
            a.setFont(font);
        }

        WindowMain w(nullptr);
        w.show();
        ret = a.exec();
    }
    Py_Finalize();
    return ret;
}
