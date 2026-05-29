#include <gtest/gtest.h>

#include <QColor>
#include <QFile>
#include <QImage>
#include <QPixmap>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QVector>
#include <memory>

#include "cached_images_list.hpp"
#include "images_navigator.hpp"
#include "lists.hpp"
#include "task_queue.hpp"

/*
 * Unlike cacher_test.cc / viewer_test.cc (which drive the abstract algorithm on
 * integer stand-ins), this suite exercises the *production* CachedImagesList on
 * real image files: the QtConcurrent decode, the lazy QFuture->QPixmap resolve,
 * scaling, the corrupt-file placeholder, and the alignment of the three
 * parallel lists (source_/scaled_/pending_).
 *
 * Each generated image is a solid colour whose red channel equals its index, so
 * the colour of whatever ends up displayed tells us exactly which slot was
 * resolved -- a wrong colour would expose a list misalignment.
 */
class CachedImagesListTest : public ::testing::Test {
 protected:
  using NextImage = ImageBase<NumericalOrder, QString, QPixmap>;
  using PreviousImage = ImageBase<ReverseOrder, QString, QPixmap>;
  using ImageNumber = ImageNumberImpl<QString, QPixmap>;

  static constexpr int kW = 200;
  static constexpr int kH = 150;

  QString MakeImage(int index) {
    QImage image(kW, kH, QImage::Format_RGB32);
    image.fill(QColor(index, 0, 0));
    QString path =
        tmp_.filePath(QStringLiteral("img_%1.png").arg(index, 3, 10, QChar('0')));
    image.save(path, "PNG");
    return path;
  }

  QString MakeCorrupt(int index) {
    QString path = tmp_.filePath(QStringLiteral("bad_%1.png").arg(index));
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write("this is not a valid png");
    file.close();
    return path;
  }

  QVector<QString> MakeImages(int count) {
    QVector<QString> paths;
    for (int i = 0; i < count; ++i) paths << MakeImage(i);
    return paths;
  }

  // Mirrors MainWindow::Construct: task queue, cache object, navigator.
  void Build(QVector<QString> paths, int capacity, int start_pos_1_based) {
    images_ = std::make_shared<ImagePath>();
    images_->CreateTaskQueue<TaskQueue>(capacity);
    cache_ = images_->CreateCacheObject<CachedImagesList>(
        capacity, [this](QPixmap const& p) { displayed_ = p; },
        [this] { return screen_width_; });
    cache_->SetImageSizePasser([] {}, [](QSize) {});
    folders_ = std::make_shared<FolderPath>();
    move_ = std::make_unique<ImagesNavigator<QString, QPixmap>>(
        images_, cache_, folders_, [this] { return displayed_.isNull(); });
    images_->setNewList(std::move(paths));
    move_->moveTo<ImageNumber>(start_pos_1_based);
  }

  template <typename Where, typename... Args>
  Step Go(Args... args) {
    return move_->moveTo<Where>(args...);
  }

  int DisplayedIndex() const {
    return displayed_.toImage().pixelColor(0, 0).red();
  }

  void TearDown() override {
    // Let background decodes finish before QTemporaryDir removes the files.
    QThreadPool::globalInstance()->waitForDone();
  }

  QTemporaryDir tmp_;
  int screen_width_ = 1000;
  QPixmap displayed_;
  std::shared_ptr<ImagePath> images_;
  std::shared_ptr<CachedImagesList> cache_;
  std::shared_ptr<FolderPath> folders_;
  std::unique_ptr<ImagesNavigator<QString, QPixmap>> move_;
};

// The decode future is resolved into a real, correctly-sized pixmap.
TEST_F(CachedImagesListTest, DecodesAndDisplaysRealPixmap) {
  Build(MakeImages(10), /*capacity=*/5, /*start=*/1);  // index 0
  cache_->DisplayImage();

  ASSERT_FALSE(displayed_.isNull());
  EXPECT_EQ(displayed_.size(), QSize(kW, kH));
  EXPECT_EQ(DisplayedIndex(), 0);
}

// Sliding the window forward (push_back + pop_front on all three lists) must
// keep source_/pending_ aligned: the resolved colour proves the right slot.
TEST_F(CachedImagesListTest, ForwardNavigationResolvesCorrectImages) {
  Build(MakeImages(10), /*capacity=*/5, /*start=*/1);  // index 0

  Go<NextImage>();  // first move only displays current
  EXPECT_EQ(DisplayedIndex(), 0);
  for (int expected = 1; expected <= 5; ++expected) {
    Go<NextImage>();
    EXPECT_EQ(DisplayedIndex(), expected);
    EXPECT_LE(cache_->Size(), static_cast<std::size_t>(5));
  }
}

// Same, sliding backward (push_front + pop_back).
TEST_F(CachedImagesListTest, BackwardNavigationResolvesCorrectImages) {
  Build(MakeImages(10), /*capacity=*/5, /*start=*/10);  // index 9

  Go<PreviousImage>();  // first move only displays current
  EXPECT_EQ(DisplayedIndex(), 9);
  for (int expected = 8; expected >= 4; --expected) {
    Go<PreviousImage>();
    EXPECT_EQ(DisplayedIndex(), expected);
    EXPECT_LE(cache_->Size(), static_cast<std::size_t>(5));
  }
}

// Toggling scale() swaps between source width and screen width, on the same
// image (colour is preserved through the scaling path).
TEST_F(CachedImagesListTest, ScalingTogglesWidthAndKeepsImage) {
  Build(MakeImages(5), /*capacity=*/5, /*start=*/3);  // index 2
  cache_->DisplayImage();
  EXPECT_EQ(displayed_.width(), kW);
  EXPECT_EQ(DisplayedIndex(), 2);

  cache_->scale();
  cache_->DisplayImage();
  EXPECT_EQ(displayed_.width(), screen_width_);
  EXPECT_EQ(DisplayedIndex(), 2);

  cache_->scale();
  cache_->DisplayImage();
  EXPECT_EQ(displayed_.width(), kW);
  EXPECT_EQ(DisplayedIndex(), 2);
}

// A file that fails to decode must show the visible placeholder, not a blank
// (null) pixmap.
TEST_F(CachedImagesListTest, CorruptImageShowsPlaceholder) {
  QVector<QString> paths;
  paths << MakeImage(0) << MakeCorrupt(1) << MakeImage(2);
  Build(std::move(paths), /*capacity=*/5, /*start=*/2);  // index 1 (corrupt)
  cache_->DisplayImage();

  ASSERT_FALSE(displayed_.isNull());
  EXPECT_EQ(displayed_.size(), QSize(640, 360));
}
