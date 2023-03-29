#include <gtest/gtest.h>

#include <list>
#include <queue>
#include <tuple>

#include "cache-builder.hpp"
#include "images_navigator.hpp"

using images_task_t = std::tuple<int, int, int, info_storage_test_t>;
class InitialImagesTask : public testing::TestWithParam<images_task_t> {};

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

using cache_building_t =
    std::tuple<int, int, int, image_storage_test_t, int, int, int>;
class CacheBuilding : public testing::TestWithParam<cache_building_t> {};

std::ostream& operator<<(std::ostream& os, const info_t& item) {
  return os << item->value_;
}

TEST_P(InitialImagesTask, TrueTesting) {
  auto [begin, end, index, start_queue] = GetParam();
  auto file_info = FakeImagePathList::Construct(begin, end, elem_pos_t(index));
  int initial_task_size = 5;
  file_info->CreateTaskQueue<FakeTaskQueue>(initial_task_size);
  FakeImagePathList::InitialImageTask(*file_info, file_info->TaskQueueObject());
  FakeTaskQueue& reference =
      dynamic_cast<FakeTaskQueue&>(file_info->TaskQueueObject());
  EXPECT_EQ(reference.queue_, start_queue);
}

TEST_P(CacheBuilding, TrueTesting) {
  auto [begin, end, index, cache, index_in_cache, next, prev] = GetParam();
  auto file_info = FakeImagePathList::Construct(begin, end, elem_pos_t(index));
  int initial_task_size, cache_capacity;
  initial_task_size = cache_capacity = 5;
  file_info->CreateTaskQueue<FakeTaskQueue>(initial_task_size);
  FakeImagePathList::InitialImageTask(*file_info, file_info->TaskQueueObject());
  FakeTaskQueue& reference =
      dynamic_cast<FakeTaskQueue&>(file_info->TaskQueueObject());
  auto image_cache = file_info->CreateCacheObject<FakeCachedImageList>(
      cache_capacity, *file_info);
  image_cache->ProcessTaskQueue(reference);
  EXPECT_EQ(reference.queue_, cache);
}

auto images_task = testing::Values(
    images_task_t(31, 36, 33, info_storage_test_t({33, 34, 32, 35, 31})),
    images_task_t(0, 9, 5, info_storage_test_t({5, 6, 4, 7, 3})),
    images_task_t(0, 3, 1, info_storage_test_t({1, 2, 0})),
    images_task_t(3, 5, 3, info_storage_test_t({3, 4})),
    images_task_t(5, 6, 5, info_storage_test_t({info_test_t(5)})),
    images_task_t(0, 10, 0, info_storage_test_t({0, 1, 2, 3, 4})));

auto cache_building = testing::Values(
    cache_building_t(31, 36, 33, image_storage_test_t({31, 32, 33, 34, 35}), 33,
                     34, 32),
    cache_building_t(0, 9, 5, image_storage_test_t({3, 4, 5, 6, 7}), 5, 6, 4),
    cache_building_t(0, 3, 1, image_storage_test_t({0, 1, 2}), 1, 2, 0),
    cache_building_t(3, 5, 3, image_storage_test_t({3, 4}), 3, 4, -1),
    cache_building_t(5, 6, 5, image_storage_test_t({5}), 5, -1, -1),
    cache_building_t(0, 10, 0, image_storage_test_t({0, 1, 2, 3, 4}), 0, 1,
                     -1));

INSTANTIATE_TEST_SUITE_P(, InitialImagesTask, images_task);
INSTANTIATE_TEST_SUITE_P(, CacheBuilding, cache_building);

