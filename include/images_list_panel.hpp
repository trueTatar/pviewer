#pragma once

#include <QDialog>
#include <QListWidget>

#include "image_comparison_model.hpp"

class ImagesListPanel : public QDialog {
  Q_OBJECT
 public:
  explicit ImagesListPanel(QWidget* parent = nullptr);

  void SetEntries(QVector<ImageEntry> entries);
  void SetCurrentPath(QString path);
  QVector<ImageEntry> Entries() const { return entries_; }

 signals:
  void entriesChanged(QVector<ImageEntry> entries);
  void imageActivated(QString path);

 private:
  void RebuildList();
  void SyncEntriesFromList();
  void ScheduleEntriesSync();
  void EmitEntriesChanged();

  QListWidget* list_;
  QVector<ImageEntry> entries_;
  QString current_path_;
  bool updating_ = false;
  bool sync_scheduled_ = false;
};
