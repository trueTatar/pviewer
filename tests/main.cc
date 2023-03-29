#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QTimer>

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  QTimer::singleShot(0, [&]() {
    ::testing::InitGoogleTest(&argc, argv);
    //::testing::GTEST_FLAG(filter) = "*ImageChanging*";
    //::testing::GTEST_FLAG(filter) = "*ViewerFixture*";

    auto testResult = RUN_ALL_TESTS();
    app.exit(testResult);
  });
  return app.exec();
}