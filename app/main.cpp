#include <QHostInfo>
#include <QApplication>

#include "PhotoScroller.hpp"

#define SELECTOR 1

#if SELECTOR == 1

int main( int argc, char* argv[] )
{
  QApplication app( argc, argv );

  QString p = "/home/user/Pictures/cosplay/bulma_bunny_girl/";
  PhotoScroller* ps = argc > 1 ? new PhotoScroller(argv[1]) : new PhotoScroller(p);
  ps->setWindowState(Qt::WindowMaximized | Qt::WindowFullScreen);
  /*
  PhotoScroller* ps;
  if (argc > 1) {
    ps = new PhotoScroller( argv[1], QString(argv[2]).toInt() );
  } else {
    ps = new PhotoScroller;
  }
  */

  //ps->setWindowState(Qt::WindowFullScreen);
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