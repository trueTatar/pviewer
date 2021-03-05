#include "PhotoScroller.hpp"

#include <QApplication>
#include <QTimer>
#include <QScreen>
#include <QMessageBox>
#include <QScrollBar>
#include <QKeyEvent>
#include "PhotoViewer.hpp"
#include <QDebug>


void PhotoScroller::formatWidget()
{
  setMinimumSize(640, 360);
  setFrameShape(NoFrame);
  setAlignment( Qt::AlignCenter );
  setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  setWidgetResizable( true );
}

void PhotoScroller::createConnections()
{
  connect( this, &PhotoScroller::signalScreenChanged, &PhotoScroller::slotChangeScreen );
  connect(m_ppv, &PhotoViewer::pixmapIsChanging, &resetScrollBars, &ResetScrollBar::ifNecessary);
  connect(downloader, &PictureDownloader::imageIsDownloaded, m_ppv, &PhotoViewer::appendToImagePaths);
  connect( m_psd, SIGNAL(downloadChosen()), downloader, SLOT(showDialog()) );
  connect( m_psd, &SelectingDialog::stringListPrepared, m_ppv, &PhotoViewer::setImagePaths);
  connect( m_psd, &SelectingDialog::folderIsChanged, [&] {
    m_ppv->reset(NextImage);
  });
  connect(autoScrolling, &AutoScrolling::goToNextImage, [&] {
    m_ppv->goTo(NextImage);
  });
}

PhotoScroller::PhotoScroller(QString path, int num, QWidget* pwgt) 
  : QScrollArea( pwgt )
  , resetScrollBars(this, horizontalScrollBar(), verticalScrollBar())
{
  currentScreen = screen();
  setFocusPolicy( Qt::StrongFocus );        //  accept key command

  autoScrolling = new AutoScrolling(this, verticalScrollBar());
  m_psd = new SelectingDialog(this);
  m_ppv = new PhotoViewer(this);
  downloader = new PictureDownloader(this);
  createConnections();
  
  formatWidget();

  m_ppv->prepareWidget(num);
  m_psd->setDirectory(path);
  setWidget( m_ppv );
}

void PhotoScroller::keyReleaseEvent(QKeyEvent* pe)
{
  QWidget::keyReleaseEvent(pe);
}

void PhotoScroller::keyPressEvent(QKeyEvent* pe)
{
  switch (pe->key())
  {
    case Qt::Key_Q: {
      autoScrolling->launch();
    } break;

    case Qt::Key_BracketRight: {
      m_psd->goTo(std::make_unique<NextFolder>());
    } break;

    case Qt::Key_BracketLeft: {
      m_psd->goTo(std::make_unique<PreviousFolder>());
    } break;

    case Qt::Key_R: {
      autoScrolling->stop();
      resetScrollBars();
    } break;

    case Qt::Key_D: {
      downloader->showDialog();
    } break;

    case Qt::Key_S: {
      m_ppv->scale();
    } break;

    case Qt::Key_Right: {
      if (pe->modifiers() & Qt::ControlModifier) {
        m_ppv->goTo(NextImage);
      }
    } break;

    case Qt::Key_Left: {
      if (pe->modifiers() & Qt::ControlModifier) {
        m_ppv->goTo(PreviousImage);
      }
    } break;

    case Qt::Key_O: {
      m_ppv->setFiles();
      m_psd->exec();
    } break;

    case Qt::Key_P: {
      !resetScrollBars;
    } break;

    case Qt::Key_N: {
      QMessageBox::information( this, QString(), m_ppv->imageNumber() );
    } break;

    case Qt::Key_Home: {
      m_ppv->reset(NextImage);
    } break;

    case Qt::Key_End: {
      m_ppv->reset(PreviousImage);
    } break;

    case Qt::Key_Escape: {
      QApplication::quit();
    } break;

    case Qt::Key_Space: {
      setWindowState( Qt::WindowMinimized );
    } break;

    case Qt::Key_F11: {
      if (windowState() & Qt::WindowFullScreen) {
        setWindowState(Qt::WindowMaximized);
      }
      else if (windowState() == Qt::WindowNoState) {
        setWindowState(Qt::WindowFullScreen);
      }
      else if (windowState() == Qt::WindowMaximized) {
        setWindowState(Qt::WindowMaximized | Qt::WindowFullScreen);
      }
    } break;      //  breaks case Qt::Key_F11

    default: { }
  }
  QWidget::keyPressEvent( pe );
}

void PhotoScroller::mousePressEvent(QMouseEvent* pe)
{
  if (pe->buttons() & Qt::LeftButton && pe->buttons() & Qt::RightButton) {
    setWindowState( Qt::WindowMinimized );
  }

  if (pe->button() & Qt::MiddleButton) {
    vertical = !vertical;
  }

  if (pe->button() & Qt::LeftButton) {
    m_ppv->goTo(PreviousImage);
  }

  if (pe->button() & Qt::RightButton) {
    m_ppv->goTo(NextImage);
    autoScrolling->launchWithDelay();
  }
  QWidget::mousePressEvent(pe);
}

void PhotoScroller::wheelEvent(QWheelEvent* pe)
{
  if (AutoScrolling::wheelRotatedAwayFromUser(pe)) {
    autoScrolling->stop();
  }

  QWheelEvent* hor;
  if (vertical) {
    QPoint ad(pe->angleDelta().y(), pe->angleDelta().x());
    hor = new QWheelEvent(pe->position(), pe->globalPosition(), pe->pixelDelta(),
               ad, pe->buttons(), pe->modifiers(), pe->phase(), pe->inverted());
    pe->ignore();
  } else {
    hor = pe;
  }
  
  QScrollArea::wheelEvent(hor);
}

void PhotoScroller::moveEvent(QMoveEvent*)
{
  if (currentScreen->geometry() != screen()->geometry()) {
    currentScreen = screen();
    emit signalScreenChanged( currentScreen );
  }
}

void PhotoScroller::slotChangeScreen(QScreen*)
{
  m_ppv->repaintPixmap();
}

void PhotoScroller::mouseDoubleClickEvent(QMouseEvent* pe)
{
  if (m_ppv->pixmapIsNull()) {
    m_ppv->setFiles();
    m_psd->exec();
  }
  mousePressEvent( pe );
}