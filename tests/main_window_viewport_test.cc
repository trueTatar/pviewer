#include <gtest/gtest.h>

#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QImage>
#include <QKeyEvent>
#include <QMimeData>
#include <QScrollBar>
#include <QTemporaryDir>

#include "main_window.hpp"

namespace {

QString MakeImage(QTemporaryDir& dir, QString name, QSize size, QColor color) {
  QImage image(size, QImage::Format_RGB32);
  image.fill(color);

  const QString path = dir.filePath(name);
  EXPECT_TRUE(image.save(path, "PNG"));
  return path;
}

void SendCtrlArrow(MainWindow& window, Qt::Key key) {
  QKeyEvent press(QEvent::KeyPress, key, Qt::ControlModifier);
  QApplication::sendEvent(&window, &press);
  QKeyEvent release(QEvent::KeyRelease, key, Qt::ControlModifier);
  QApplication::sendEvent(&window, &release);
  QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

double ViewportCenterYFraction(MainWindow& window) {
  const QRectF scene_rect = window.sceneRect();
  EXPECT_GT(scene_rect.height(), 0.0);

  const QPointF center = window.mapToScene(window.viewport()->rect().center());
  return (center.y() - scene_rect.top()) / scene_rect.height();
}

void SendKey(MainWindow& window, Qt::Key key,
             Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
  QKeyEvent press(QEvent::KeyPress, key, modifiers);
  QApplication::sendEvent(&window, &press);
  QKeyEvent release(QEvent::KeyRelease, key, modifiers);
  QApplication::sendEvent(&window, &release);
  QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

}  // namespace

TEST(MainWindowViewportTest, OpensTallImageFitToViewportHeight) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const QString tall =
      MakeImage(dir, "tall.png", QSize(500, 1400), QColor(0, 0, 180));

  MainWindow window(QList<QString>{tall});
  window.resize(800, 600);
  window.show();
  QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

  EXPECT_EQ(window.sceneRect().size(), QSizeF(500, 1400));
  EXPECT_DOUBLE_EQ(window.transform().m11(),
                   window.viewport()->height() / 1400.0);
  EXPECT_EQ(window.horizontalScrollBar()->maximum(), 0);
  EXPECT_EQ(window.verticalScrollBar()->maximum(), 0);
}

TEST(MainWindowViewportTest, HidesCurrentImageAndDisplaysNextActiveImage) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const QString first =
      MakeImage(dir, "first.png", QSize(320, 240), QColor(180, 0, 0));
  const QString second =
      MakeImage(dir, "second.png", QSize(640, 360), QColor(0, 180, 0));

  MainWindow window(QList<QString>{first, second});
  window.resize(800, 600);
  window.show();
  QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  ASSERT_EQ(window.sceneRect().size(), QSizeF(320, 240));

  SendKey(window, Qt::Key_H);

  EXPECT_EQ(window.sceneRect().size(), QSizeF(640, 360));
}

TEST(MainWindowViewportTest, CopiesCurrentImageFileToClipboard) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const QString image =
      MakeImage(dir, "image.png", QSize(320, 240), QColor(180, 0, 0));

  MainWindow window(QList<QString>{image});
  window.resize(800, 600);
  window.show();
  QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

  QApplication::clipboard()->clear();
  SendKey(window, Qt::Key_C, Qt::ControlModifier);

  const QMimeData* mime_data = QApplication::clipboard()->mimeData();
  ASSERT_TRUE(mime_data->hasUrls());
  ASSERT_EQ(mime_data->urls().size(), 1);
  EXPECT_EQ(mime_data->urls().first().toLocalFile(), image);
  EXPECT_EQ(mime_data->text(), image);
}

TEST(MainWindowViewportTest,
     KeepsLowerResolutionViewportAfterZoomedAtRightEdgeAndReturn) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const QString first_same =
      MakeImage(dir, "same_1.png", QSize(1000, 3000), QColor(180, 0, 0));
  const QString second_same =
      MakeImage(dir, "same_2.png", QSize(1000, 3000), QColor(0, 180, 0));
  const QString lower =
      MakeImage(dir, "lower.png", QSize(500, 1400), QColor(0, 0, 180));

  MainWindow window(QList<QString>{first_same, second_same, lower});
  window.resize(800, 600);
  window.show();
  QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

  SendCtrlArrow(window, Qt::Key_Right);
  SendCtrlArrow(window, Qt::Key_Right);
  ASSERT_EQ(window.sceneRect().size(), QSizeF(500, 1400));
  SendKey(window, Qt::Key_Plus);
  SendKey(window, Qt::Key_Plus);
  SendKey(window, Qt::Key_Plus);
  ASSERT_GT(window.verticalScrollBar()->maximum(), 0);

  window.verticalScrollBar()->setValue(window.verticalScrollBar()->maximum() *
                                       2 / 3);
  QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  const double before = ViewportCenterYFraction(window);

  SendCtrlArrow(window, Qt::Key_Right);
  SendCtrlArrow(window, Qt::Key_Left);
  ASSERT_EQ(window.sceneRect().size(), QSizeF(500, 1400));
  const double after = ViewportCenterYFraction(window);

  EXPECT_NEAR(after, before, 0.005);
}
