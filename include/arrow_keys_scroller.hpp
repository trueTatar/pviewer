#pragma once

#include <QKeyEvent>
#include <QObject>
#include <QScrollBar>
#include <QTimer>

class ArrowKeysScroller : public QObject {
  Q_OBJECT
 private:
  struct Param {
    std::function<void()> func;
    bool pressed;
    Param(std::function<void()> f) : func(f), pressed(false) {}
  };

 public:
  static bool isNoModifier(const QKeyEvent* const event);
  static bool isArrowKeys(const QKeyEvent* const event);

  ArrowKeysScroller(QScrollBar* h, QScrollBar* v);
  void setKeyState(QKeyEvent* event);

 private:
  static int increment(int a, int b);
  static int decrement(int a, int b);

  static void scroll(std::function<int(int, int)> op,
                     int (QAbstractSlider::*f)() const,
                     QAbstractSlider* slider);
  void Scroll();

  std::map<Qt::Key, Param> k;
  QTimer timer;
  QScrollBar* horizontal_;
  QScrollBar* vertical_;
};