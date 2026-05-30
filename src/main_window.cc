#include "main_window.hpp"

#include <QApplication>
#include <QFile>
#include <algorithm>

#include "cached_images_list.hpp"
#include "images_selector_dialog.hpp"
#include "images_list_panel.hpp"

void MainWindow::formatWidget() {
  setMinimumSize(640, 360);
  setFrameStyle(QFrame::NoFrame);
  setAlignment(Qt::AlignCenter);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setInteractive(false);
  setTransformationAnchor(AnchorViewCenter);
  setResizeAnchor(AnchorViewCenter);
}

void MainWindow::applyZoom() {
  if (item_->pixmap().isNull()) return;
  fit_zoom_ = viewport()->width() / qreal(item_->pixmap().width());
  const double s = fit_zoom_ * zoom_factor_;
  setTransform(QTransform::fromScale(s, s));
}

void MainWindow::fitToWidth() {
  zoom_factor_ = 1.0;
  is_fitted_ = true;
  applyZoom();
}

void MainWindow::zoomBy(double factor) {
  if (!imageDisplayed()) return;
  zoom_factor_ = std::clamp(zoom_factor_ * factor, kMinZoom, kMaxZoom);
  is_fitted_ = false;
  applyZoom();
}

void MainWindow::toggleFitNative() {
  if (!imageDisplayed()) return;
  if (!is_fitted_) {
    fitToWidth();
  } else {
    // Native: 1 source pixel = 1 screen pixel for the current image.
    // zoom_factor_ = 1/fit_zoom_ makes fit_zoom_ * zoom_factor_ = 1.0.
    zoom_factor_ = 1.0 / fit_zoom_;
    is_fitted_ = false;
    applyZoom();
  }
}

void MainWindow::toggleImagesListPanel() {
  images_panel_->isVisible() ? images_panel_->hide() : images_panel_->show();
}

QString MainWindow::currentImagePath() const {
  if (!hasActiveImages()) return QString();
  return images_->pathByIndex();
}

bool MainWindow::hasActiveImages() const {
  return images_ && !images_->isEmpty();
}

void MainWindow::setComparisonImages(QList<QString> list, int position) {
  QVector<QString> vector;
  vector.reserve(list.size());
  std::move(list.begin(), list.end(), std::back_inserter(vector));

  comparison_model_.SetImages(std::move(vector));
  images_panel_->SetEntries(comparison_model_.Entries());
  rebuildActiveImages(QString(), position);
}

void MainWindow::applyPanelEntries(QVector<ImageEntry> entries) {
  const QString preferred_path = currentImagePath();
  comparison_model_.SetEntries(std::move(entries));
  rebuildActiveImages(preferred_path, 1);
}

void MainWindow::activatePanelImage(QString path) {
  rebuildActiveImages(path, 1);
}

void MainWindow::deletePanelImage(QString path) {
  const QString preferred_path = currentImagePath();
  const int fallback_position = comparison_model_.EnabledPositionFor(path);

  if (!QFile::moveToTrash(path)) {
    MessageBox::inform(QString("Failed to delete: ") + path, 1500);
    return;
  }

  comparison_model_.RemovePath(path);
  images_panel_->SetEntries(comparison_model_.Entries());
  rebuildActiveImages(preferred_path == path ? QString() : preferred_path,
                      fallback_position);
}

void MainWindow::navigateToPreviousImage() {
  if (!hasActiveImages()) return;
  move->moveTo<PreviousImage>();
  updatePanelCurrentImage();
}

void MainWindow::navigateToNextImage() {
  if (!hasActiveImages()) return;
  move->moveTo<NextImage>();
  updatePanelCurrentImage();
}

void MainWindow::moveCurrentImage(int offset) {
  const QString path = currentImagePath();
  if (path.isEmpty() || !comparison_model_.MovePath(path, offset)) return;

  images_panel_->SetEntries(comparison_model_.Entries());
  rebuildActiveImages(path, 1);
}

void MainWindow::updatePanelCurrentImage() {
  images_panel_->SetCurrentPath(currentImagePath());
}

void MainWindow::rebuildActiveImages(QString const& preferred_path,
                                     int fallback_position) {
  QVector<QString> active_paths = comparison_model_.EnabledPaths();
  if (active_paths.empty()) {
    images_->setNewList(std::move(active_paths));
    clearImage();
    updatePanelCurrentImage();
    return;
  }

  int position = comparison_model_.EnabledPositionFor(preferred_path);
  if (position == 0) {
    position = std::clamp(fallback_position, 1, active_paths.size());
  }

  images_->setNewList(std::move(active_paths));
  move->moveTo<ImageNumber>(position);
  cache_->DisplayImage();
  updatePanelCurrentImage();
}

