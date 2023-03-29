#pragma once

#include <QList>
#include <QPixmap>
#include <QString>

#include "abstract_task_queue.hpp"

class TaskQueue : public Abstract::TaskQueue<QString, QPixmap> {
 public:
  TaskQueue(const int initial_task_size)
      : Abstract::TaskQueue<QString, QPixmap>(initial_task_size) {}
  void PushBack(info_t value) override { queue_.push_back(value); }
  info_t PopFront() override {
    info_t value = queue_.front();
    queue_.pop_front();
    return value;
  }
  bool Empty() const override { return queue_.empty(); }

 private:
  QList<info_t> queue_;
};