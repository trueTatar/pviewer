#pragma once

#include <ostream>

#include "abstract_image_cache.hpp"
#include "lists.hpp"
#include "messagebox.hpp"

using folder_info_t = std::shared_ptr<FolderPath>;

// Outcome of a single navigation step, as a set of flags. This is consumed
// only by the tests (production ignores moveTo's return value); it replaces the
// former additive magic integers so assertions and failures read in plain
// terms, e.g. Enqueue|Move|Forward instead of 11.
enum class Step : unsigned {
  None = 0,
  Forward = 1u << 0,
  Backward = 1u << 1,
  Move = 1u << 2,        // advanced the index and displayed the new image
  ShowCached = 1u << 3,  // displayed the already-cached current image
  Enqueue = 1u << 4,     // pushed a new image into the cache task queue
  HideAtEdge = 1u << 5,  // hid the image when stepping past a list edge
};

constexpr Step operator|(Step a, Step b) {
  return static_cast<Step>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}
constexpr Step& operator|=(Step& a, Step b) { return a = a | b; }

inline std::ostream& operator<<(std::ostream& os, Step step) {
  if (step == Step::None) return os << "None";
  const unsigned bits = static_cast<unsigned>(step);
  const char* separator = "";
  auto append = [&](Step flag, const char* name) {
    if (bits & static_cast<unsigned>(flag)) {
      os << separator << name;
      separator = "|";
    }
  };
  append(Step::Forward, "Forward");
  append(Step::Backward, "Backward");
  append(Step::Move, "Move");
  append(Step::ShowCached, "ShowCached");
  append(Step::Enqueue, "Enqueue");
  append(Step::HideAtEdge, "HideAtEdge");
  return os;
}

template <typename Order, typename In, typename Im>
class ImageBase;

template <typename In, typename Im>
class ImagesNavigator {
  using file_info_t = Abstract::ImageLocation<In, Im>;
  using cached_images_t = Abstract::ImageCache<In, Im>;
  using task_queue_t = Abstract::TaskQueue<In, Im>;
  using NextImage = ImageBase<NumericalOrder, In, Im>;
  using PreviousImage = ImageBase<ReverseOrder, In, Im>;

  using pointer_t = void (QList<Im>::*)(const Im&);
  static constexpr pointer_t Back = &QList<Im>::push_back;
  static constexpr pointer_t Front = &QList<Im>::push_front;

 public:
  ImagesNavigator(std::shared_ptr<file_info_t> i,
                 std::shared_ptr<cached_images_t> c, folder_info_t f,
                 std::function<bool()> is_null_image)
      : is_null_image_(is_null_image), file_info_(i), cache_(c), folders_(f) {}
  template <typename WhereTo, typename... Args>
  Step moveTo(Args... args) {
    if constexpr (std::is_same_v<WhereTo, NextImage> ||
                  std::is_same_v<WhereTo, PreviousImage>) {
      return WhereTo(*file_info_, *cache_, is_null_image_).Result();
    } else if constexpr (std::is_constructible_v<WhereTo, folder_info_t>) {
      WhereTo w(folders_);
    } else if constexpr (std::is_constructible_v<WhereTo, file_info_t&,
                                                 cached_images_t&, Args...>) {
      WhereTo w(*file_info_, *cache_, args...);
    }
    return Step::None;
  }

 private:
  std::function<bool()> is_null_image_;
  std::shared_ptr<file_info_t> file_info_;
  std::shared_ptr<cached_images_t> cache_;
  std::shared_ptr<FolderPath> folders_;
};

template <typename In, typename Im>
struct ImageNumberImpl {
  ImageNumberImpl() = default;
  ImageNumberImpl(Abstract::ImageLocation<In, Im>& images,
                  Abstract::ImageCache<In, Im>& cache, int position) {
    LimitValue(--position, images.size() - 1);
    Process(images, cache, position);
  }

