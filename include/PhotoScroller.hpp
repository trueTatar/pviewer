#pragma once

#include <QScrollArea>
#include <map>
#include <functional>

#include "SelectingDialog.hpp"
#include "PictureDownloader.hpp"

#include "PhotoViewer.hpp"
class PhotoViewer;
class QTimer;
class QScreen;
class QMoveEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class ResetScrollBar : public QObject {
  Q_OBJECT
private:
  std::map<QScrollBar*, bool> s;
public:
  ResetScrollBar(QObject* pobj, QScrollBar* h, QScrollBar* v) : QObject(pobj) {
    s.emplace(h, true);
    s.emplace(v, true);
  }
  void operator!() {
    for (auto& i : s) {
      i.second = !i.second;
    }
  }
  void operator()() {
    for (auto& i : s) {
      i.first->setValue(0);
    }
  }
  void ifNecessary() {
    for (auto& i : s) {
      if (i.second) {
        i.first->setValue(0);
      }
    }
  }
};

class AutoScrolling : public QObject {
  Q_OBJECT
private:
  int sliderShift;
  int delay;
  int scrollingSpeed;
  QTimer* m_pAutoScrollTimer;
  QScrollBar* scroller;
public:
  AutoScrolling(QObject* pobj, QScrollBar* s)
    : QObject(pobj)
    , scroller(s)
    , sliderShift(20)
    , scrollingSpeed(60)
    , delay(2000),
    m_pAutoScrollTimer(new QTimer(this)) {
    QObject::connect( m_pAutoScrollTimer, &QTimer::timeout, this, &AutoScrolling::slotScroller);
  }
  static bool wheelRotatedAwayFromUser(QWheelEvent* pe) {
    return pe->angleDelta().y() > 0;
  }
  void launch() {
    if (m_pAutoScrollTimer->isActive()) {
      m_pAutoScrollTimer->stop();
    }
    else {
      m_pAutoScrollTimer->start(scrollingSpeed);
    }
  }
  void stop() {
    m_pAutoScrollTimer->stop();
  }
  void launchWithDelay() {
    if (m_pAutoScrollTimer->isActive()) {
      m_pAutoScrollTimer->stop();
      QTimer::singleShot( delay, this, [&] {
        m_pAutoScrollTimer->start(scrollingSpeed);
      });
    }
  }
  void slotScroller()
  {
    if (scroller->value() != scroller->maximum()) {
      scroller->setValue( scroller->value() + sliderShift );
    } else {
      m_pAutoScrollTimer->stop();

      QTimer::singleShot( delay, this, [&] {
        emit goToNextImage();

        QTimer::singleShot( delay, this, [&] {
          m_pAutoScrollTimer->start(scrollingSpeed);
        });
      });
    }
  }
signals:
  void goToNextImage();
};

class PhotoScroller : public QScrollArea {
  Q_OBJECT
private:
  ResetScrollBar resetScrollBars;
  AutoScrolling* autoScrolling;
  PictureDownloader* downloader;
  PhotoViewer* m_ppv;
  SelectingDialog* m_psd;
  QScreen* currentScreen;
  bool vertical;

  void formatWidget();
  void createConnections();
  void slotChangeScreen( QScreen* );
protected:
  void moveEvent(QMoveEvent*) override;
  void keyPressEvent(QKeyEvent*) override;
  void keyReleaseEvent(QKeyEvent*) override;
  void mousePressEvent(QMouseEvent*) override;
  void wheelEvent(QWheelEvent*) override;
  void mouseDoubleClickEvent(QMouseEvent*) override;
public:
  PhotoScroller( QString = QString(), int = 1, QWidget* = nullptr );
signals:
  void arrowKeyPressed(QKeyEvent*);
  void arrowKeyReleased(QKeyEvent*);
  void signalScreenChanged( QScreen* );
};