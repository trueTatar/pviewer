#pragma once

#include <QObject>

#include "task_queue.hpp"

struct NumericalOrder {
  static constexpr int step = 1;
};

struct ReverseOrder {
  static constexpr int step = -1;
};

namespace Abstract {

/*
 * The Abstract::ImageCache class is an abstract class, that provides a base
 * implementation for classes that hold and modify preloaded images in RAM.
 */

template <typename In, typename Im>
class ImageCache : public QObject {
 protected:
  using info_t = typename QVector<In>::iterator;
  using pointer_t = void (QList<Im>::*)(const Im&);
  using queue_t = Abstract::TaskQueue<In, Im>;

 public:
  ImageCache(info_t const& image, std::size_t capacity);
  // The ProcessTaskQueue function is responsible for processing all existing
  // task items in the task queue.
  void ProcessTaskQueue(queue_t& queue);
  // The ProcessTaskItem function is responsible for processing a single task
  // item from the task queue.
  void ProcessTaskItem(queue_t& queue);
  // The CheckCacheThreshold function is responsible for determining whether a
  // new image should be added to the cache or not, based on the distance from
  // the current image iterator to the left and right borders of the cached
  // images.
  int CheckCacheThreshold(int move_direction) {
    int left_distance =
        std::distance(info_cached_left_, current_image_iterator_);
    int right_distance =
        std::distance(current_image_iterator_, info_cached_right_);
    if (left_distance < distance_threshold_ && move_direction < 0) {
      return -distance_threshold_;
    } else if (right_distance < distance_threshold_ && move_direction > 0) {
      return distance_threshold_;
    }
    return 0;
  }
  // The ImageIterator function returns an iterator that allows you to retrieve
  // the location of images (stored on a hard drive) that are cached and
  // currently being displayed on the screen.
  info_t ImageIterator() const { return current_image_iterator_; }
  void RemoveOutdated(int direction) {
    if (Size() > capacity_) {
      direction > 0 ? PopFront() : PopBack();
      direction > 0 ? ++info_cached_left_ : --info_cached_right_;
    }
  }
  virtual void PopFront() = 0;
  virtual void PopBack() = 0;
  virtual void Push(pointer_t, info_t) = 0;

  virtual void DisplayImage() = 0;
  virtual void HideImage() = 0;
  virtual void UpdateCurrentScaledImage() = 0;
  virtual bool isEmpty() const = 0;

  virtual void Clear() = 0;

  int index() const;

  // LeftEdge functions is for unit-tests
  info_t LeftEdge() const { return info_cached_left_; }
  // RightEdge functions is for unit-tests
  info_t RightEdge() const { return info_cached_right_; }

 protected:
  const QString& CurrentImageLocation() { return *current_image_iterator_; }

 private:
  static int CacheThreshold(int capacity) {
    int result = (capacity % 2 == 0) ? (capacity - 1) / 2 : capacity / 2;
    return result > 0 ? result : 1;
  }
  virtual std::size_t Size() const = 0;
  std::size_t capacity_;
  const int distance_threshold_;
  info_t const& current_image_iterator_;  // pointer to displayed image
  info_t info_cached_left_;   // pointer to most leftward image now cached
  info_t info_cached_right_;  // pointer to most rightward image now cached
};

template <typename In, typename Im>
inline ImageCache<In, Im>::ImageCache(info_t const& image,
                                      const std::size_t capacity)
    : capacity_(capacity),
      distance_threshold_(CacheThreshold(capacity)),
      current_image_iterator_(image) {}

template <typename In, typename Im>
inline void ImageCache<In, Im>::ProcessTaskQueue(queue_t& queue) {
  while (!queue.Empty()) {
    ProcessTaskItem(queue);
  }
}

template <typename In, typename Im>
inline void ImageCache<In, Im>::ProcessTaskItem(queue_t& queue) {
  constexpr pointer_t push_back_op = &QList<Im>::push_back;
  constexpr pointer_t push_front_op = &QList<Im>::push_front;
  auto [value, op] = queue.Next();
  if (op == nullptr) {
    std::tie(info_cached_left_, info_cached_right_) = std::tie(value, value);
    return Push(push_back_op, value);
  } else if (push_back_op == op)
    info_cached_right_ = value;
  else if (push_front_op == op)
    info_cached_left_ = value;
  Push(op, value);
}

template <typename In, typename Im>
inline int ImageCache<In, Im>::index() const {
  return std::distance(info_cached_left_, current_image_iterator_);
}

}  // namespace Abstract