void MainWindow::clearImage() {
  item_->setPixmap(QPixmap());
  setSceneRect(QRectF());
}

void MainWindow::Construct() {
  currentScreen = screen();
  setFocusPolicy(Qt::StrongFocus);

  scene_ = new QGraphicsScene(this);
  item_ = new QGraphicsPixmapItem();
  item_->setTransformationMode(Qt::SmoothTransformation);
  scene_->addItem(item_);
  setScene(scene_);

  // Dark background matching the original Viewer palette.
  setBackgroundBrush(QColor(32, 32, 32));

  wheel_scrolling = std::make_unique<WheelScrollingState>();
  sliders_state = std::make_unique<SlidersState>(this);

  auto update_image = [this](QPixmap const& image) {
    item_->setPixmap(image);
    setSceneRect(item_->boundingRect());
    if (!image.isNull()) applyZoom();
  };
  int initial_task_queue, cache_capacity;
  initial_task_queue = cache_capacity = 10;
  images_ = std::make_shared<ImagePath>();
  images_->CreateTaskQueue<TaskQueue>(initial_task_queue);
  cache_ =
      images_->CreateCacheObject<CachedImagesList>(cache_capacity, update_image);

  cache_->SetScrollCallbacks(
      std::bind(&SlidersState::SaveScrollPosition, sliders_state.get()),
      std::bind(&SlidersState::RestoreScrollPosition, sliders_state.get()),
      [this] { return imageDisplayed(); });

  folders_ = std::make_shared<FolderPath>();
  auto is_null_image = [this] { return item_->pixmap().isNull(); };
  move = std::make_unique<move_t>(images_, cache_, folders_, is_null_image);

  arrows_scroller_ =
      new ArrowKeysScroller(horizontalScrollBar(), verticalScrollBar());
  m_psd = new ImagesSelectorDialog(this);
  images_panel_ = new ImagesListPanel(this);
  formatWidget();

  connect(images_panel_, &ImagesListPanel::entriesChanged, this,
          &MainWindow::applyPanelEntries);
  connect(images_panel_, &ImagesListPanel::imageActivated, this,
          &MainWindow::activatePanelImage);
  connect(images_panel_, &ImagesListPanel::imageDeleteRequested, this,
          &MainWindow::deletePanelImage);
  connect(images_panel_, &ImagesListPanel::previousImageRequested, this,
          &MainWindow::navigateToPreviousImage);
  connect(images_panel_, &ImagesListPanel::nextImageRequested, this,
          &MainWindow::navigateToNextImage);
  connect(images_panel_, &ImagesListPanel::zoomInRequested, this,
          [this] { zoomBy(1.25); });
  connect(images_panel_, &ImagesListPanel::zoomOutRequested, this,
          [this] { zoomBy(0.8); });
  connect(folders_.get(), &FolderPath::folderIsChanged, [this] {
    if (hasActiveImages()) move->moveTo<BeginOfTheList>();
    clearImage();
  });
  connect(this, &MainWindow::chooseFilesToOpen, [this] {
    clearImage();
    m_psd->exec();
  });
  connect(this, &MainWindow::displayImageNumber,
          [this] {
            MessageBox::inform(
                hasActiveImages() ? images_->imageNumber() : QString("0 / 0"),
                1000);
          });
  connect(this, &MainWindow::repaintImage, [this] {
    if (imageDisplayed()) {
      cache_.get()->DisplayImage();
      applyZoom();
      updatePanelCurrentImage();
    }
  });

  connect(m_psd, &ImagesSelectorDialog::updateFolderList,
          [this](QList<QString> list) {
            if (!list.empty()) {
              QVector<QString> vector;
              std::move(list.begin(), list.end(), std::back_inserter(vector));
              folders_->setNewList(std::move(vector));
            }
          });
  connect(m_psd, &ImagesSelectorDialog::stringListPrepared,
          [this](QList<QString> list, int pos) {
            if (!list.empty()) {
              setComparisonImages(std::move(list), pos);
            }
          });

  m_psd->AssociateWith(folders_);
}

MainWindow::MainWindow(QString path, int pos, QWidget* parent)
    : QGraphicsView(parent) {
  Construct();
  m_psd->setDirectory(path, pos);
}

MainWindow::MainWindow(QList<QString> images, QWidget* parent)
    : QGraphicsView(parent) {
  Construct();
  m_psd->setImages(images);
}

void MainWindow::keyReleaseEvent(QKeyEvent* pe) {
  if (pe->isAutoRepeat()) return QWidget::keyReleaseEvent(pe);
  if (ArrowKeysScroller::isArrowKeys(pe)) {
    arrows_scroller_->setKeyState(pe);
  }
  QWidget::keyReleaseEvent(pe);
}

