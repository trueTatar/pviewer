#include <gtest/gtest.h>

#include <vector>

#include "cache-builder.hpp"
#include "images_navigator.hpp"

/*
 * Each test drives a sequence of navigation commands and checks, first and
 * foremost, what the user actually sees (DisplayingResults -- the sequence of
 * displayed image identities). The secondary check, PerformedOperations, spells
 * out the navigation outcome of each step as readable Step flags.
 */

namespace {
constexpr Step Fwd = Step::Forward;
constexpr Step Bwd = Step::Backward;
constexpr Step Move = Step::Move;
constexpr Step Show = Step::ShowCached;
constexpr Step Enq = Step::Enqueue;
constexpr Step Hide = Step::HideAtEdge;
}  // namespace

TEST_F(ViewerFixture, TwoStepsLeftSideRightAway) {
  SeveralTimesTo<PreviousImage>(2);
  EXPECT_EQ(displayed_t({}), DisplayingResults());
  EXPECT_EQ(steps_t({Hide | Bwd, Hide | Bwd}), PerformedOperations());
}

TEST_F(ViewerFixture, SetCachedImage) {
  SeveralTimesTo<NextImage>(1);
  SeveralTimesTo<PreviousImage>(1);
  SeveralTimesTo<NextImage>(1);
  EXPECT_EQ(displayed_t({0, 0}), DisplayingResults());
  EXPECT_EQ(steps_t({Show | Fwd, Hide | Bwd, Show | Fwd}), PerformedOperations());
}

TEST_F(ViewerFixture, OneImageInList) {
  Step result = moveTo<NextImage>();
  EXPECT_EQ(displayed_t({0}), DisplayingResults());
  EXPECT_EQ(Show | Fwd, result);
}

TEST_F(ViewerFixture, SimpleCacheFilling) {
  SeveralTimesTo<NextImage>(4);
  EXPECT_EQ(displayed_t({0, 1, 2, 3}), DisplayingResults());
  EXPECT_EQ(steps_t({Show | Fwd, Move | Fwd, Move | Fwd, Move | Fwd}),
            PerformedOperations());
}

TEST_F(ViewerFixture, OneStepBackAfterCacheFilling) {
  SeveralTimesTo<NextImage>(4);
  SeveralTimesTo<PreviousImage>(1);
  SeveralTimesTo<NextImage>(2);
  EXPECT_EQ(displayed_t({0, 1, 2, 3, 2, 3, 4}), DisplayingResults());
  EXPECT_EQ(steps_t({Show | Fwd, Move | Fwd, Move | Fwd, Move | Fwd, Move | Bwd,
                     Move | Fwd, Move | Fwd}),
            PerformedOperations());
}

TEST_F(ViewerFixture, ReversedCacheUpdate) {
  SeveralTimesTo<NextImage>(4);
  SeveralTimesTo<PreviousImage>(3);
  EXPECT_EQ(displayed_t({0, 1, 2, 3, 2, 1, 0}), DisplayingResults());
  EXPECT_EQ(steps_t({Show | Fwd, Move | Fwd, Move | Fwd, Move | Fwd, Move | Bwd,
                     Move | Bwd, Move | Bwd}),
            PerformedOperations());
}

TEST_F(ViewerFixture, LeafListOverEntirely) {
  SeveralTimesTo<NextImage>(17);
  SeveralTimesTo<PreviousImage>(2);
  EXPECT_EQ(
      displayed_t({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 14, 13}),
      DisplayingResults());
  EXPECT_EQ(steps_t({Show | Fwd, Move | Fwd, Move | Fwd, Move | Fwd, Move | Fwd,
                     Move | Fwd, Enq | Move | Fwd, Enq | Move | Fwd,
                     Enq | Move | Fwd, Enq | Move | Fwd, Enq | Move | Fwd,
                     Move | Fwd, Move | Fwd, Move | Fwd, Move | Fwd, Hide | Fwd,
                     Hide | Fwd, Show | Bwd, Move | Bwd}),
            PerformedOperations());
}

TEST_F(ViewerFixture, LeafListOverEntirely_FromEnd) {
  moveTo<EndOfTheList>();
  SeveralTimesTo<PreviousImage>(17);
  SeveralTimesTo<NextImage>(3);
  EXPECT_EQ(
      displayed_t({14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2}),
      DisplayingResults());
  EXPECT_EQ(steps_t({Show | Bwd, Move | Bwd, Move | Bwd, Move | Bwd, Move | Bwd,
                     Move | Bwd, Enq | Move | Bwd, Enq | Move | Bwd,
                     Enq | Move | Bwd, Enq | Move | Bwd, Enq | Move | Bwd,
                     Move | Bwd, Move | Bwd, Move | Bwd, Move | Bwd, Hide | Bwd,
                     Hide | Bwd, Show | Fwd, Move | Fwd, Move | Fwd}),
            PerformedOperations());
}

/**************************
 * move to specific image *
 **************************/

TEST_F(ViewerFixture, JumpToSpecificImageAndMoveForward) {
  moveTo<ImageNumber>(4);
  SeveralTimesTo<NextImage>(4);
  EXPECT_EQ(displayed_t({3, 4, 5, 6}), DisplayingResults());
  EXPECT_EQ(steps_t({Show | Fwd, Move | Fwd, Move | Fwd, Enq | Move | Fwd}),
            PerformedOperations());
}

TEST_F(ViewerFixture, JumpToSpecificImageAndMoveBackward) {
  moveTo<ImageNumber>(8);
  SeveralTimesTo<PreviousImage>(4);
  EXPECT_EQ(displayed_t({7, 6, 5, 4}), DisplayingResults());
  EXPECT_EQ(steps_t({Show | Bwd, Enq | Move | Bwd, Enq | Move | Bwd,
                     Enq | Move | Bwd}),
            PerformedOperations());
}

TEST_F(ViewerFixture, JumpToSpecificImageAndMoveForwardAndBackward) {
  moveTo<ImageNumber>(4);
  SeveralTimesTo<NextImage>(1);
  SeveralTimesTo<PreviousImage>(1);
  EXPECT_EQ(displayed_t({3, 2}), DisplayingResults());
  EXPECT_EQ(steps_t({Show | Fwd, Move | Bwd}), PerformedOperations());
}

/*-----------------------------*
 | test reverse cache managing |
 *-----------------------------*/

TEST_F(ViewerFixture, SimpleCaching_ReversedOrder) {
  moveTo<EndOfTheList>();
  SeveralTimesTo<PreviousImage>(9);
  EXPECT_EQ(displayed_t({14, 13, 12, 11, 10, 9, 8, 7, 6}), DisplayingResults());
  EXPECT_EQ(steps_t({Show | Bwd, Move | Bwd, Move | Bwd, Move | Bwd, Move | Bwd,
                     Move | Bwd, Enq | Move | Bwd, Enq | Move | Bwd,
                     Enq | Move | Bwd}),
            PerformedOperations());
}

TEST_F(ViewerFixture, SettingPixmapFromCache_ReversedOrder) {
  moveTo<EndOfTheList>();
  SeveralTimesTo<PreviousImage>(1);
  SeveralTimesTo<NextImage>(1);
  SeveralTimesTo<PreviousImage>(1);
  EXPECT_EQ(displayed_t({14, 14}), DisplayingResults());
  EXPECT_EQ(steps_t({Show | Bwd, Hide | Fwd, Show | Bwd}), PerformedOperations());
}
