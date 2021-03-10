#include <gtest/gtest.h>

#include <list>
#include <queue>
#include <tuple>

#include "cache-builder.hpp"

using images_task_t = std::tuple<int, int, int, info_storage_test_t>;
class InitialImagesTask: public testing::TestWithParam<images_task_t> { };

bool operator==(const queue_storage_t& lhs, const info_storage_test_t& rhs) {
  auto r = rhs.begin();
  for (auto ref : lhs) {
    if (*ref != *r) return false;
    ++r;
  }
  return true;
}
bool operator==(const queue_storage_t& lhs, const image_storage_test_t& rhs) {
  auto r = rhs.begin();
  for (auto ref : lhs) {
    if (*ref != *r) return false;
    ++r;
  }
  return true;
}


using cache_building_t = std::tuple<int, int, int, image_storage_test_t, int, int, int>;
class CacheBuilding: public testing::TestWithParam<cache_building_t> { };

std::ostream& operator<<(std::ostream& os, const info_t& item) {
  return os << item->value_;
}

TEST_P(InitialImagesTask, TrueTesting) {
  auto [begin, end, index, start_queue] = GetParam();
  auto file_info = FakeImagePathList::Construct(begin, end, elem_pos_t(index));
  FakeImagePathList::InitialImageTask(*file_info, file_info->TaskQueue());
  FakeTaskQueue& reference = dynamic_cast<FakeTaskQueue&>(file_info->TaskQueue());
  EXPECT_EQ(reference.queue_, start_queue);
}

TEST_P(CacheBuilding, TrueTesting) {
  auto [begin, end, index, cache, index_in_cache, next, prev] = GetParam();
  auto file_info = FakeImagePathList::Construct(begin, end, elem_pos_t(index));
  FakeImagePathList::InitialImageTask(*file_info, file_info->TaskQueue());
  FakeTaskQueue& reference = dynamic_cast<FakeTaskQueue&>(file_info->TaskQueue());
  auto image_cache = file_info->CreateCacheObject<FakeCachedImageList>(10, *file_info);
  image_cache->FormCache(reference);
  EXPECT_EQ(reference.queue_, cache);
}

auto images_task = testing::Values(
  images_task_t(31, 36, 33, info_storage_test_t({ 33, 34, 32, 35, 31 })),
  images_task_t(0, 9, 5, info_storage_test_t({ 5, 6, 4, 7, 3 })),
  images_task_t(0, 3, 1, info_storage_test_t({ 1, 2, 0 })),
  images_task_t(3, 5, 3, info_storage_test_t({ 3, 4 })),
  images_task_t(5, 6, 5, info_storage_test_t({ info_test_t(5) })),
  images_task_t(0, 10, 0, info_storage_test_t({ 0, 1, 2, 3, 4 }))
);

auto cache_building = testing::Values(
  cache_building_t(31, 36, 33, image_storage_test_t({ 31, 32, 33, 34, 35 }), 33, 34, 32),
  cache_building_t(0, 9, 5, image_storage_test_t({ 3, 4, 5, 6, 7 }), 5, 6, 4),
  cache_building_t(0, 3, 1, image_storage_test_t({ 0, 1, 2 }), 1, 2, 0),
  cache_building_t(3, 5, 3, image_storage_test_t({ 3, 4 }), 3, 4, -1),
  cache_building_t(5, 6, 5, image_storage_test_t({ 5 }), 5, -1, -1),
  cache_building_t(0, 10, 0, image_storage_test_t({ 0, 1, 2, 3, 4 }), 0, 1, -1)
);

INSTANTIATE_TEST_SUITE_P(, InitialImagesTask, images_task);
INSTANTIATE_TEST_SUITE_P(, CacheBuilding, cache_building);