void MainWindow::keyPressEvent(QKeyEvent* pe) {
  if (ArrowKeysScroller::isArrowKeys(pe) &&
      ArrowKeysScroller::isNoModifier(pe)) {
    arrows_scroller_->setKeyState(pe);
  }
  switch (pe->key()) {
    case Qt::Key_Q: {
      auto_scrolling = (auto_scrolling == nullptr)
                           ? std::make_unique<AutoScrolling>(*this)
                           : nullptr;
      break;
    }
    case Qt::Key_BracketRight: {
      move->moveTo<NextFolder>();
      break;
    }
    case Qt::Key_BracketLeft: {
      move->moveTo<PreviousFolder>();
      break;
    }
    case Qt::Key_S: {
      toggleFitNative();
      break;
    }
    case Qt::Key_Plus:
    case Qt::Key_Equal: {
      zoomBy(1.25);
      break;
    }
    case Qt::Key_Minus: {
      zoomBy(0.8);
      break;
    }
    case Qt::Key_0: {
      fitToWidth();
      break;
    }
    case Qt::Key_L: {
      if (pe->modifiers() == Qt::NoModifier) {
        toggleImagesListPanel();
      }
      break;
    }
    case Qt::Key_Right: {
      if (pe->modifiers() & Qt::ControlModifier) {
        navigateToNextImage();
      }
      break;
    }
    case Qt::Key_Left: {
      if (pe->modifiers() & Qt::ControlModifier) {
        navigateToPreviousImage();
      }
      break;
    }
    case Qt::Key_Up: {
      if (pe->modifiers() == Qt::AltModifier) {
        moveCurrentImage(-1);
      }
      break;
    }
    case Qt::Key_Down: {
      if (pe->modifiers() == Qt::AltModifier) {
        moveCurrentImage(1);
      }
      break;
    }
    case Qt::Key_O: {
      emit chooseFilesToOpen();
      break;
    }
    case Qt::Key_P: {
      sliders_state->ToggleResetting();
      break;
    }
    case Qt::Key_N: {
      emit displayImageNumber();
      break;
    }
    case Qt::Key_Home: {
      if (hasActiveImages()) {
        move->moveTo<BeginOfTheList>();
        clearImage();
      }
      break;
    }
    case Qt::Key_End: {
      if (hasActiveImages()) {
        move->moveTo<EndOfTheList>();
        clearImage();
      }
      break;
    }
    case Qt::Key_Escape: {
      QApplication::quit();
      break;
    }
    case Qt::Key_Space: {
      setWindowState(Qt::WindowMinimized);
      break;
    }
    case Qt::Key_F11: {
      if (windowState() & Qt::WindowFullScreen) {
        setWindowState(Qt::WindowMaximized);
      } else if (windowState() == Qt::WindowNoState) {
        setWindowState(Qt::WindowFullScreen);
      } else if (windowState() == Qt::WindowMaximized) {
        setWindowState(Qt::WindowMaximized | Qt::WindowFullScreen);
      }
      break;
    }
    default: {
    }
  }
  QWidget::keyPressEvent(pe);
}

void MainWindow::mousePressEvent(QMouseEvent* pe) {
  if (pe->buttons() & Qt::LeftButton && pe->buttons() & Qt::RightButton) {
    setWindowState(Qt::WindowMinimized);
  }
  if (pe->button() & Qt::MiddleButton) {
    wheel_scrolling->ToggleOrientation();
  }
  if (pe->button() & Qt::LeftButton) {
    navigateToPreviousImage();
  }
  if (pe->button() & Qt::RightButton) {
    navigateToNextImage();
  }
  QWidget::mousePressEvent(pe);
}

void MainWindow::wheelEvent(QWheelEvent* pe) {
  if (!wheel_scrolling->isHorizontal()) {
    QGraphicsView::wheelEvent(pe);
    return;
  }
  QWheelEvent swapped(pe->position(), pe->globalPosition(), pe->pixelDelta(),
                      QPoint(pe->angleDelta().y(), pe->angleDelta().x()),
                      pe->buttons(), pe->modifiers(), pe->phase(),
                      pe->inverted());
  QGraphicsView::wheelEvent(&swapped);
  pe->setAccepted(swapped.isAccepted());
}

void MainWindow::moveEvent(QMoveEvent*) {
  QCoreApplication::processEvents();
  if (currentScreen->geometry() != screen()->geometry()) {
    currentScreen = screen();
    emit repaintImage();
  }
}

void MainWindow::resizeEvent(QResizeEvent* e) {
  QGraphicsView::resizeEvent(e);
  if (imageDisplayed()) applyZoom();
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent* pe) {
  if (!imageDisplayed()) {
    emit chooseFilesToOpen();
  }
  mousePressEvent(pe);
}

bool MainWindow::imageDisplayed() {
  return !item_->pixmap().isNull();
}
