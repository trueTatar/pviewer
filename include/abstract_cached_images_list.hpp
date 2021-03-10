#pragma once

#include <QObject>

#include "order.hpp"

#include "task_queue.hpp"

namespace Abstract {

/*
 * The Abstract::ImageCache class is an abstract class, that provides a base implementation
 * for classes that hold and modify preloaded images in RAM.
 */

  template<typename In, typename Im>
  class ImageCache: public QObject {
  protected:
    using info_t = typename QVector<In>::iterator;
    using pointer_t = void(QList<Im>::*)(const Im&);
    using queue_t = Abstract::TaskQueue<In, Im>;
  public:
    ImageCache(info_t const& image, std::size_t capacity)
      : pos_(-1)
      , capacity_(capacity)
      , current_image_in_cache_(image) { }
    void FormCache(queue_t& queue) {
      while (!queue.Empty()) {
        HandleCacheTask(queue);
      }
    }
    int AddQuery() {
      constexpr int dist = 2;
      int a = std::distance(info_cached_left_, current_image_in_cache_);
      int b = std::distance(current_image_in_cache_, info_cached_right_);
      if (a < dist) {
        return -dist;
      } else if (b < dist) {
        return dist;
      }
      return 0;
    }
    void HandleCacheTask(queue_t& queue) {
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
    In const& ImagePath() {
      return *current_image_in_cache_;
    }
    info_t LeftEdge() const {
      return info_cached_left_;
    }
    info_t RightEdge() const {
      return info_cached_right_;
    }
    void RemoveOutdated(int direction) {
      if (Size() > capacity_) {
        direction > 0 ? PopFront() : PopBack();
        direction > 0 ? ++info_cached_left_ : --info_cached_right_;
      }
    }
    virtual void PopFront() = 0;
    virtual void PopBack() = 0;
    virtual std::size_t Size() const = 0;
    virtual void Push(pointer_t, info_t) = 0;


    virtual void CacheImage() = 0;
    virtual void DisplayImage() = 0;
    virtual void HideImage() = 0;
    virtual void UpdateCurrentScaledImage() = 0;
    virtual int RemoveOutdatedImage() = 0;
    virtual bool isEmpty() const = 0;
    bool isFilled() const;
    void Reset();
    int index() const;
    void setIndex(int value);
    void MoveIndex(int step);
    template<typename Order> bool lastIsPointed();
  protected:
    int OutdatedImagePos() const;
  private:
    virtual void Clear() = 0;
    int pos_;

  private:
    std::size_t capacity_;
    info_t const& current_image_in_cache_; // pointer to displayed image
    info_t info_cached_left_;  // pointer to most leftward image now cached
    info_t info_cached_right_;  // pointer to most rightward image now cached
  };

  template<typename In, typename Im>
  inline int ImageCache<In, Im>::OutdatedImagePos() const {
    return capacity_ - (pos_ + 1);
  }

  template<typename In, typename Im>
  inline bool ImageCache<In, Im>::isFilled() const {
    return Size() == capacity_;
  }

  template<typename In, typename Im>
  inline void ImageCache<In, Im>::Reset() {
    Clear();
    pos_ = 0;
  }

  template<typename In, typename Im>
  inline int ImageCache<In, Im>::index() const {
    return pos_;
  }

  template<typename In, typename Im>
  inline void ImageCache<In, Im>::setIndex(int value) {
    pos_ = value;
  }

  template<typename In, typename Im>
  inline void ImageCache<In, Im>::MoveIndex(int step) {
    bool increment = pos_ < static_cast<int>(capacity_) - 1;
    bool decrement = pos_ > 0;
    if ((step == 1 && increment) || (step == -1 && decrement))
      pos_ = pos_ + step;
  }

  template<typename In, typename Im>
  template<typename Order>
  bool ImageCache<In, Im>::lastIsPointed() {
    if constexpr (std::is_same_v<NumericalOrder, Order>)
      return pos_ == static_cast<int>(Size()) - 1;
    else if constexpr (std::is_same_v<ReverseOrder, Order>)
      return pos_ == 0;
  }

}