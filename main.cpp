#include "mainwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  QFile styleFile(":/styles/style.qss");
  if (styleFile.open(QFile::ReadOnly)) {
    QString styleSheet = QLatin1String(styleFile.readAll());
    a.setStyleSheet(styleSheet);
    styleFile.close();
  } else {
    qWarning() << "Failed to open :/styles/style.qss" << styleFile.error();
  }
  MainWindow w;
  w.show();
  return a.exec();
}
