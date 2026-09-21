#pragma once

#include <QList>
#include <QString>
#include <QtGlobal>

struct ImageListEntry {
  QString path;
  qint64 modified_msecs_since_epoch = 0;
};

void SortImagesNewestFirst(QList<ImageListEntry>& entries);
QList<QString> ImagePaths(QList<ImageListEntry> entries);