template <int number_of_images, int index, int capacity>
struct ImageChanging : public ::testing::Test {
  using pointer_t = void (QList<image_test_t>::*)(const image_test_t&);
  static constexpr pointer_t Back = &QList<image_test_t>::push_back;
  static constexpr pointer_t Front = &QList<image_test_t>::push_front;
  using move_t = ImagesNavigator<info_test_t, image_test_t>;
  using ImageNumber = ImageNumberImpl<info_test_t, image_test_t>;
  using EndOfTheList = EndOfTheListImpl<info_test_t, image_test_t>;
  using NextImage = ImageBase<NumericalOrder, info_test_t, image_test_t>;
  using PreviousImage = ImageBase<ReverseOrder, info_test_t, image_test_t>;
  image_storage_test_t& Cache() { return image_cache_->cache_; }
  int Index() { return *file_info_->Index(); }
  int LeftEdge() { return *image_cache_->LeftEdge(); }
  int RightEdge() { return *image_cache_->RightEdge(); }

 public:
  std::shared_ptr<FakeImagePathList> file_info_;
  std::shared_ptr<FakeCachedImageList> image_cache_;
  std::unique_ptr<move_t> move_;
  FakeTaskQueue* task_queue_;
  ImageChanging() {
    file_info_ = FakeImagePathList::Construct(number_of_images);
    int initial_task_size = capacity;
    file_info_->CreateTaskQueue<FakeTaskQueue>(initial_task_size);
    task_queue_ = dynamic_cast<FakeTaskQueue*>(&file_info_->TaskQueueObject());
    image_cache_ = file_info_->CreateCacheObject<FakeCachedImageList>(
        capacity, *file_info_);
    move_ = std::make_unique<move_t>(file_info_, image_cache_, nullptr,
                                     image_cache_->GetIsImageNullFunctor());
    move_->moveTo<ImageNumber>(index);
  }
  template <typename WhereTo, typename... Args>
  int moveTo(Args... args) {
    return move_->moveTo<WhereTo>(args...);
  }
  template <typename WhereTo>
  void SeveralTimesTo(int time) {
    for (int i = 0; i != time; ++i) moveTo<WhereTo>();
  }
  void CheckCache(image_storage_test_t expected) {
    EXPECT_EQ(expected, Cache());
  }
  void CheckIndex(int expected) {
    EXPECT_EQ(expected, Index());
    EXPECT_EQ(expected, *image_cache_->ImageIterator());
  }
  void CheckBorders(int left, int right) {
    EXPECT_EQ(left, LeftEdge());
    EXPECT_EQ(right, RightEdge());
  }
};

using FirstForward = ImageChanging<10, 6, 6>;

TEST_F(FirstForward, ObjectSetup) {
  CheckCache({3, 4, 5, 6, 7, 8});
  CheckBorders(3, 8);
  CheckIndex(5);
}

TEST_F(FirstForward, StepOne) {
  // Since no image is initially displayed, the first call to 'moveTo' displays
  // the 'current' image. Subsequent calls to 'moveTo' cause a transition from
  // one image to another.
  moveTo<NextImage>();
  CheckCache({3, 4, 5, 6, 7, 8});
  CheckBorders(3, 8);
  CheckIndex(5);
}

TEST_F(FirstForward, StepTwo) {
  SeveralTimesTo<NextImage>(2);
  CheckCache({3, 4, 5, 6, 7, 8});
  CheckBorders(3, 8);
  CheckIndex(6);
}

TEST_F(FirstForward, StepThree) {
  SeveralTimesTo<NextImage>(3);
  CheckCache({4, 5, 6, 7, 8, 9});
  CheckBorders(4, 9);
  CheckIndex(7);
}

TEST_F(FirstForward, StepFour) {
  SeveralTimesTo<NextImage>(4);
  CheckCache({4, 5, 6, 7, 8, 9});
  CheckBorders(4, 9);
  CheckIndex(8);
}

using SecondForward = ImageChanging<10, 0, 5>;

TEST_F(SecondForward, ObjectSetup) {
  CheckCache({0, 1, 2, 3, 4});
  CheckBorders(0, 4);
  CheckIndex(0);
}

TEST_F(SecondForward, StepOne) {
  // Since no image is initially displayed, the first call to 'moveTo' displays
  // the 'current' image. Subsequent calls to 'moveTo' cause a transition from
  // one image to another.
  moveTo<NextImage>();
  CheckCache({0, 1, 2, 3, 4});
  CheckBorders(0, 4);
  CheckIndex(0);
}

