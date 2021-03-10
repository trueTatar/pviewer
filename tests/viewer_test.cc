#include <gtest/gtest.h>
#include <vector>
#include <QtWidgets>
#include "move.h"

#include "cache-builder.hpp"
/*  Return codes
 *  1 - Filling Cache
 *  2 - Updating Cache
 *  3 - Moving Through Cache
 *  4 - Setting Image With Index From Cache
 *  5 - No Action
 */

TEST_F(ViewerFixture, TwoStepsLeftSideRightAway) {
  SeveralTimesTo<PreviousImage>(2);
  EXPECT_EQ(expected_t({ 50, 50 }), PerformedOperations());
  EXPECT_EQ(expected_t({ }), DisplayingResults());
}

TEST_F(ViewerFixture, SetCachedImage) {
  SeveralTimesTo<NextImage>(1);
  SeveralTimesTo<PreviousImage>(1);
  SeveralTimesTo<NextImage>(1);
  EXPECT_EQ(expected_t({ 1, 50, 4 }), PerformedOperations());
  EXPECT_EQ(expected_t({ 0, 0 }), DisplayingResults());
}

TEST_F(ViewerFixture, OneImageInList) {
  int result = moveTo<NextImage>();
  EXPECT_EQ(result, 1);
  EXPECT_EQ(expected_t({ 0 }), DisplayingResults());
}

TEST_F(ViewerFixture, SimpleCacheFilling) {
  SeveralTimesTo<NextImage>(4);
  EXPECT_EQ(expected_t({ 1, 1, 2, 2 }), PerformedOperations());
  EXPECT_EQ(expected_t({ 0, 1, 2, 3 }), DisplayingResults());
}

TEST_F(ViewerFixture, OneStepBackAfterCacheFilling) {
  SeveralTimesTo<NextImage>(4);
  SeveralTimesTo<PreviousImage>(1);
  SeveralTimesTo<NextImage>(2);
  EXPECT_EQ(expected_t({ 1, 1, 2, 2, 30, 3, 2 }), PerformedOperations());
  EXPECT_EQ(expected_t({ 0, 1, 2, 3,  2, 3, 4 }), DisplayingResults());
}

TEST_F(ViewerFixture, ReversedCacheUpdate) {
  SeveralTimesTo<NextImage>(4);
  SeveralTimesTo<PreviousImage>(3);
  EXPECT_EQ(expected_t({ 1, 1, 2, 2, 30, 20, 20 }), PerformedOperations());
  EXPECT_EQ(expected_t({ 0, 1, 2, 3,  2,  1,  0 }), DisplayingResults());
}

TEST_F(ViewerFixture, LeafListOverEntirely) {
  SeveralTimesTo<NextImage>(12);
  SeveralTimesTo<PreviousImage>(2);
  EXPECT_EQ(expected_t({ 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 5, 5, 40, 30 }), PerformedOperations());
  EXPECT_EQ(expected_t({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,        9,  8 }), DisplayingResults());
}

/**************************
 * move to specific image *
 **************************/

TEST_F(ViewerFixture, JumpToSpecificImageAndMoveForward) {
  moveTo<ImageNumber>(4); // while ImageList is 10
  SeveralTimesTo<NextImage>(4);
  EXPECT_EQ(expected_t({ 1, 1, 2, 2 }), PerformedOperations());
  EXPECT_EQ(expected_t({ 3, 4, 5, 6 }), DisplayingResults());
}

TEST_F(ViewerFixture, JumpToSpecificImageAndMoveBackward) {
  moveTo<ImageNumber>(8); // while ImageList is 10
  SeveralTimesTo<PreviousImage>(4);
  EXPECT_EQ(expected_t({ 10, 10, 20, 20 }), PerformedOperations());
  EXPECT_EQ(expected_t({  7,  6,  5,  4 }), DisplayingResults());
}

TEST_F(ViewerFixture, JumpToSpecificImageAndMoveForwardAndBackward) {
  moveTo<ImageNumber>(4); // while ImageList is 10
  SeveralTimesTo<NextImage>(1);
  SeveralTimesTo<PreviousImage>(1);
  EXPECT_EQ(expected_t({ 1, 10 }), PerformedOperations());
  EXPECT_EQ(expected_t({ 3,  2 }), DisplayingResults());
}

