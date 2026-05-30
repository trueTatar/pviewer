#pragma once

#include <QString>
#include <QVector>
#include <utility>

struct ImageEntry {
  QString path;
  bool enabled = true;
};

class ImageComparisonModel {
 public:
  void SetImages(QVector<QString> paths) {
    entries_.clear();
    entries_.reserve(paths.size());
    for (QString& path : paths) {
      entries_.push_back(ImageEntry{std::move(path), true});
    }
  }

  void SetEntries(QVector<ImageEntry> entries) { entries_ = std::move(entries); }

  const QVector<ImageEntry>& Entries() const { return entries_; }

  QVector<QString> EnabledPaths() const {
    QVector<QString> paths;
    for (const ImageEntry& entry : entries_) {
      if (entry.enabled) paths.push_back(entry.path);
    }
    return paths;
  }

  bool SetEnabled(int row, bool enabled) {
    if (!IsValidRow(row)) return false;
    entries_[row].enabled = enabled;
    return true;
  }

  bool Move(int from, int to) {
    if (!IsValidRow(from) || to < 0 || to >= entries_.size()) return false;
    if (from == to) return true;

    ImageEntry entry = entries_.takeAt(from);
    entries_.insert(to, std::move(entry));
    return true;
  }

  int EnabledPositionFor(QString const& path) const {
    if (entries_.isEmpty()) return 0;

    const int row = RowOf(path);
    if (row >= 0 && entries_[row].enabled) {
      return EnabledPositionForRow(row);
    }

    if (row < 0) return 0;

    const int anchor = row;
    for (int distance = 0; distance < entries_.size(); ++distance) {
      const int right = anchor + distance;
      if (right < entries_.size() && entries_[right].enabled) {
        return EnabledPositionForRow(right);
      }

      const int left = anchor - distance;
      if (left >= 0 && entries_[left].enabled) {
        return EnabledPositionForRow(left);
      }
    }
    return 0;
  }

 private:
  bool IsValidRow(int row) const { return 0 <= row && row < entries_.size(); }

  int RowOf(QString const& path) const {
    for (int i = 0; i < entries_.size(); ++i) {
      if (entries_[i].path == path) return i;
    }
    return -1;
  }

  int EnabledPositionForRow(int row) const {
    int position = 0;
    for (int i = 0; i <= row; ++i) {
      if (entries_[i].enabled) ++position;
    }
    return position;
  }

  QVector<ImageEntry> entries_;
};
