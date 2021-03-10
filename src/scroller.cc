#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QKeyEvent>

#include "scroller.h"
#include "messagebox.h"
#include "SelectingDialog.hpp"
#include "PictureDownloader.hpp"
#include "cached_images_list.hpp"

void Scroller::formatWidget() {
  setMinimumSize(640, 360);
  setFrameShape(NoFrame);
  setAlignment(Qt::AlignCenter);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setWidgetResizable(true);
}


void Scroller::Construct() {
  currentScreen = screen();
  setFocusPolicy(Qt::StrongFocus);        //  accept key command

  sliders_state = std::make_unique<Sliders::Reset>(*this);
  wheel_scrolling = std::make_unique<WheelScrolling::Vertical>(*this);
  viewer_ = new Viewer(this, sliders_state);

  auto screen_width = [this] { return viewer_->screen()->size().width(); };
  auto update_image = [this] (QPixmap const & image) { viewer_->UpdateImage(image); };
  auto images = std::make_shared<ImagePath>();
  auto cache = images->CreateCacheObject<CachedImagesList>(10, update_image, screen_width);

  auto folders = std::make_shared<FolderPath>();
  auto is_null_image = [this] { return viewer_->pixmap(Qt::ReturnByValue).isNull(); };
  move = std::make_unique<move_t>(images, cache, folders, is_null_image);

  n = new Navigator(horizontalScrollBar(), verticalScrollBar());
  m_psd = new SelectingDialog(this);
  downloader = new PictureDownloader(this);
  formatWidget();

  connect(folders.get(), &FolderPath::folderIsChanged, [&] {
    move->moveTo<BeginOfTheList>();
    viewer_->clear();
  });
  connect(this, &Scroller::chooseFilesToOpen, [this, images] {
    move->moveTo<BeginOfTheList>();
    images.get()->clear();
    m_psd->exec();
  });
  connect(this, &Scroller::displayImageNumber, [images] {
    MessageBox::inform(images.get()->imageNumber(), 1000);
  });
  connect(this, &Scroller::scaleImage, [this, cache] {
    if (!cache->isEmpty() && imageDisplayed()) {
      cache.get()->scale();
      cache.get()->DisplayImage();
    }
  });
  connect(this, &Scroller::repaintImage, [this, cache] {
    if (imageDisplayed()) {
      cache.get()->UpdateCurrentScaledImage();
      cache.get()->DisplayImage();
    }
  });

  connect(m_psd, &SelectingDialog::updateFolderList, [folders] (QList<QString> list) {
    QVector<QString> vector;
    std::move(list.begin(), list.end(), std::back_inserter(vector));
    folders->setNewList(std::move(vector));
  });
  connect(m_psd, &SelectingDialog::stringListPrepared, [images] (QList<QString> list) {
    QVector<QString> vector;
    std::move(list.begin(), list.end(), std::back_inserter(vector));
    images->setNewList(std::move(vector));
  });
  connect(downloader, &PictureDownloader::imageIsDownloaded,
      images.get(), &ImagePath::appendItem);
  connect(m_psd, &SelectingDialog::downloadChosen,
      downloader, &PictureDownloader::showDialog);

  m_psd->AssociateWith(folders);
  setWidget(viewer_);
}

Scroller::Scroller(QString path, int num, QWidget* pwgt) : QScrollArea(pwgt) {
  Construct();
  m_psd->setDirectory(path);
  move->moveTo<ImageNumber>(num);
}

Scroller::Scroller(QList<QString> images, QWidget* parent) : QScrollArea(parent) {
  Construct();
  m_psd->setImages(images);
}

void Scroller::keyReleaseEvent(QKeyEvent* pe) {
  if (pe->isAutoRepeat()) return QWidget::keyReleaseEvent(pe);
  if (Navigator::isArrowKeys(pe)) {
    n->setKeyState(pe);
  }
  QWidget::keyReleaseEvent(pe);
}

void Scroller::keyPressEvent(QKeyEvent* pe) {
  if (Navigator::isArrowKeys(pe) && Navigator::isNoModifier(pe)) {
    n->setKeyState(pe);
  }
  switch (pe->key()) {
    case Qt::Key_Q: {
      auto_scrolling = (auto_scrolling == nullptr) ?
          std::make_unique<AutoScrolling::Delay>(*this) : nullptr;
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
    case Qt::Key_R: {
      auto_scrolling = nullptr;
      sliders_state->reset();
      break;
    }
    case Qt::Key_D: {
      downloader->showDialog();
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
      setWindowState( Qt::WindowMinimized );
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
    default: { }
  }
  QWidget::keyPressEvent(pe);
}

void Scroller::mousePressEvent(QMouseEvent* pe) {
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

void Scroller::wheelEvent(QWheelEvent* pe) {
  QScrollArea::wheelEvent(wheel_scrolling->GetWheelEvent(pe));
}

void Scroller::moveEvent(QMoveEvent*) {
  QCoreApplication::processEvents();  // this line need for screen updating;
                                      // otherwise QWidget::screen returns "old" screen
  if (currentScreen->geometry() != screen()->geometry()) {
    currentScreen = screen();
    emit repaintImage();
  }
}

void Scroller::mouseDoubleClickEvent(QMouseEvent* pe) {
  if (!imageDisplayed()) {
    emit chooseFilesToOpen();
  }
  mousePressEvent(pe);
}

bool Scroller::imageDisplayed() {
  return !viewer_->pixmap(Qt::ReturnByValue).isNull();
}
