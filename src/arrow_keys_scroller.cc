#include "arrow_keys_scroller.hpp"

ArrowKeysScroller::ArrowKeysScroller(QScrollBar* h, QScrollBar* v)
    : horizontal_(h), vertical_(v) {
  k.emplace(Qt::Key_Down,
            std::bind(scroll, increment, &QAbstractSlider::maximum, vertical_));
  k.emplace(Qt::Key_Right, std::bind(scroll, increment,
                                     &QAbstractSlider::maximum, horizontal_));
  k.emplace(Qt::Key_Left, std::bind(scroll, decrement,
                                    &QAbstractSlider::minimum, horizontal_));
  k.emplace(Qt::Key_Up,
            std::bind(scroll, decrement, &QAbstractSlider::minimum, vertical_));
  connect(&timer, &QTimer::timeout, this, &ArrowKeysScroller::Scroll);
  timer.setInterval(5);
}

int ArrowKeysScroller::increment(int a, int b) { return a + b; }
int ArrowKeysScroller::decrement(int a, int b) { return a - b; }

void ArrowKeysScroller::scroll(std::function<int(int, int)> op,
                       int (QAbstractSlider::*f)() const,
                       QAbstractSlider* slider) {
  int sliderShift = 5;
  if (slider->value() != (slider->*f)()) {
    slider->setValue(op(slider->value(), sliderShift));
  }
}

void ArrowKeysScroller::Scroll() {
  for (auto i = k.begin(); i != k.end(); ++i) {
    if (i->second.pressed) {
      i->second.func();
    }
  }
}

bool ArrowKeysScroller::isNoModifier(const QKeyEvent* const event) {
  return event->modifiers() == Qt::NoModifier;
}

bool ArrowKeysScroller::isArrowKeys(const QKeyEvent* const event) {
  return event->key() == Qt::Key_Down || event->key() == Qt::Key_Left ||
         event->key() == Qt::Key_Up || event->key() == Qt::Key_Right;
}

void ArrowKeysScroller::setKeyState(QKeyEvent* event) {
  Qt::Key key = static_cast<Qt::Key>(event->key());
  if (event->type() == QEvent::KeyPress) {
    k.find(key)->second.pressed = true;
  }
  if (event->type() == QEvent::KeyRelease) {
    k.find(key)->second.pressed = false;
  }
  timer.start();
}