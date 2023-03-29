#include <QApplication>
#include <iostream>

#include "main_window.hpp"

MainWindow* CreateWindow(int argc, char* argv[]) {
  QString default_folder = "/home/user/Pictures/test_folder/";
  if (argc > 1 && QFileInfo(argv[1]).isDir()) {
    int start = argc == 3 ? QString(argv[2]).toInt() : 0;
    return new MainWindow(argv[1], start);
  } else if (argc > 1) {
    QList<QString> images;
    for (int i = 1; i < argc; ++i) {
      QString filename = argv[i];
      QFileInfo info(filename);
      if (!info.isFile()) {
        std::cout << '\'' << filename.toStdString() << '\''
                  << " is not a file\nAborting..." << std::endl;
        exit(1);
      }
      QString s = info.suffix();
      if (s == "jpg" || s == "png" || s == "jpeg") {
        images.append(filename);
      }
    }
    return new MainWindow(images);
  } else {
    return new MainWindow(default_folder);
  }
}

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  MainWindow* ps = CreateWindow(argc, argv);
  ps->setWindowState(Qt::WindowMaximized | Qt::WindowFullScreen);
  ps->show();
  return app.exec();
}