TEST_F(SecondForward, StepTwo) {
  SeveralTimesTo<NextImage>(2);
  CheckCache({0, 1, 2, 3, 4});
  CheckBorders(0, 4);
  CheckIndex(1);
}

TEST_F(SecondForward, StepThree) {
  SeveralTimesTo<NextImage>(3);
  CheckCache({0, 1, 2, 3, 4});
  CheckBorders(0, 4);
  CheckIndex(2);
}

TEST_F(SecondForward, StepFour) {
  SeveralTimesTo<NextImage>(4);
  CheckCache({1, 2, 3, 4, 5});
  CheckBorders(1, 5);
  CheckIndex(3);
}

TEST_F(SecondForward, StepFive) {
  SeveralTimesTo<NextImage>(5);
  CheckCache({2, 3, 4, 5, 6});
  CheckBorders(2, 6);
  CheckIndex(4);
}

using ThirdForward = ImageChanging<6, 0, 10>;

TEST_F(ThirdForward, ObjectSetup) {
  CheckCache({0, 1, 2, 3, 4, 5});
  CheckBorders(0, 5);
  CheckIndex(0);
}

TEST_F(ThirdForward, StepOne) {
  // Since no image is initially displayed, the first call to 'moveTo' displays
  // the 'current' image. Subsequent calls to 'moveTo' cause a transition from
  // one image to another.
  moveTo<NextImage>();
  CheckCache({0, 1, 2, 3, 4, 5});
  CheckBorders(0, 5);
  CheckIndex(0);
}

TEST_F(ThirdForward, StepTwo) {
  SeveralTimesTo<NextImage>(2);
  CheckCache({0, 1, 2, 3, 4, 5});
  CheckBorders(0, 5);
  CheckIndex(1);
}

TEST_F(ThirdForward, StepThree) {
  SeveralTimesTo<NextImage>(3);
  CheckCache({0, 1, 2, 3, 4, 5});
  CheckBorders(0, 5);
  CheckIndex(2);
}

TEST_F(ThirdForward, StepFour) {
  SeveralTimesTo<NextImage>(4);
  CheckCache({0, 1, 2, 3, 4, 5});
  CheckBorders(0, 5);
  CheckIndex(3);
}

TEST_F(ThirdForward, StepFive) {
  SeveralTimesTo<NextImage>(5);
  CheckCache({0, 1, 2, 3, 4, 5});
  CheckBorders(0, 5);
  CheckIndex(4);
}

TEST_F(ThirdForward, StepSix) {
  SeveralTimesTo<NextImage>(6);
  CheckCache({0, 1, 2, 3, 4, 5});
  CheckBorders(0, 5);
  CheckIndex(5);
}

TEST_F(ThirdForward, StepSeven) {
  SeveralTimesTo<NextImage>(7);
  CheckCache({0, 1, 2, 3, 4, 5});
  CheckBorders(0, 5);
  CheckIndex(5);
}

using FourthForward = ImageChanging<15, 4, 10>;

