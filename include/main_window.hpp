#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "arrow_keys_scroller.hpp"
#include "image_comparison_model.hpp"
#include "images_navigator.hpp"

class CachedImagesList;
class ImagesSelectorDialog;
class ImagesListPanel;

class MainWindow : public QGraphicsView {
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
  class AutoScrolling;

 public:
  MainWindow(QString = QString(), int = 1, QWidget* = nullptr);
  MainWindow(QList<QString>, QWidget* = nullptr);

 protected:
  void moveEvent(QMoveEvent*) override;
  void resizeEvent(QResizeEvent*) override;
  void keyPressEvent(QKeyEvent*) override;
  void keyReleaseEvent(QKeyEvent*) override;
  void mousePressEvent(QMouseEvent*) override;
  void wheelEvent(QWheelEvent*) override;
  void mouseDoubleClickEvent(QMouseEvent*) override;

 private:
  void Construct();
  bool imageDisplayed();
  void clearImage();
  void formatWidget();
  // Recomputes fit_zoom_ from the current pixmap and viewport, then applies
  // fit_zoom_ * zoom_factor_ as the view transform. Single source of truth for
  // all zoom state changes.
  void applyZoom();
  void fitToView();
  void zoomBy(double factor);
  void toggleFitNative();
  void toggleImagesListPanel();
  void setComparisonImages(QList<QString> list, int position);
  void rebuildActiveImages(QString const& preferred_path,
                           int fallback_position);
  void applyPanelEntries(QVector<ImageEntry> entries);
  void activatePanelImage(QString path);
  void deletePanelImage(QString path);
  void deleteCurrentImage();
  void hideCurrentImage();
  void navigateToPreviousImage();
  void navigateToNextImage();
  void moveCurrentImage(int offset);
  void updatePanelCurrentImage();
  QString currentImagePath() const;
  bool hasActiveImages() const;

  // zoom_factor_ is [kMinZoom, kMaxZoom] relative to fit-to-view.
  // fit_zoom_ is recomputed per image; it is NOT a session property.
  static constexpr double kMinZoom = 0.01;
  static constexpr double kMaxZoom = 32.0;

  std::unique_ptr<move_t> move;
  std::unique_ptr<SlidersState> sliders_state;
  std::unique_ptr<WheelScrollingState> wheel_scrolling;
  std::unique_ptr<AutoScrolling> auto_scrolling;
  std::shared_ptr<ImagePath> images_;
  std::shared_ptr<CachedImagesList> cache_;
  std::shared_ptr<FolderPath> folders_;
  ImageComparisonModel comparison_model_;
  ArrowKeysScroller* arrows_scroller_;
  ImagesSelectorDialog* m_psd;
  ImagesListPanel* images_panel_;
  QScreen* currentScreen;
  QGraphicsScene* scene_;
  QGraphicsPixmapItem* item_;
  double fit_zoom_ = 1.0;  // scale that fits the current image in the viewport
  double zoom_factor_ =
      1.0;  // user multiplier relative to fit; shared across images
  bool is_fitted_ = true;

 signals:
  void repaintImage();
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
        main_window_.navigateToNextImage();
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

// Saves and restores the scene point at the centre of the viewport as a
// fraction of the scene rect. This handles images of different sizes: a
// fractional position of (0.5, 0.5) always means "centre of the image",
// regardless of whether the next image is larger or smaller.
class MainWindow::SlidersState {
 public:
  explicit SlidersState(MainWindow* view)
      : view_(view), saved_fx_(0.5), saved_fy_(0.5), need_reset_(false) {}
  void ToggleResetting() { need_reset_ = !need_reset_; }
  void SaveScrollPosition() {
    QPointF c = view_->mapToScene(view_->viewport()->rect().center());
    QRectF sr = view_->sceneRect();
    if (sr.width() > 0 && sr.height() > 0) {
      saved_fx_ = (c.x() - sr.left()) / sr.width();
      saved_fy_ = (c.y() - sr.top()) / sr.height();
    }
  }
  void RestoreScrollPosition() {
    if (need_reset_) {
      view_->horizontalScrollBar()->setValue(0);
      view_->verticalScrollBar()->setValue(0);
    } else {
      QRectF sr = view_->sceneRect();
      view_->centerOn(sr.left() + saved_fx_ * sr.width(),
                      sr.top() + saved_fy_ * sr.height());
    }
  }

 private:
  MainWindow* view_;
  double saved_fx_;
  double saved_fy_;
  bool need_reset_;
};

class MainWindow::WheelScrollingState {
 public:
  WheelScrollingState() : horizontal_(false) {}
  void ToggleOrientation() { horizontal_ = !horizontal_; }
  bool isHorizontal() const { return horizontal_; }

 private:
  bool horizontal_;
};
