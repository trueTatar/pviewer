#pragma once

#include "lists.h"
#include "abstract_cached_images_list.hpp"
#include "messagebox.h"

using folder_info_t = std::shared_ptr<FolderPath>;

template<typename Order, typename In, typename Im>
class ImageBase;

template<typename In, typename Im>
class Move {
  using file_info_t = Abstract::ImageLocation<In, Im>;
  using cached_images_t = Abstract::ImageCache<In, Im>;
  using task_queue_t = Abstract::TaskQueue<In, Im>;
  using NextImage = ImageBase<NumericalOrder, In, Im>;
  using PreviousImage = ImageBase<ReverseOrder, In, Im>;

  using pointer_t = void(QList<Im>::*)(const Im&);
  static constexpr pointer_t Back = &QList<Im>::push_back;
  static constexpr pointer_t Front = &QList<Im>::push_front;
public:
  Move(std::shared_ptr<file_info_t> i,
    std::shared_ptr<cached_images_t> c,
    folder_info_t f,
    std::function<bool()> is_null_image)
    : is_null_image_(is_null_image)
    , file_info_(i)
    , cache_(c)
    , folders_(f)
    , task_queue_(file_info_->TaskQueue()) { }
  template<typename WhereTo, typename... Args>
  int moveTo(Args... args) {
    if constexpr (std::is_same_v<WhereTo, NextImage> || std::is_same_v<WhereTo, PreviousImage>) {
      return WhereTo(*file_info_, *cache_, is_null_image_).Result();
    } else if constexpr (std::is_constructible_v<WhereTo, folder_info_t>) {
      WhereTo w(folders_);
    } else if constexpr (std::is_constructible_v<WhereTo, file_info_t&, cached_images_t&, Args...>) {
      WhereTo w(*file_info_, *cache_, args...);
    }
    return 0;
  }
private:
  std::function<bool()> is_null_image_;
  std::shared_ptr<file_info_t> file_info_;
  std::shared_ptr<cached_images_t> cache_;
  std::shared_ptr<FolderPath> folders_;
  task_queue_t& task_queue_;
};

template<typename In, typename Im>
struct EndOfTheListImpl {
  EndOfTheListImpl(Abstract::ImageLocation<In, Im>& images, Abstract::ImageCache<In, Im>& cache) {
    cache.Reset();
    images.setIndex(images.size());
  }
};

template<typename In, typename Im>
struct BeginOfTheListImpl {
  BeginOfTheListImpl(Abstract::ImageLocation<In, Im>& images, Abstract::ImageCache<In, Im>& cache) {
    cache.Reset();
    images.setIndex(-1);
  }
};

template<typename In, typename Im>
struct ImageNumberImpl {
  ImageNumberImpl(Abstract::ImageLocation<In, Im>& images, Abstract::ImageCache<In, Im>& cache, int number) {
    if (number < 1) {
      BeginOfTheListImpl(images, cache);
    } else if (number > images.size()) {
      EndOfTheListImpl(images, cache);
    } else {
      cache.Reset();
      images.setIndex(number - 1);
    }
  }
};

template<typename Order, typename In, typename Im>
class ImageBase {
public:
  ImageBase(Abstract::ImageLocation<In, Im>& images, Abstract::ImageCache<In, Im>& cache,
    std::function<bool()> is_null_image): result_(-1) {
    SingleThreaded(images, cache, is_null_image);
  }
  void SingleThreaded(Abstract::ImageLocation<In, Im>& images, Abstract::ImageCache<In, Im>& cache,
    std::function<bool()> is_null_image) {
    if (images.isEmpty()) {
      return;
    }
    if (!cache.isEmpty() && images.template initialState<Order>()) {
      images.MoveIndex(Order::step);
      cache.DisplayImage();
      result_ = Order::ExitCode::SetImageFromCache;
    } else if (images.template lastIsPointed<Order>() || images.template pastTheLastIsPointed<Order>()) {
      images.MoveIndex(Order::step);
      cache.HideImage();
      result_ = Order::ExitCode::NoAction;
    } else if (cache.isFilled() && cache.template lastIsPointed<Order>()) {
      images.MoveIndex(Order::step);
      cache.RemoveOutdatedImage();
      cache.CacheImage();
      cache.DisplayImage();
      result_ = Order::ExitCode::UpdateCache;
    } else if (!cache.isEmpty() && !cache.template lastIsPointed<Order>()) {
      cache.MoveIndex(Order::step);
      images.MoveIndex(Order::step);
      cache.DisplayImage();
      result_ = Order::ExitCode::GoThroughCache;
    } else {
      cache.MoveIndex(Order::step);
      images.MoveIndex(Order::step, is_null_image());
      if (cache.isEmpty())
        cache.setIndex(0);
      cache.CacheImage();
      cache.DisplayImage();
      result_ = Order::ExitCode::FillCache;  // returns 1
    }
  }
  // This is a refactored version
  /*
  void ChangeImage(int direction, PathBase<In, Im>& file_info,
                   AbstractCachedImagesList<In, Im>& image_cache,
                   AbstractTaskQueue<In, Im>& task_queue, std::function<bool()> is_null) {
    using pointer_t = void(QList<Im>::*)(const Im&);
    static constexpr pointer_t Back = &QList<Im>::push_back;
    static constexpr pointer_t Front = &QList<Im>::push_front;
    file_info.MoveIndex(direction, is_null());
    image_cache.MoveIndex(direction);
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
      image_cache.DisplayImage();
    }
  }
  */
  int Result() {
    return result_;
  }
private:
  int result_;
};

template<typename Impl>
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
  static QString errorMessage() {
    return "there is no next folder";
  }
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
  static QString errorMessage() {
    return "there is no previous folder";
  }
};

using NextFolder = FolderBase<NextFolderImpl>;
using PreviousFolder = FolderBase<PreviousFolderImpl>;
