#include "qNBA.h"

#include <QDate>
#include <QProcess>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qNBA w;
    w.showMaximized();
    return a.exec();
}