 protected:
  static void Process(Abstract::ImageLocation<In, Im>& images,
                      Abstract::ImageCache<In, Im>& cache, int position) {
    cache.Clear();
    images.setIndex(position);
    Abstract::TaskQueue<In, Im>& task_queue = images.TaskQueueObject();
    Abstract::ImageLocation<In, Im>::InitialImageTask(images, task_queue);
    cache.ProcessTaskQueue(task_queue);
  }

 private:
  void LimitValue(int& position, int max) {
    if (position < 0)
      position = 0;
    else if (position > max)
      position = max;
  }
};

template <typename In, typename Im>
struct EndOfTheListImpl : public ImageNumberImpl<In, Im> {
  EndOfTheListImpl(Abstract::ImageLocation<In, Im>& images,
                   Abstract::ImageCache<In, Im>& cache) {
    ImageNumberImpl<In, Im>::Process(images, cache, images.size() - 1);
  }
};

template <typename In, typename Im>
struct BeginOfTheListImpl : public ImageNumberImpl<In, Im> {
  BeginOfTheListImpl(Abstract::ImageLocation<In, Im>& images,
                     Abstract::ImageCache<In, Im>& cache) {
    ImageNumberImpl<In, Im>::Process(images, cache, 0);
  }
};

template <typename Order, typename In, typename Im>
class ImageBase {
 private:
  using pointer_t = void (QList<Im>::*)(const Im&);
  static constexpr pointer_t Back = &QList<Im>::push_back;
  static constexpr pointer_t Front = &QList<Im>::push_front;

 public:
  ImageBase(Abstract::ImageLocation<In, Im>& location,
            Abstract::ImageCache<In, Im>& cache,
            std::function<bool()> is_null_image)
      : result_(Step::None) {
    int direction = Order::step;
    Step dir = direction > 0 ? Step::Forward : Step::Backward;
    if (location.ImageHideNeeded(direction)) {
      result_ = Step::HideAtEdge | dir;
      cache.HideImage();
      return;
    }
    if (location.DisplayCachedImageNeeded(direction) && is_null_image()) {
      result_ = Step::ShowCached | dir;
      cache.DisplayImage();
      return;
    }
    Abstract::TaskQueue<In, Im>& task_queue = location.TaskQueueObject();
    location.MoveIndex(direction);
    if (int shift = cache.CheckCacheThreshold(direction); shift) {
      auto iter = location.Index();
      std::advance(iter, shift);
      if (location.Begin() <= iter && iter < location.End()) {
        result_ |= Step::Enqueue;
        direction > 0 ? task_queue.template Push<Back>(iter)
                      : task_queue.template Push<Front>(iter);
        cache.ProcessTaskItem(task_queue);
        cache.RemoveOutdated(direction);
      }
    }
    result_ |= Step::Move | dir;
    cache.DisplayImage();
  }
  Step Result() { return result_; }

 private:
  Step result_;
};

template <typename Impl>
class FolderBase {
 public:
  FolderBase(std::shared_ptr<FolderPath> folders) {
    if (Impl::moveIndex(folders)) {
      QString item = folders->pathByIndex();
      MessageBox::inform(item, 600);
      folders->sendNewFolder();
    } else {
      QString message = Impl::errorMessage();
      MessageBox::inform(std::move(message), 1500);
    }
  }
};

struct NextFolderImpl {
 public:
  static bool moveIndex(folder_info_t folders) {
    if (!folders->isEmpty() && !folders->lastIsPointed<NumericalOrder>()) {
      folders->MoveIndex(1);
      return true;
    }
    return false;
  }
  static QString errorMessage() { return "there is no next folder"; }
};

class PreviousFolderImpl {
 public:
  static bool moveIndex(folder_info_t folders) {
    if (!folders->isEmpty() && folders->lastIsPointed<ReverseOrder>()) {
      folders->MoveIndex(-1);
      return true;
    }
    return false;
  }
  static QString errorMessage() { return "there is no previous folder"; }
};

using NextFolder = FolderBase<NextFolderImpl>;
using PreviousFolder = FolderBase<PreviousFolderImpl>;
