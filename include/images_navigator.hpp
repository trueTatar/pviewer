#pragma once

#include "abstract_image_cache.hpp"
#include "lists.hpp"
#include "messagebox.hpp"

using folder_info_t = std::shared_ptr<FolderPath>;

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
  int moveTo(Args... args) {
    if constexpr (std::is_same_v<WhereTo, NextImage> ||
                  std::is_same_v<WhereTo, PreviousImage>) {
      return WhereTo(*file_info_, *cache_, is_null_image_).Result();
    } else if constexpr (std::is_constructible_v<WhereTo, folder_info_t>) {
      WhereTo w(folders_);
    } else if constexpr (std::is_constructible_v<WhereTo, file_info_t&,
                                                 cached_images_t&, Args...>) {
      WhereTo w(*file_info_, *cache_, args...);
    }
    return 0;
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
      : result_(0) {
    int direction = Order::step;
    if (location.ImageHideNeeded(direction)) {
      result_ += direction > 0 ? 1000 : 2000;
      cache.HideImage();
      return;
    }
    if (location.DisplayCachedImageNeeded(direction) && is_null_image()) {
      result_ += direction > 0 ? 100 : 200;
      cache.DisplayImage();
      return;
    }
    Abstract::TaskQueue<In, Im>& task_queue = location.TaskQueueObject();
    location.MoveIndex(direction);
    if (int shift = cache.CheckCacheThreshold(direction); shift) {
      auto iter = location.Index();
      std::advance(iter, shift);
      if (location.Begin() <= iter && iter < location.End()) {
        result_ += 10;
        direction > 0 ? task_queue.template Push<Back>(iter)
                      : task_queue.template Push<Front>(iter);
        cache.ProcessTaskItem(task_queue);
        cache.RemoveOutdated(direction);
      }
    }
    result_ += direction > 0 ? 1 : 2;
    cache.DisplayImage();
  }
  int Result() { return result_; }

 private:
  int result_;
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