/*-----------------------------*
 | test reverse cache managing |
 *-----------------------------*/

TEST_F(ViewerFixture, SimpleCaching_ReversedOrder) {
  moveTo<EndOfTheList>();
  SeveralTimesTo<PreviousImage>(4);
  EXPECT_EQ(expected_t({ 10, 10, 20, 20 }), PerformedOperations());
  EXPECT_EQ(expected_t({  9,  8,  7,  6 }), DisplayingResults());
}

TEST_F(ViewerFixture, SettingPixmapFromCache_ReversedOrder) {
  moveTo<EndOfTheList>();
  SeveralTimesTo<PreviousImage>(1);
  SeveralTimesTo<NextImage>(1);
  SeveralTimesTo<PreviousImage>(1);
  EXPECT_EQ(expected_t({ 10, 5, 40 }), PerformedOperations());
  EXPECT_EQ(expected_t({  9,     9 }), DisplayingResults());
}

/*---------------------------------------------------*
 | testing indexes which is using for caching images |
 *---------------------------------------------------*/

TEST_F(ViewerFixture, PlainImageCaching) {
  auto [number_of_images, cache_capacity] = std::make_pair(12, 10);
  Configure(number_of_images, cache_capacity);
  SeveralTimesTo<NextImage>(12);
  EXPECT_EQ(expected_t({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  2,  2 }), PerformedOperations());
  EXPECT_EQ(expected_t({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }), DisplayingResults());
  EXPECT_EQ(expected_t({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,  9,  9 }), CachingResults());
  EXPECT_EQ(expected_t({                                0,  0 }), RemovingResults());
}

TEST_F(ViewerFixture, CachingAtTheEndAndThenAtTheBegin) {
  auto [number_of_images, cache_capacity] = std::make_pair(10, 4);
  Configure(number_of_images, cache_capacity);
  SeveralTimesTo<NextImage>(8);
  SeveralTimesTo<PreviousImage>(6);
  EXPECT_EQ(expected_t({ 1, 1, 1, 1, 2, 2, 2, 2, 30, 30, 30, 20, 20, 20 }), PerformedOperations());
  EXPECT_EQ(expected_t({ 0, 1, 2, 3, 4, 5, 6, 7,  6,  5,  4,  3,  2,  1 }), DisplayingResults());
  EXPECT_EQ(expected_t({ 0, 1, 2, 3, 3, 3, 3, 3,              0,  0,  0 }), CachingResults());
  EXPECT_EQ(expected_t({             0, 0, 0, 0,              3,  3,  3 }), RemovingResults());
}

TEST_F(ViewerFixture, CachingAtTheBegin) {
  auto [number_of_images, cache_capacity] = std::make_pair(10, 4);
  Configure(number_of_images, cache_capacity);
  moveTo<EndOfTheList>();
  SeveralTimesTo<PreviousImage>(8);
  SeveralTimesTo<NextImage>(6);
  EXPECT_EQ(expected_t({ 10, 10, 10, 10, 20, 20, 20, 20, 3, 3, 3, 2, 2, 2 }), PerformedOperations());
  EXPECT_EQ(expected_t({  9,  8,  7,  6,  5,  4,  3,  2, 3, 4, 5, 6, 7, 8 }), DisplayingResults());
  EXPECT_EQ(expected_t({  0,  0,  0,  0,  0,  0,  0,  0,          3, 3, 3 }), CachingResults());
  EXPECT_EQ(expected_t({                  3,  3,  3,  3,          0, 0, 0 }), RemovingResults());
}

int main(int argc, char* argv[]) {
  QApplication app( argc, argv );
  QTimer::singleShot(0, [&] () {
    ::testing::InitGoogleTest(&argc, argv);
    //::testing::GTEST_FLAG(filter) = "*PlainImage*";
    //::testing::GTEST_FLAG(filter) = "*ViewerFixture*";
    //::testing::GTEST_FLAG(filter) = "*ViewerFixture.SetCachedImage*";
    //::testing::GTEST_FLAG(filter) = "*InitialImagesTask.TrueTesting*";
    auto testResult = RUN_ALL_TESTS();
    app.exit(testResult);
  });
  return app.exec();
}
