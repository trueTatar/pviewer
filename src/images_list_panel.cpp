#include "images_list_panel.hpp"

#include <QAbstractItemModel>
#include <QBrush>
#include <QColor>
#include <QShortcut>
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
  auto* delete_shortcut = new QShortcut(QKeySequence::Delete, list_);
  connect(delete_shortcut, &QShortcut::activated, this, [this] {
    QListWidgetItem* item = list_->currentItem();
    if (item) emit imageDeleteRequested(item->data(Qt::UserRole).toString());
  });
  auto* previous_shortcut =
      new QShortcut(QKeySequence(QStringLiteral("Ctrl+Left")), this);
  connect(previous_shortcut, &QShortcut::activated, this,
          &ImagesListPanel::previousImageRequested);
  auto* next_shortcut =
      new QShortcut(QKeySequence(QStringLiteral("Ctrl+Right")), this);
  connect(next_shortcut, &QShortcut::activated, this,
          &ImagesListPanel::nextImageRequested);
  auto* move_up_shortcut =
      new QShortcut(QKeySequence(QStringLiteral("Alt+Up")), this);
  connect(move_up_shortcut, &QShortcut::activated, this,
          [this] { MoveSelectedRow(-1); });
  auto* move_down_shortcut =
      new QShortcut(QKeySequence(QStringLiteral("Alt+Down")), this);
  connect(move_down_shortcut, &QShortcut::activated, this,
          [this] { MoveSelectedRow(1); });
  auto* zoom_in_shortcut =
      new QShortcut(QKeySequence(QStringLiteral("+")), this);
  connect(zoom_in_shortcut, &QShortcut::activated, this,
          &ImagesListPanel::zoomInRequested);
  auto* zoom_out_shortcut =
      new QShortcut(QKeySequence(QStringLiteral("-")), this);
  connect(zoom_out_shortcut, &QShortcut::activated, this,
          &ImagesListPanel::zoomOutRequested);

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
    QString text = entry.path;
    if (is_current) text += QStringLiteral("  (Current)");
    auto* item = new QListWidgetItem(text);
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

void ImagesListPanel::MoveSelectedRow(int offset) {
  const int from = list_->currentRow();
  const int to = from + offset;
  if (from < 0 || to < 0 || to >= list_->count()) return;

  updating_ = true;
  QListWidgetItem* item = list_->takeItem(from);
  list_->insertItem(to, item);
  list_->setCurrentRow(to);
  updating_ = false;

  SyncEntriesFromList();
  EmitEntriesChanged();
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
