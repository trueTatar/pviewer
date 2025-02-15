#pragma once

#include <gtest/gtest.h>

#include <QTimer>
#include <algorithm>
#include <functional>
#include <memory>

#include "abstract_image_cache.hpp"
#include "abstract_image_location.hpp"
#include "abstract_task_queue.hpp"
#include "images_navigator.hpp"

struct Image {
  Image(int value) : value_(value) {}
  operator int() const { return value_; }
  int value_;
};

struct Info {
  Info(int value) : value_(value) {}
  operator int() const { return value_; }
  operator Image() const { return value_; }
  int value_;
};

using image_test_t = Image;
using info_test_t = Info;

using info_storage_test_t = QVector<info_test_t>;
using image_storage_test_t = QList<image_test_t>;

using info_t = info_storage_test_t::iterator;
using queue_storage_t = QList<info_t>;

class FakeTaskQueue : public Abstract::TaskQueue<info_test_t, image_test_t> {
 public:
  FakeTaskQueue(const int initial_task_size)
      : Abstract::TaskQueue<info_test_t, image_test_t>(initial_task_size) {}
  void PushBack(info_t value) override { queue_.push_back(value); }
  info_t PopFront() override {
    info_t value = queue_.front();
    queue_.pop_front();
    return value;
  }
  bool Empty() const override { return queue_.empty(); }
  QList<info_t> queue_;
};

struct elem_pos_t {
  explicit elem_pos_t(int v) : value(v) {}
  operator int() const { return value; }
  int value;
};

class FakeImagePathList
    : public Abstract::ImageLocation<info_test_t, image_test_t> {
  using smart_pointer_t = std::shared_ptr<FakeImagePathList>;

 public:
  FakeImagePathList(info_storage_test_t vec, info_t pos)
      : ImageLocation(pos), file_info_(std::move(vec)) {}
  static smart_pointer_t Construct(int size, elem_pos_t pos = elem_pos_t(-1)) {
    return Construct(0, size, pos);
  }
  static smart_pointer_t Construct(int begin, int end,
                                   elem_pos_t pos = elem_pos_t(-1)) {
    info_storage_test_t result;
    info_t iter;
    for (int i = begin; i != end; ++i) result.push_back(i);
    if (pos.value == -1)
      iter = result.begin();
    else
      iter = std::find(result.begin(), result.end(), pos.value);
    return std::make_shared<FakeImagePathList>(std::move(result), iter);
  }
  int size() const override { return file_info_.size(); }
  bool isEmpty() const override { return file_info_.empty(); }
  int Image() { return file_info_.at(Pos()); }
  info_t Begin() override { return file_info_.begin(); }
  info_t End() override { return file_info_.end(); }
  const_info_t CBegin() const override { return file_info_.begin(); }

  info_storage_test_t file_info_;
};

class FakeCachedImageList
    : public Abstract::ImageCache<info_test_t, image_test_t> {
 public:
  FakeCachedImageList(std::size_t capacity, info_t const& index,
                      FakeImagePathList& file_info)
      : ImageCache(index, capacity),
        is_image_null_(true),
        file_info_(file_info) {}
  bool isEmpty() const override { return cache_.empty(); }
  void Clear() override { cache_.clear(); }
  std::function<bool()> GetIsImageNullFunctor() {
    return [&] { return is_image_null_; };
  }
  void DisplayImage() final {
    displaying_test_result.push_back(file_info_.Image());
    is_image_null_ = false;
  }
  void HideImage() final { is_image_null_ = true; }
  void UpdateCurrentScaledImage() final {}

  void PopFront() override { cache_.pop_front(); }
  void PopBack() override { cache_.pop_back(); }
  std::size_t Size() const override { return cache_.size(); }
  void Push(pointer_t op, info_t value) override { (cache_.*op)(*value); }

 public:
  QList<image_test_t> cache_;
  std::vector<int> displaying_test_result;

 private:
  bool is_image_null_;
  FakeImagePathList& file_info_;
};

struct ViewerFixture : public ::testing::Test {
  using ImageNumber = ImageNumberImpl<info_test_t, image_test_t>;
  using EndOfTheList = EndOfTheListImpl<info_test_t, image_test_t>;
  using NextImage = ImageBase<NumericalOrder, info_test_t, image_test_t>;
  using PreviousImage = ImageBase<ReverseOrder, info_test_t, image_test_t>;

  using file_info_t = Abstract::ImageLocation<info_test_t, image_test_t>;
  using cached_images_t = Abstract::ImageCache<info_test_t, image_test_t>;

  using move_t = ImagesNavigator<info_test_t, image_test_t>;

 public:
  ViewerFixture() {
    auto [number_of_image, cache_capacity] =
        std::tuple<std::size_t, std::size_t>(15, 10);
    Configure(number_of_image, cache_capacity);
  }
  void Configure(std::size_t number_of_images, std::size_t cache_capacity) {
    images_ = FakeImagePathList::Construct(number_of_images);
    const int initial_task_queue = cache_capacity;
    images_->CreateTaskQueue<FakeTaskQueue>(initial_task_queue);
    FakeImagePathList::InitialImageTask(*images_, images_->TaskQueueObject());
    cache_ = images_->CreateCacheObject<FakeCachedImageList>(cache_capacity,
                                                             *images_);
    move_ = std::make_unique<move_t>(images_, cache_, nullptr,
                                     cache_->GetIsImageNullFunctor());
    images_->CacheImage();
  }
  template <typename WhereTo, typename... Args>
  int moveTo(Args... args) {
    return move_->moveTo<WhereTo>(args...);
  }
  template <typename WhereTo>
  void SeveralTimesTo(int time) {
    for (int i = 0; i != time; ++i)
      performed_operations_.push_back(moveTo<WhereTo>());
  }
  using expected_t = std::vector<int>;
  auto const& DisplayingResults() const {
    return cache_->displaying_test_result;
  }
  auto const& PerformedOperations() const { return performed_operations_; }

 public:
  std::shared_ptr<FakeImagePathList> images_;
  std::shared_ptr<FakeCachedImageList> cache_;
  std::unique_ptr<move_t> move_;
  std::vector<int> performed_operations_;
};
