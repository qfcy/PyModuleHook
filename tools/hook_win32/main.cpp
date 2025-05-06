#include "dllinject.h"
#include "dllinjectgui.h"
#include <QApplication>
#include <QFile>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QFile dllFile(DLL_PATH);
    if (!dllFile.exists()) {
        QMessageBox::critical(nullptr, "DLL Not Found",
                              QString("DLL file not found:\n%1").arg(DLL_PATH));
        return 1;
    }

    DLLInjectGUI w;
    w.show();
    return a.exec();
}
