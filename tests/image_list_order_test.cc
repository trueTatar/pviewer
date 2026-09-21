#include "image_list_order.hpp"

#include <gtest/gtest.h>

TEST(ImageListOrderTest, SortsNewestImagesFirst) {
  QList<ImageListEntry> entries{{"old.png", 1000},
                                {"newest.png", 3000},
                                {"newer.png", 2000}};

  SortImagesNewestFirst(entries);

  EXPECT_EQ(ImagePaths(std::move(entries)),
            QList<QString>({"newest.png", "newer.png", "old.png"}));
}

TEST(ImageListOrderTest, UsesNaturalFileNameOrderForEqualTimestamps) {
  QList<ImageListEntry> entries{{"image10.png", 1000},
                                {"image2.png", 1000},
                                {"image1.png", 1000}};

  SortImagesNewestFirst(entries);

  EXPECT_EQ(ImagePaths(std::move(entries)),
            QList<QString>({"image1.png", "image2.png", "image10.png"}));
}
