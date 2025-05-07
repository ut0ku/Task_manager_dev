#include "mainwindow.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    QTranslator translator;
    if (translator.load(QLocale(), QLatin1String("taskmanager"), QLatin1String("_"), QLatin1String(":/translations"))) {
        a.installTranslator(&translator);
    }
    
    MainWindow w;
    w.show();
    return a.exec();
}
