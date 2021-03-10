#include <QApplication>

#include "scroller.h"
#include <iostream>

#define SELECTOR 1

Scroller* CreateScroller(int argc, char* argv[]) {
  QString default_folder = "/home/user/Pictures/cosplay/cosplay1/";
  if (argc > 1 && QFileInfo(argv[1]).isDir()) {
    int start = argc == 3 ? QString(argv[2]).toInt() : 0;
    return new Scroller(argv[1], start);
  } else if (argc > 1) {
    QList<QString> images;
    for (int i = 1; i < argc; ++i) {
      QString filename = argv[i];
      QFileInfo info(filename);
      if (!info.isFile()) {
        std::cout << '\'' << filename.toStdString() << '\'' << " is not a file\nAborting..." << std::endl;
        exit(1);
      }
      QString s = info.suffix();
      if (s == "jpg" || s == "png" || s == "jpeg") {
        images.append(filename);
      }
    }
    return new Scroller(images);
  } else {
    return new Scroller(default_folder);
  }
}

#if SELECTOR == 1

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  Scroller* ps = CreateScroller(argc, argv);
  ps->setWindowState(Qt::WindowMaximized | Qt::WindowFullScreen);
  ps->show();
  return app.exec();
}

#elif SELECTOR == 2

#include "DownloadingDialog.hpp"
int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  DownloadingDialog widget;
  QPushButton step("step");
  QPushButton success("success");
  int pvalue = 78;

  QObject::connect(&success, &QPushButton::clicked, [&] {
    widget.displaySuccess();
  });
  
  QObject::connect(&step, &QPushButton::clicked, [&] {
    ++pvalue;
    widget.setProgressValue(pvalue);
  });
  widget.prepareDialog();
  widget.show();
  step.show();
  success.show();
  return app.exec();
}

#elif SELECTOR == 3

#include "DownloadingDialog.hpp"
int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  DownloadingDialog widget;
  widget.prepareDialog();
  widget.show();


  return app.exec();
}

#elif SELECTOR == 4

#include "PhotoViewer.hpp"

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);

  PhotoViewer* pv = new PhotoViewer;
  QString p = "/home/user/Pictures/cosplay/bulma_bunny_girl/";
  pv->prepareWidget(p);
  pv->goTo(NextImage);
  QScrollArea* ps = new QScrollArea;
  ps->setWidget(pv);
  ps->setFocusPolicy(Qt::StrongFocus);
  ps->show();

  
  return app.exec();
}

#endif
