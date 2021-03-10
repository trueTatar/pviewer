#pragma once

#include <QScrollArea>
#include <QScrollBar>
#include <QWheelEvent>
#include <QTimer>
#include <map>
#include <functional>
#include "move.h"

class QScreen;
class QMoveEvent;
class QKeyEvent;
class QMouseEvent;
class Scroller;
class SelectingDialog;
class PictureDownloader;

class Navigator : public QObject {
  Q_OBJECT
private:
  QTimer timer;
  QScrollBar* horizontal_;
  QScrollBar* vertical_;
  struct Param {
    std::function<void()> func;
    bool pressed;
    Param(std::function<void()> f) : func(f), pressed(false) { }
  };
  std::map<Qt::Key, Param> k;
  static int increment(int a, int b) { return a + b; }
  static int decrement(int a, int b) { return a - b; }

  static void scroll(std::function<int(int,int)> op, int(QAbstractSlider::*f)() const, QAbstractSlider* slider)
  {
    int sliderShift = 5;
    if (slider->value() != (slider->*f)()) {
      slider->setValue( op(slider->value(), sliderShift) );
    }
  }
  void navigate() {
    for (auto i = k.begin(); i != k.end(); ++i) {
      if (i->second.pressed) {
        i->second.func();
      }
    }
  }
public:
  static bool isNoModifier(const QKeyEvent* const event) {
    return event->modifiers() == Qt::NoModifier;
  }
  static bool isArrowKeys(const QKeyEvent* const event) {
    return event->key() == Qt::Key_Down || event->key() == Qt::Key_Left ||
        event->key() == Qt::Key_Up || event->key() == Qt::Key_Right;
  }
  Navigator(QScrollBar* h, QScrollBar* v) : horizontal_(h), vertical_(v) {
    k.emplace(Qt::Key_Down,  std::bind(scroll, increment, &QAbstractSlider::maximum, vertical_));
    k.emplace(Qt::Key_Right, std::bind(scroll, increment, &QAbstractSlider::maximum, horizontal_));
    k.emplace(Qt::Key_Left,  std::bind(scroll, decrement, &QAbstractSlider::minimum, horizontal_));
    k.emplace(Qt::Key_Up,    std::bind(scroll, decrement, &QAbstractSlider::minimum, vertical_));
    connect(&timer, &QTimer::timeout, this, &Navigator::navigate);
    timer.setInterval(5);
  }
  void setKeyState(QKeyEvent* event) {
    Qt::Key key = static_cast<Qt::Key>(event->key());
    if (event->type() == QEvent::KeyPress) {
      k.find(key)->second.pressed = true;
    }
    if (event->type() == QEvent::KeyRelease) {
      k.find(key)->second.pressed = false;
    }
    timer.start();
  }
};

class Scroller : public QScrollArea {
  Q_OBJECT
 private:
  using BeginOfTheList = BeginOfTheListImpl<QString, QPixmap>;
  using EndOfTheList = EndOfTheListImpl<QString, QPixmap>;
  using ImageNumber = ImageNumberImpl<QString, QPixmap>;
  using NextImage = ImageBase<NumericalOrder, QString, QPixmap>;
  using PreviousImage = ImageBase<ReverseOrder, QString, QPixmap>;

  using move_t = Move<QString, QPixmap>;

 public:
  Scroller(QString = QString(), int = 1, QWidget* = nullptr);
  Scroller(QList<QString>, QWidget* = nullptr);
 protected:
  void moveEvent(QMoveEvent*) override;
  void keyPressEvent(QKeyEvent*) override;
  void keyReleaseEvent(QKeyEvent*) override;
  void mousePressEvent(QMouseEvent*) override;
  void wheelEvent(QWheelEvent*) override;
  void mouseDoubleClickEvent(QMouseEvent*) override;
 private:
  Navigator* n;
  struct AutoScrolling {
    class State;
    class Scroll;
    class Delay;
  };
  struct WheelScrolling {
    class State;
    class Horizontal;
    class Vertical;
  };
  struct Sliders {
    class State;
    class Preserve;
    class Reset;
  };
  class Viewer;

  void Construct();
  bool imageDisplayed();
  void formatWidget();
  std::unique_ptr<move_t> move;
  std::unique_ptr<Sliders::State> sliders_state;
  std::unique_ptr<WheelScrolling::State> wheel_scrolling;
  std::unique_ptr<AutoScrolling::State> auto_scrolling;
  PictureDownloader* downloader;
  Viewer* viewer_;
  SelectingDialog* m_psd;
  QScreen* currentScreen;

 signals:
  void repaintImage();
  void scaleImage();
  void chooseFilesToOpen();
  void displayImageNumber();
};