class ImageChanging: public ::testing::Test {
  using pointer_t = void(QList<image_test_t>::*)(const image_test_t&);
  static constexpr pointer_t Back = &QList<image_test_t>::push_back;
  static constexpr pointer_t Front = &QList<image_test_t>::push_front;
  image_storage_test_t& Cache() {
    return image_cache_->cache_;
  }
  int Index() {
    return *file_info_->Index();
  }
  int LeftEdge() {
    return *image_cache_->LeftEdge();
  }
  int RightEdge() {
    return *image_cache_->RightEdge();
  }
public:
  std::shared_ptr<FakeImagePathList> file_info_;
  std::shared_ptr<FakeCachedImageList> image_cache_;
  FakeTaskQueue* task_queue_;
  void SetObject(int begin, int end, int index, int capacity) {
    file_info_ = FakeImagePathList::Construct(begin, end, elem_pos_t(index));
    FakeImagePathList::InitialImageTask(*file_info_, file_info_->TaskQueue());
    task_queue_ = dynamic_cast<FakeTaskQueue*>(&file_info_->TaskQueue());
    image_cache_ = file_info_->CreateCacheObject<FakeCachedImageList>(capacity, *file_info_);
    image_cache_->FormCache(*task_queue_);
  }
  void FirstSetup() {
    SetObject(0, 10, 5, 6);
  }
  void SecondSetup() {
    SetObject(0, 10, 4, 6);
  }

  template<typename In, typename Im>
  void ChangeImage(int direction, Abstract::ImageLocation<In, Im>& file_info, Abstract::ImageCache<In, Im>& image_cache, Abstract::TaskQueue<In, Im>& task_queue) {
    file_info.MoveIndex(direction);
    auto first = file_info.Begin();
    auto last = std::prev(file_info.End());
    if (image_cache.LeftEdge() == first && direction < 0) return;
    if (image_cache.RightEdge() == last && direction > 0) return;
    if (int shift = image_cache.AddQuery(); shift) {
      auto iter = file_info.Index();
      std::advance(iter, shift);
      direction > 0 ? task_queue.template Push<Back>(iter) : task_queue.template Push<Front>(iter);
      image_cache.HandleCacheTask(task_queue);
      image_cache.RemoveOutdated(direction);
    }
  }
  void ChangeImage(int direction) {
    ChangeImage<info_test_t, image_test_t>(direction, *file_info_, *image_cache_, *task_queue_);
  }

  template<int step_number>
  void ChangeImage(int direction) {
    for (int i = step_number; i > 0; --i)
      ChangeImage(direction);
  }
  void CheckCache(image_storage_test_t expected) {
    EXPECT_EQ(expected, Cache());
  }
  void CheckIndex(int expected) {
    EXPECT_EQ(expected, Index());
  }
  void CheckBorders(int left, int right) {
    EXPECT_EQ(left, LeftEdge());
    EXPECT_EQ(right, RightEdge());
  }
};

class ICForward: public ImageChanging { };

TEST_F(ICForward, ObjectSetup) {
  FirstSetup();
  CheckCache({ 3, 4, 5, 6, 7 });
  CheckBorders(3, 7);
  CheckIndex(5);
}

TEST_F(ICForward, StepOne) {
  FirstSetup();
  ChangeImage<1>(1);
  CheckCache({ 3, 4, 5, 6, 7, 8 });
  CheckBorders(3, 8);
  CheckIndex(6);
}

TEST_F(ICForward, StepTwo) {
  FirstSetup();
  ChangeImage<2>(1);
  CheckCache({ 4, 5, 6, 7, 8, 9 });
  CheckBorders(4, 9);
  CheckIndex(7);
}

TEST_F(ICForward, StepThree) {
  FirstSetup();
  ChangeImage<3>(1);
  CheckCache({ 4, 5, 6, 7, 8, 9 });
  CheckBorders(4, 9);
  CheckIndex(8);
}

class ICBackward: public ImageChanging { };

TEST_F(ICBackward, ObjectSetup) {
  SecondSetup();
  CheckCache({ 2, 3, 4, 5, 6 });
  CheckBorders(2, 6);
  CheckIndex(4);
}

TEST_F(ICBackward, StepOne) {
  SecondSetup();
  ChangeImage<1>(-1);
  CheckCache({ 1, 2, 3, 4, 5, 6 });
  CheckBorders(1, 6);
  CheckIndex(3);
}

TEST_F(ICBackward, StepTwo) {
  SecondSetup();
  ChangeImage<2>(-1);
  CheckCache({ 0, 1, 2, 3, 4, 5 });
  CheckBorders(0, 5);
  CheckIndex(2);
}

TEST_F(ICBackward, StepThree) {
  SecondSetup();
  ChangeImage<3>(-1);
  CheckCache({ 0, 1, 2, 3, 4, 5 });
  CheckBorders(0, 5);
  CheckIndex(1);
}
