#include <gtest/gtest.h>

#include "image_comparison_model.hpp"

namespace {

QVector<QString> Paths(std::initializer_list<const char*> paths) {
  QVector<QString> result;
  for (const char* path : paths) result.push_back(QString::fromUtf8(path));
  return result;
}

}  // namespace

TEST(ImageComparisonModelTest, EnabledPathsFollowCheckboxState) {
  ImageComparisonModel model;
  model.SetImages(Paths({"a.png", "b.png", "c.png"}));

  ASSERT_TRUE(model.SetEnabled(1, false));

  EXPECT_EQ(model.EnabledPaths(), Paths({"a.png", "c.png"}));
}

TEST(ImageComparisonModelTest, MoveReordersEnabledPaths) {
  ImageComparisonModel model;
  model.SetImages(Paths({"a.png", "b.png", "c.png"}));

  ASSERT_TRUE(model.Move(2, 0));

  EXPECT_EQ(model.EnabledPaths(), Paths({"c.png", "a.png", "b.png"}));
}

TEST(ImageComparisonModelTest, MovePathReordersByImageIdentity) {
  ImageComparisonModel model;
  model.SetImages(Paths({"a.png", "b.png", "c.png"}));

  ASSERT_TRUE(model.MovePath("b.png", 1));

  EXPECT_EQ(model.EnabledPaths(), Paths({"a.png", "c.png", "b.png"}));
  EXPECT_FALSE(model.MovePath("missing.png", 1));
  EXPECT_FALSE(model.MovePath("b.png", 1));
}

TEST(ImageComparisonModelTest, CurrentEnabledImageKeepsActivePosition) {
  ImageComparisonModel model;
  model.SetImages(Paths({"a.png", "b.png", "c.png"}));
  ASSERT_TRUE(model.Move(2, 0));

  EXPECT_EQ(model.EnabledPositionFor("b.png"), 3);
}

TEST(ImageComparisonModelTest, DisabledCurrentImageFallsBackToNearestEnabled) {
  ImageComparisonModel model;
  model.SetImages(Paths({"a.png", "b.png", "c.png", "d.png"}));
  ASSERT_TRUE(model.SetEnabled(1, false));

  EXPECT_EQ(model.EnabledPositionFor("b.png"), 2);
}

TEST(ImageComparisonModelTest, UnknownImageHasNoActivePosition) {
  ImageComparisonModel model;
  model.SetImages(Paths({"a.png", "b.png", "c.png"}));

  EXPECT_EQ(model.EnabledPositionFor("missing.png"), 0);
}

TEST(ImageComparisonModelTest, RemovePathDropsImageFromComparison) {
  ImageComparisonModel model;
  model.SetImages(Paths({"a.png", "b.png", "c.png"}));

  ASSERT_TRUE(model.RemovePath("b.png"));

  EXPECT_EQ(model.EnabledPaths(), Paths({"a.png", "c.png"}));
  EXPECT_FALSE(model.RemovePath("missing.png"));
}

TEST(ImageComparisonModelTest, NoEnabledImagesHaveNoActivePosition) {
  ImageComparisonModel model;
  model.SetImages(Paths({"a.png", "b.png"}));
  ASSERT_TRUE(model.SetEnabled(0, false));
  ASSERT_TRUE(model.SetEnabled(1, false));

  EXPECT_TRUE(model.EnabledPaths().isEmpty());
  EXPECT_EQ(model.EnabledPositionFor("a.png"), 0);
}