class Scroller::AutoScrolling::State : public QObject {
 protected:
  State(Scroller& ref) : timer_(new QTimer(&ref)) {
    timer_->callOnTimeout(this, &State::Process);
  }
  bool isMaximumValue(Scroller& ref) {
    QScrollBar* sb = ref.verticalScrollBar();
    return sb->value() == sb->maximum();
  }
  void execute(int value) { timer_->start(value); }
 private:
  virtual void ToggleState() = 0;
  virtual void Process() = 0;
  QTimer* timer_;
};

class Scroller::AutoScrolling::Scroll : public Scroller::AutoScrolling::State {
 public:
  Scroll(Scroller& ref) : State(ref), ref_(ref), step_(1), stroke_(15) {
    execute(stroke_);
  }
 private:
  void ToggleState() override;
  void Process() override {
    if (!isMaximumValue(ref_)) {
      SliderSingleStepAdd();
    } else {
      ToggleState();
    }
  }
  void SliderSingleStepAdd() {
    ref_.verticalScrollBar()->setValue(
        ref_.verticalScrollBar()->value() + step_);
  }
  Scroller& ref_;
  int step_;
  int stroke_;
};

class Scroller::AutoScrolling::Delay : public Scroller::AutoScrolling::State {
 public:
  Delay(Scroller& ref) : State(ref), ref_(ref), delay_(2000) {
    execute(delay_);
  }
 private:
  void ToggleState() override {
    ref_.auto_scrolling = std::make_unique<Scroll>(ref_);
  }
  void Process() override {
    if (isMaximumValue(ref_)) {
      return GoToNextImage();
    }
    ToggleState();
  }
  void GoToNextImage() {
    ref_.move->moveTo<NextImage>();
  }
  Scroller& ref_;
  int delay_;
};

class Scroller::Sliders::State {
 public:
  void reset() {
    ref_.verticalScrollBar()->setValue(0);
    ref_.horizontalScrollBar()->setValue(0);
  }
  virtual ~State() = default;
  virtual void ToggleResetting() = 0;
  virtual void resetIfNecessary() = 0;
 protected:
  State(Scroller& ref) : ref_(ref) { }
  template<typename T> void SwitchTo() {
    ref_.sliders_state = std::make_unique<T>(ref_);
  }
 private:
  Scroller& ref_;
};

class Scroller::Sliders::Preserve : public Scroller::Sliders::State {
 public:
  Preserve(Scroller& ref) : State(ref) { }
  void ToggleResetting() final { SwitchTo<Reset>(); }
  void resetIfNecessary() override { }
};

class Scroller::Sliders::Reset : public Scroller::Sliders::State {
 public:
  Reset(Scroller& ref) : State(ref) { }
  void ToggleResetting() final { SwitchTo<Preserve>(); }
  void resetIfNecessary() override { reset(); }
};

class Scroller::WheelScrolling::State {
 public:
  virtual void ToggleOrientation() = 0;
  virtual QWheelEvent* GetWheelEvent(QWheelEvent*) = 0;
  virtual ~State() = default;
 protected:
  State(Scroller& ref) : ref_(ref) { }
  template<typename T> void SwitchTo() {
    ref_.wheel_scrolling = std::make_unique<T>(ref_);
  }
 private:
  Scroller& ref_;
};

class Scroller::WheelScrolling::Horizontal : public Scroller::WheelScrolling::State {
 public:
  Horizontal(Scroller& ref) : State(ref) { }
  void ToggleOrientation() final { SwitchTo<Vertical>(); }
  QWheelEvent* GetWheelEvent(QWheelEvent* pe) override {
    pe->ignore();
    pe = new QWheelEvent(pe->position(), pe->globalPosition(), pe->pixelDelta(),
        QPoint(pe->angleDelta().y(), pe->angleDelta().x()),
        pe->buttons(), pe->modifiers(), pe->phase(), pe->inverted());
    return pe;
  }
};

class Scroller::WheelScrolling::Vertical : public Scroller::WheelScrolling::State {
 public:
  Vertical(Scroller& ref) : State(ref) { }
  void ToggleOrientation() final { SwitchTo<Horizontal>(); }
  QWheelEvent* GetWheelEvent(QWheelEvent* pe) override { return pe; }
};

inline void Scroller::AutoScrolling::Scroll::ToggleState() {
  ref_.auto_scrolling = std::make_unique<Delay>(ref_);
}

class Scroller::Viewer : public QLabel {
 public:
  Viewer(QWidget* parent, std::unique_ptr<Scroller::Sliders::State>& state)
      : QLabel(parent)
      , TryResetSlider([&state] { state->resetIfNecessary(); }) {
    setAlignment(Qt::AlignCenter);
    QPalette palette;
    int color = 32;
    QColor gray(color, color, color);
    palette.setColor(QPalette::Window, gray);
    setAutoFillBackground(true);
    setPalette(palette);
  }
  void UpdateImage(QPixmap const & image) {
    TryResetSlider();
    setPixmap(image);
  }
 private:
  std::function<void()> TryResetSlider;
};
