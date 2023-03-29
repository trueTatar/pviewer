#pragma once

#include <QScrollArea>

#include "arrow_keys_scroller.hpp"
#include "images_navigator.hpp"

class ImagesSelectorDialog;

class MainWindow : public QScrollArea {
  Q_OBJECT
 private:
  using BeginOfTheList = BeginOfTheListImpl<QString, QPixmap>;
  using EndOfTheList = EndOfTheListImpl<QString, QPixmap>;
  using ImageNumber = ImageNumberImpl<QString, QPixmap>;
  using NextImage = ImageBase<NumericalOrder, QString, QPixmap>;
  using PreviousImage = ImageBase<ReverseOrder, QString, QPixmap>;
  using move_t = ImagesNavigator<QString, QPixmap>;

  class SlidersState;
  class WheelScrollingState;
  class Viewer;
  class AutoScrolling;

 public:
  MainWindow(QString = QString(), int = 1, QWidget* = nullptr);
  MainWindow(QList<QString>, QWidget* = nullptr);

 protected:
  void moveEvent(QMoveEvent*) override;
  void keyPressEvent(QKeyEvent*) override;
  void keyReleaseEvent(QKeyEvent*) override;
  void mousePressEvent(QMouseEvent*) override;
  void wheelEvent(QWheelEvent*) override;
  void mouseDoubleClickEvent(QMouseEvent*) override;

 private:
  void Construct();
  bool imageDisplayed();
  void formatWidget();
  std::unique_ptr<move_t> move;
  std::unique_ptr<SlidersState> sliders_state;
  std::unique_ptr<WheelScrollingState> wheel_scrolling;
  std::unique_ptr<AutoScrolling> auto_scrolling;
  ArrowKeysScroller* arrows_scroller_;
  Viewer* viewer_;
  ImagesSelectorDialog* m_psd;
  QScreen* currentScreen;

 signals:
  void repaintImage();
  void scaleImage();
  void chooseFilesToOpen();
  void displayImageNumber();
};

class MainWindow::AutoScrolling : public QObject {
 public:
  AutoScrolling(MainWindow& main_window)
      : scrolling_(true),
        pixels_to_scroll_away_(1),
        scroll_tick_(15),
        delay_before_scrolling_(2000),
        main_window_(main_window),
        timer_(new QTimer(&main_window_)) {
    timer_->callOnTimeout(this, &AutoScrolling::Process);
    timer_->start(delay_before_scrolling_);
    ToggleState(scroll_tick_);
  }

 private:
  void Process() {
    if (scrolling_) {
      if (!isScrolledToBottom()) {
        int value =
            main_window_.verticalScrollBar()->value() + pixels_to_scroll_away_;
        main_window_.verticalScrollBar()->setValue(value);
        return;
      }
      ToggleState(delay_before_scrolling_);
    } else {
      if (isScrolledToBottom()) {
        main_window_.move->moveTo<NextImage>();
        return;
      }
      ToggleState(scroll_tick_);
    }
  }
  bool isScrolledToBottom() {
    QScrollBar* sb = main_window_.verticalScrollBar();
    return sb->value() == sb->maximum();
  }
  void ToggleState(int timeout) {
    scrolling_ = !scrolling_;
    timer_->start(timeout);
  }

  bool scrolling_;
  const int pixels_to_scroll_away_;
  const int scroll_tick_;
  const int delay_before_scrolling_;
  MainWindow& main_window_;
  QTimer* timer_;
};

class MainWindow::SlidersState {
 public:
  SlidersState(QScrollBar* vertical, QScrollBar* horizontal)
      : vertical_(vertical),
        horizontal_(horizontal),
        last_vertical_(0),
        last_horizontal_(0),
        need_reset_(true) {}
  void ToggleResetting() { need_reset_ = !need_reset_; }
  void SaveScrollPosition() {
    if (vertical_->value() || horizontal_->value()) {
      last_vertical_ = vertical_->value() / double(image_size_.height());
      last_horizontal_ = horizontal_->value() / double(image_size_.width());
    }
  }
  void RestoreScrollPosition(QSize image) {
    int v = 0, h = 0;
    if (!need_reset_) {
      v = last_vertical_ * image.height();
      h = last_horizontal_ * image.width();
    }
    vertical_->setValue(v);
    horizontal_->setValue(h);
    image_size_ = image;
  }

 private:
  QScrollBar* vertical_;
  QScrollBar* horizontal_;
  double last_vertical_;
  double last_horizontal_;
  bool need_reset_;
  QSize image_size_;
};

class MainWindow::WheelScrollingState {
 public:
  WheelScrollingState() : horizontal_(false) {}
  void ToggleOrientation() { horizontal_ = !horizontal_; }
  QWheelEvent* GetWheelEvent(QWheelEvent* event) {
    if (horizontal_) {
      event->ignore();
      event = new QWheelEvent(
          event->position(), event->globalPosition(), event->pixelDelta(),
          QPoint(event->angleDelta().y(), event->angleDelta().x()),
          event->buttons(), event->modifiers(), event->phase(),
          event->inverted());
    }
    return event;
  }

 private:
  bool horizontal_;
};

class MainWindow::Viewer : public QLabel {
 public:
  Viewer(QWidget* parent) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    QPalette palette;
    int color = 32;
    QColor gray(color, color, color);
    palette.setColor(QPalette::Window, gray);
    setAutoFillBackground(true);
    setPalette(palette);
  }
};