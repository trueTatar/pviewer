#include <gtest/gtest.h>

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QKeyEvent>
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

  const QPointF center =
      window.mapToScene(window.viewport()->rect().center());
  return (center.y() - scene_rect.top()) / scene_rect.height();
}

}  // namespace

TEST(MainWindowViewportTest,
     KeepsLowerResolutionViewportAfterHideAtRightEdgeAndReturn) {
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
  ASSERT_GT(window.verticalScrollBar()->maximum(), 0);

  window.verticalScrollBar()->setValue(
      window.verticalScrollBar()->maximum() * 2 / 3);
  QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  const double before = ViewportCenterYFraction(window);

  SendCtrlArrow(window, Qt::Key_Right);
  SendCtrlArrow(window, Qt::Key_Left);
  ASSERT_EQ(window.sceneRect().size(), QSizeF(500, 1400));
  const double after = ViewportCenterYFraction(window);

  EXPECT_NEAR(after, before, 0.005);
}
