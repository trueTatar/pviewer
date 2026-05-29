#include <gtest/gtest.h>

#include <QApplication>
#include <QTimer>

int main(int argc, char* argv[]) {
  // Tests instantiate real QPixmap/QImage/QPainter, which need a GUI
  // application and a platform plugin. Default to the headless "offscreen"
  // plugin so the suite runs without a display, unless the caller overrides it.
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);
  QTimer::singleShot(0, [&]() {
    ::testing::InitGoogleTest(&argc, argv);
    //::testing::GTEST_FLAG(filter) = "*ImageChanging*";
    //::testing::GTEST_FLAG(filter) = "*ViewerFixture*";

    auto testResult = RUN_ALL_TESTS();
    app.exit(testResult);
  });
  return app.exec();
}