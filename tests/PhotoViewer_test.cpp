#include <gtest/gtest.h>
#include <vector>
#include "PhotoViewer.hpp"
#include <QtWidgets>

/*  Return codes
 *  1 - Filling Cache
 *  2 - Updating Cache
 *  3 - Moving Through Cache
 *  4 - Setting Image With Index From Cache
 *  5 - No Action
 */

class FakeImagePathList : public ImagePathList {
public:
  void setSize(int value) {
    for (int i = 0; i != value; ++i)
      container_.push_back("");
  }
};

class FakeCachedImageList : public CachedImageList {
public:
  FakeCachedImageList(QWidget& wgt, std::shared_ptr<ImagePathList> p) : CachedImageList(wgt, p) { }
  std::vector<int> caching_test_result;
  std::vector<int> removing_test_result;
  int __cache_size;
  int cacheSize() const override { return __cache_size; }
  void cacheCurrentImage() override {
    container_.push_back(std::move(QPixmap()));
    scaled_.push_back(std::move(QPixmap()));
    caching_test_result.push_back(index_);
  }
  int removeOutdatedImage() override {
    removing_test_result.push_back(CachedImageList::removeOutdatedImage());
    return -42;
  }
};

struct ImageViewerFixture : public ::testing::Test, public PhotoViewer {
public:
  void createContainers() override {
    paths = std::make_shared<FakeImagePathList>();
    cache = std::make_shared<FakeCachedImageList>(*this, paths);
  }
  void formatWidget() override { }
  void SetUp() override {
    prepareWidget();
    fake_cache()->__cache_size = 2;
    fake_paths()->setSize(10);
  }
  bool pixmapIsNull() override {
    return false;
  }
  std::shared_ptr<FakeCachedImageList> fake_cache() {
    return std::dynamic_pointer_cast<FakeCachedImageList, CachedImageList>(cache);
  }
  std::shared_ptr<FakeImagePathList> fake_paths() {
    return dynamic_pointer_cast<FakeImagePathList, ImagePathList>(paths);
  }
  std::vector<int> res;
  const std::vector<int>& nextPixmap_test(int time) {
    for(int i = 0; i != time; ++i) {
      res.push_back(goTo(NextImage));
    }
    return res;
  }
  std::vector<int>& previousPixmap_test(int time) {
    for(int i = 0; i != time; ++i) {
      res.push_back(goTo(PreviousImage));
    }
    return res;
  }
};

TEST_F(ImageViewerFixture, TwoStepsLeftSideRightAway)
{
  previousPixmap_test(2);
  EXPECT_EQ(std::vector<int>({50,50}), res);
}

TEST_F(ImageViewerFixture, SetCachedImage)
{
  nextPixmap_test(1);
  previousPixmap_test(1);
  nextPixmap_test(1);
  EXPECT_EQ(std::vector<int>({1,50,4}), res);
}

TEST_F(ImageViewerFixture, OneImageInList)
{
  EXPECT_EQ(goTo(NextImage), 1);
}

TEST_F(ImageViewerFixture, SimpleCacheFilling)
{
  nextPixmap_test(4);
  EXPECT_EQ(std::vector<int>({1,1,2,2}), res);
}

TEST_F(ImageViewerFixture, OneStepBackAfterCacheFilling)
{
  nextPixmap_test(4);
  previousPixmap_test(1);
  nextPixmap_test(2);
  EXPECT_EQ(std::vector<int>({1,1,2,2,30,3,2}), res);
}

TEST_F(ImageViewerFixture, ReversedCacheUpdate)
{
  nextPixmap_test(4);
  previousPixmap_test(3);
  EXPECT_EQ(std::vector<int>({1,1,2,2,30,20,20}), res);
}

TEST_F(ImageViewerFixture, LeafListOverEntirely)
{
  nextPixmap_test(12);
  previousPixmap_test(2);
  EXPECT_EQ(std::vector<int>({1,1,2,2,2,2,2,2,2,2,5,5,40,30}), res);
}

/*******************************
 * test reverse cache managing *
 *******************************/

TEST_F(ImageViewerFixture, SimpleCaching_ReversedOrder)
{
  reset(PreviousImage);
  previousPixmap_test(4);
  EXPECT_EQ(std::vector<int>({10,10,20,20}), res);
}

TEST_F(ImageViewerFixture, SettingPixmapFromCache_ReversedOrder)
{
  reset(PreviousImage);
  previousPixmap_test(1);
  nextPixmap_test(1);
  previousPixmap_test(1);
  EXPECT_EQ(std::vector<int>({10, 5, 40}), res);
}

/*****************************************************
 * testing indexes which is using for caching images *
 *****************************************************/

TEST_F(ImageViewerFixture, PlainImageCaching)
{
  fake_paths()->setSize(12);
  fake_cache()->__cache_size = 10;
  nextPixmap_test(12);
  EXPECT_EQ(std::vector<int>({1,1,1,1,1,1,1,1,1,1,2,2}), res);
  EXPECT_EQ(std::vector<int>({0,1,2,3,4,5,6,7,8,9,9,9}), fake_cache()->caching_test_result);
  EXPECT_EQ(std::vector<int>({0,0}), fake_cache()->removing_test_result);
}

TEST_F(ImageViewerFixture, CachingAtTheEndAndThenAtTheBegin)
{
  fake_cache()->__cache_size = 4;
  nextPixmap_test(8);
  previousPixmap_test(6);
  EXPECT_EQ(std::vector<int>({1,1,1,1,2,2,2,2,30,30,30,20,20,20}), res);
  EXPECT_EQ(std::vector<int>({0,1,2,3,3,3,3,3,0,0,0}), fake_cache()->caching_test_result);
  EXPECT_EQ(std::vector<int>({0,0,0,0,3,3,3}), fake_cache()->removing_test_result);
}

TEST_F(ImageViewerFixture, CachingAtTheBegin)
{
  reset(PreviousImage);
  fake_cache()->__cache_size = 4;
  previousPixmap_test(8);
  nextPixmap_test(6);
  EXPECT_EQ(std::vector<int>({10,10,10,10,20,20,20,20,3,3,3,2,2,2}), res);
  EXPECT_EQ(std::vector<int>({0,0,0,0,0,0,0,0,3,3,3}), fake_cache()->caching_test_result);
  EXPECT_EQ(std::vector<int>({3,3,3,3,0,0,0}), fake_cache()->removing_test_result);
}

int main( int argc, char* argv[] )
{
  QApplication app( argc, argv );

  QTimer::singleShot( 0, [&] ()
    {
      ::testing::InitGoogleTest( &argc, argv );
      auto testResult = RUN_ALL_TESTS();
      app.exit( testResult );
    } );

  return app.exec();
}