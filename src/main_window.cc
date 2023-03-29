#include "main_window.hpp"

#include "cached_images_list.hpp"
#include "images_selector_dialog.hpp"

void MainWindow::formatWidget() {
  setMinimumSize(640, 360);
  setFrameShape(NoFrame);
  setAlignment(Qt::AlignCenter);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setWidgetResizable(true);
}

void MainWindow::Construct() {
  currentScreen = screen();
  setFocusPolicy(Qt::StrongFocus);  //  accept key command

  wheel_scrolling = std::make_unique<WheelScrollingState>();
  sliders_state = std::make_unique<SlidersState>(verticalScrollBar(),
                                                 horizontalScrollBar());
  viewer_ = new Viewer(this);

  auto screen_width = [this] { return viewer_->screen()->size().width(); };
  auto update_image = [this](QPixmap const& image) {
    viewer_->setPixmap(image);
  };
  auto images = std::make_shared<ImagePath>();
  int initial_task_queue, cache_capacity;
  initial_task_queue = cache_capacity = 10;
  images->CreateTaskQueue<TaskQueue>(initial_task_queue);
  auto cache = images->CreateCacheObject<CachedImagesList>(
      cache_capacity, update_image, screen_width);

  cache->SetImageSizePasser(
      std::bind(&SlidersState::SaveScrollPosition, sliders_state.get()),
      std::bind(&SlidersState::RestoreScrollPosition, sliders_state.get(),
                std::placeholders::_1));

  auto folders = std::make_shared<FolderPath>();
  auto is_null_image = [this] {
    return viewer_->pixmap(Qt::ReturnByValue).isNull();
  };
  move = std::make_unique<move_t>(images, cache, folders, is_null_image);

  arrows_scroller_ =
      new ArrowKeysScroller(horizontalScrollBar(), verticalScrollBar());
  m_psd = new ImagesSelectorDialog(this);
  formatWidget();

  connect(folders.get(), &FolderPath::folderIsChanged, [&] {
    move->moveTo<BeginOfTheList>();
    viewer_->clear();
  });
  connect(this, &MainWindow::chooseFilesToOpen, [this] {
    viewer_->clear();
    m_psd->exec();
  });
  connect(this, &MainWindow::displayImageNumber,
          [images] { MessageBox::inform(images.get()->imageNumber(), 1000); });
  connect(this, &MainWindow::scaleImage, [this, cache] {
    if (!cache->isEmpty() && imageDisplayed()) {
      cache.get()->scale();
      cache.get()->DisplayImage();
    }
  });
  connect(this, &MainWindow::repaintImage, [this, cache] {
    if (imageDisplayed()) {
      cache.get()->UpdateCurrentScaledImage();
      cache.get()->DisplayImage();
    }
  });

  connect(m_psd, &ImagesSelectorDialog::updateFolderList,
          [folders](QList<QString> list) {
            if (!list.empty()) {
              QVector<QString> vector;
              std::move(list.begin(), list.end(), std::back_inserter(vector));
              folders->setNewList(std::move(vector));
            }
          });
  connect(m_psd, &ImagesSelectorDialog::stringListPrepared,
          [images, this](QList<QString> list, int pos) {
            if (!list.empty()) {
              QVector<QString> vector;
              std::move(list.begin(), list.end(), std::back_inserter(vector));
              images->setNewList(std::move(vector));
              move->moveTo<ImageNumber>(pos);
            }
          });

  m_psd->AssociateWith(folders);
  setWidget(viewer_);
}

MainWindow::MainWindow(QString path, int pos, QWidget* parent)
    : QScrollArea(parent) {
  Construct();
  m_psd->setDirectory(path, pos);
}

MainWindow::MainWindow(QList<QString> images, QWidget* parent)
    : QScrollArea(parent) {
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
      emit scaleImage();
      break;
    }
    case Qt::Key_Right: {
      if (pe->modifiers() & Qt::ControlModifier) {
        move->moveTo<NextImage>();
      }
      break;
    }
    case Qt::Key_Left: {
      if (pe->modifiers() & Qt::ControlModifier) {
        move->moveTo<PreviousImage>();
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
      move->moveTo<BeginOfTheList>();
      viewer_->clear();
      break;
    }
    case Qt::Key_End: {
      move->moveTo<EndOfTheList>();
      viewer_->clear();
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
    move->moveTo<PreviousImage>();
  }
  if (pe->button() & Qt::RightButton) {
    move->moveTo<NextImage>();
  }
  QWidget::mousePressEvent(pe);
}

void MainWindow::wheelEvent(QWheelEvent* pe) {
  QScrollArea::wheelEvent(wheel_scrolling->GetWheelEvent(pe));
}

void MainWindow::moveEvent(QMoveEvent*) {
  QCoreApplication::processEvents();  // this line need for screen updating;
  // otherwise QWidget::screen returns "old" screen
  if (currentScreen->geometry() != screen()->geometry()) {
    currentScreen = screen();
    emit repaintImage();
  }
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent* pe) {
  if (!imageDisplayed()) {
    emit chooseFilesToOpen();
  }
  mousePressEvent(pe);
}

bool MainWindow::imageDisplayed() {
  return !viewer_->pixmap(Qt::ReturnByValue).isNull();
}
