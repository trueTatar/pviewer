#include "images_list_panel.hpp"

#include <QAbstractItemModel>
#include <QBrush>
#include <QColor>
#include <QTimer>
#include <QVBoxLayout>

#include <utility>

ImagesListPanel::ImagesListPanel(QWidget* parent)
    : QDialog(parent), list_(new QListWidget(this)) {
  setWindowTitle("Images");
  resize(520, 360);

  list_->setDragDropMode(QAbstractItemView::InternalMove);
  list_->setDefaultDropAction(Qt::MoveAction);
  list_->setDragDropOverwriteMode(false);
  list_->setDropIndicatorShown(true);
  list_->setSelectionMode(QAbstractItemView::SingleSelection);
  list_->setTextElideMode(Qt::ElideMiddle);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->addWidget(list_);

  connect(list_, &QListWidget::itemChanged, this, [this](QListWidgetItem*) {
    if (updating_) return;
    SyncEntriesFromList();
    EmitEntriesChanged();
  });

  connect(list_, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem* item) {
            if (!item || item->checkState() != Qt::Checked) return;
            emit imageActivated(item->data(Qt::UserRole).toString());
          });

  connect(list_->model(), &QAbstractItemModel::rowsMoved, this,
          [this] { ScheduleEntriesSync(); });
  connect(list_->model(), &QAbstractItemModel::rowsInserted, this,
          [this] { ScheduleEntriesSync(); });
  connect(list_->model(), &QAbstractItemModel::rowsRemoved, this,
          [this] { ScheduleEntriesSync(); });
  connect(list_->model(), &QAbstractItemModel::layoutChanged, this,
          [this] { ScheduleEntriesSync(); });
}

void ImagesListPanel::SetEntries(QVector<ImageEntry> entries) {
  entries_ = std::move(entries);
  RebuildList();
}

void ImagesListPanel::SetCurrentPath(QString path) {
  if (current_path_ == path) return;
  current_path_ = std::move(path);
  RebuildList();
}

void ImagesListPanel::RebuildList() {
  updating_ = true;
  list_->clear();
  for (const ImageEntry& entry : entries_) {
    const bool is_current = entry.path == current_path_;
    auto* item = new QListWidgetItem(
        is_current ? entry.path + QStringLiteral("  (Current)") : entry.path);
    item->setToolTip(entry.path);
    item->setData(Qt::UserRole, entry.path);
    item->setCheckState(entry.enabled ? Qt::Checked : Qt::Unchecked);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                   Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled);
    if (is_current) {
      item->setForeground(QBrush(QColor(140, 140, 140)));
    }
    list_->addItem(item);
  }
  updating_ = false;
}

void ImagesListPanel::SyncEntriesFromList() {
  QVector<ImageEntry> entries;
  entries.reserve(list_->count());
  for (int i = 0; i < list_->count(); ++i) {
    QListWidgetItem* item = list_->item(i);
    entries.push_back(ImageEntry{
        item->data(Qt::UserRole).toString(),
        item->checkState() == Qt::Checked,
    });
  }
  entries_ = std::move(entries);
}

void ImagesListPanel::ScheduleEntriesSync() {
  if (updating_ || sync_scheduled_) return;
  sync_scheduled_ = true;
  QTimer::singleShot(0, this, [this] {
    sync_scheduled_ = false;
    if (updating_) return;
    SyncEntriesFromList();
    EmitEntriesChanged();
  });
}

void ImagesListPanel::EmitEntriesChanged() {
  emit entriesChanged(entries_);
}