TEST_F(FourthForward, ObjectSetup) {
  CheckCache({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  CheckBorders(0, 9);
  CheckIndex(3);
}

TEST_F(FourthForward, StepOne) {
  // Since no image is initially displayed, the first call to 'moveTo' displays
  // the 'current' image. Subsequent calls to 'moveTo' cause a transition from
  // one image to another.
  moveTo<NextImage>();
  CheckCache({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  CheckBorders(0, 9);
  CheckIndex(3);
}

TEST_F(FourthForward, StepTwo) {
  SeveralTimesTo<NextImage>(2);
  CheckCache({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  CheckBorders(0, 9);
  CheckIndex(4);
}

TEST_F(FourthForward, StepThree) {
  SeveralTimesTo<NextImage>(3);
  CheckCache({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  CheckBorders(0, 9);
  CheckIndex(5);
}

TEST_F(FourthForward, StepFour) {
  SeveralTimesTo<NextImage>(4);
  CheckCache({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  CheckBorders(1, 10);
  CheckIndex(6);
}

TEST_F(FourthForward, StepFifth) {
  SeveralTimesTo<NextImage>(5);
  CheckCache({2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
  CheckBorders(2, 11);
  CheckIndex(7);
}

using FirstBackward = ImageChanging<10, 5, 6>;

TEST_F(FirstBackward, ObjectSetup) {
  CheckCache({2, 3, 4, 5, 6, 7});
  CheckBorders(2, 7);
  CheckIndex(4);
}

TEST_F(FirstBackward, StepOne) {
  // Since no image is initially displayed, the first call to 'moveTo' displays
  // the 'current' image. Subsequent calls to 'moveTo' cause a transition from
  // one image to another.
  moveTo<PreviousImage>();
  CheckCache({2, 3, 4, 5, 6, 7});
  CheckBorders(2, 7);
  CheckIndex(4);
}

TEST_F(FirstBackward, StepTwo) {
  SeveralTimesTo<PreviousImage>(2);
  CheckCache({1, 2, 3, 4, 5, 6});
  CheckBorders(1, 6);
  CheckIndex(3);
}

TEST_F(FirstBackward, StepThree) {
  SeveralTimesTo<PreviousImage>(3);
  CheckCache({0, 1, 2, 3, 4, 5});
  CheckBorders(0, 5);
  CheckIndex(2);
}

TEST_F(FirstBackward, StepFour) {
  SeveralTimesTo<PreviousImage>(4);
  CheckCache({0, 1, 2, 3, 4, 5});
  CheckBorders(0, 5);
  CheckIndex(1);
}

using SecondBackward = ImageChanging<10, 10, 5>;

TEST_F(SecondBackward, ObjectSetup) {
  CheckCache({5, 6, 7, 8, 9});
  CheckBorders(5, 9);
  CheckIndex(9);
}

TEST_F(SecondBackward, StepOne) {
  // Since no image is initially displayed, the first call to 'moveTo' displays
  // the 'current' image. Subsequent calls to 'moveTo' cause a transition from
  // one image to another.
  moveTo<PreviousImage>();
  CheckCache({5, 6, 7, 8, 9});
  CheckBorders(5, 9);
  CheckIndex(9);
}

TEST_F(SecondBackward, StepTwo) {
  SeveralTimesTo<PreviousImage>(2);
  CheckCache({5, 6, 7, 8, 9});
  CheckBorders(5, 9);
  CheckIndex(8);
}

TEST_F(SecondBackward, StepThree) {
  SeveralTimesTo<PreviousImage>(3);
  CheckCache({5, 6, 7, 8, 9});
  CheckBorders(5, 9);
  CheckIndex(7);
}

TEST_F(SecondBackward, StepFour) {
  SeveralTimesTo<PreviousImage>(4);
  CheckCache({4, 5, 6, 7, 8});
  CheckBorders(4, 8);
  CheckIndex(6);
}

using ThirdBackward = ImageChanging<15, 8, 10>;
// images_number, index, cache_capacity

TEST_F(ThirdBackward, ObjectSetup) {
  CheckCache({3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  CheckBorders(3, 12);
  CheckIndex(7);
}

TEST_F(ThirdBackward, StepOne) {
  // Since no image is initially displayed, the first call to 'moveTo' displays
  // the 'current' image. Subsequent calls to 'moveTo' cause a transition from
  // one image to another.
  moveTo<PreviousImage>();
  CheckCache({3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  CheckBorders(3, 12);
  CheckIndex(7);
}

TEST_F(ThirdBackward, StepTwo) {
  SeveralTimesTo<PreviousImage>(2);
  CheckCache({2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
  CheckBorders(2, 11);
  CheckIndex(6);
}

TEST_F(ThirdBackward, StepThree) {
  SeveralTimesTo<PreviousImage>(3);
  CheckCache({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  CheckBorders(1, 10);
  CheckIndex(5);
}

TEST_F(ThirdBackward, StepFour) {
  SeveralTimesTo<PreviousImage>(4);
  CheckCache({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  CheckBorders(0, 9);
  CheckIndex(4);
}