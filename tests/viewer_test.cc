#include <gtest/gtest.h>

#include <vector>

#include "cache-builder.hpp"
#include "images_navigator.hpp"

/*  Return codes
 *      1   Move iterator and display image that is currently pointed at
 *     10   Add Next Image to Task Queue
 *    100   Display image (if it were hidden)
 *   1000   Hide Image if edge
 */

TEST_F(ViewerFixture, TwoStepsLeftSideRightAway) {
  SeveralTimesTo<PreviousImage>(2);
  EXPECT_EQ(expected_t({2000, 2000}), PerformedOperations());
  EXPECT_EQ(expected_t({}), DisplayingResults());
}

TEST_F(ViewerFixture, SetCachedImage) {
  SeveralTimesTo<NextImage>(1);
  SeveralTimesTo<PreviousImage>(1);
  SeveralTimesTo<NextImage>(1);
  EXPECT_EQ(expected_t({100, 2000, 100}), PerformedOperations());
  EXPECT_EQ(expected_t({0, 0}), DisplayingResults());
}

TEST_F(ViewerFixture, OneImageInList) {
  int result = moveTo<NextImage>();
  EXPECT_EQ(result, 100);
  EXPECT_EQ(expected_t({0}), DisplayingResults());
}

TEST_F(ViewerFixture, SimpleCacheFilling) {
  SeveralTimesTo<NextImage>(4);
  EXPECT_EQ(expected_t({100, 1, 1, 1}), PerformedOperations());
  EXPECT_EQ(expected_t({0, 1, 2, 3}), DisplayingResults());
}

TEST_F(ViewerFixture, OneStepBackAfterCacheFilling) {
  SeveralTimesTo<NextImage>(4);
  SeveralTimesTo<PreviousImage>(1);
  SeveralTimesTo<NextImage>(2);
  EXPECT_EQ(expected_t({100, 1, 1, 1, 2, 1, 1}), PerformedOperations());
  EXPECT_EQ(expected_t({0, 1, 2, 3, 2, 3, 4}), DisplayingResults());
}

TEST_F(ViewerFixture, ReversedCacheUpdate) {
  SeveralTimesTo<NextImage>(4);
  SeveralTimesTo<PreviousImage>(3);
  EXPECT_EQ(expected_t({100, 1, 1, 1, 2, 2, 2}), PerformedOperations());
  EXPECT_EQ(expected_t({0, 1, 2, 3, 2, 1, 0}), DisplayingResults());
}

TEST_F(ViewerFixture, LeafListOverEntirely) {
  SeveralTimesTo<NextImage>(17);
  SeveralTimesTo<PreviousImage>(2);
  EXPECT_EQ(expected_t({100, 1, 1, 1, 1, 1, 11, 11, 11, 11, 11, 1, 1, 1, 1,
                        1000, 1000, 200, 2}),
            PerformedOperations());
  EXPECT_EQ(
      expected_t({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 14, 13}),
      DisplayingResults());
}

TEST_F(ViewerFixture, LeafListOverEntirely_FromEnd) {
  moveTo<EndOfTheList>();
  SeveralTimesTo<PreviousImage>(17);
  SeveralTimesTo<NextImage>(3);
  EXPECT_EQ(expected_t({200, 2, 2, 2, 2, 2,    12,   12,  12, 12,
                        12,  2, 2, 2, 2, 2000, 2000, 100, 1,  1}),
            PerformedOperations());
  EXPECT_EQ(
      expected_t({14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2}),
      DisplayingResults());
}

/**************************
 * move to specific image *
 **************************/

TEST_F(ViewerFixture, JumpToSpecificImageAndMoveForward) {
  moveTo<ImageNumber>(4);
  SeveralTimesTo<NextImage>(4);
  EXPECT_EQ(expected_t({100, 1, 1, 11}), PerformedOperations());
  EXPECT_EQ(expected_t({3, 4, 5, 6}), DisplayingResults());
}

TEST_F(ViewerFixture, JumpToSpecificImageAndMoveBackward) {
  moveTo<ImageNumber>(8);
  SeveralTimesTo<PreviousImage>(4);
  EXPECT_EQ(expected_t({200, 12, 12, 12}), PerformedOperations());
  EXPECT_EQ(expected_t({7, 6, 5, 4}), DisplayingResults());
}

TEST_F(ViewerFixture, JumpToSpecificImageAndMoveForwardAndBackward) {
  moveTo<ImageNumber>(4);
  SeveralTimesTo<NextImage>(1);
  SeveralTimesTo<PreviousImage>(1);
  EXPECT_EQ(expected_t({100, 2}), PerformedOperations());
  EXPECT_EQ(expected_t({3, 2}), DisplayingResults());
}

/*-----------------------------*
 | test reverse cache managing |
 *-----------------------------*/

TEST_F(ViewerFixture, SimpleCaching_ReversedOrder) {
  moveTo<EndOfTheList>();
  SeveralTimesTo<PreviousImage>(9);
  EXPECT_EQ(expected_t({200, 2, 2, 2, 2, 2, 12, 12, 12}),
            PerformedOperations());
  EXPECT_EQ(expected_t({14, 13, 12, 11, 10, 9, 8, 7, 6}), DisplayingResults());
}

TEST_F(ViewerFixture, SettingPixmapFromCache_ReversedOrder) {
  moveTo<EndOfTheList>();
  SeveralTimesTo<PreviousImage>(1);
  SeveralTimesTo<NextImage>(1);
  SeveralTimesTo<PreviousImage>(1);
  EXPECT_EQ(expected_t({200, 1000, 200}), PerformedOperations());
  EXPECT_EQ(expected_t({14, 14}), DisplayingResults());
}

/*---------------------------------------------------*
 | testing indexes which is using for caching images |
 *---------------------------------------------------*/
/*
TEST_F(ViewerFixture, CachingAtTheEndAndThenAtTheBegin) {
  auto [number_of_images, cache_capacity] = std::make_pair(10, 4);
  Configure(number_of_images, cache_capacity);
  SeveralTimesTo<NextImage>(8);
  SeveralTimesTo<PreviousImage>(6);
   EXPECT_EQ(expected_t({ 1, 1, 1, 1, 2, 2, 2, 2, 30, 30, 30, 20, 20, 20 }),
   PerformedOperations());
  EXPECT_EQ(expected_t({0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1}),
            DisplayingResults());
   EXPECT_EQ(expected_t({ 0, 1, 2, 3, 3, 3, 3, 3,              0,  0,  0 }),
   CachingResults()); EXPECT_EQ(expected_t({ 0, 0, 0, 0,              3,  3,
   3 }), RemovingResults());
}

TEST_F(ViewerFixture, CachingAtTheBegin) {
  auto [number_of_images, cache_capacity] = std::make_pair(10, 4);
  Configure(number_of_images, cache_capacity);
  moveTo<EndOfTheList>();
  SeveralTimesTo<PreviousImage>(8);
  SeveralTimesTo<NextImage>(6);
  EXPECT_EQ(expected_t({10, 10, 10, 10, 20, 20, 20, 20, 3, 3, 3, 2, 2, 2}),
            PerformedOperations());
  EXPECT_EQ(expected_t({9, 8, 7, 6, 5, 4, 3, 2, 3, 4, 5, 6, 7, 8}),
            DisplayingResults());
  EXPECT_EQ(expected_t({0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3}), CachingResults());
  EXPECT_EQ(expected_t({3, 3, 3, 3, 0, 0, 0}), RemovingResults());
}
*/