#include "image_list_order.hpp"

#include <QCollator>
#include <QFileInfo>
#include <QLocale>
#include <algorithm>
#include <utility>

void SortImagesNewestFirst(QList<ImageListEntry>& entries) {
  QLocale locale = QLocale::system();
  if (locale.language() == QLocale::C) locale = QLocale(QLocale::English);
  QCollator collator(locale);
  collator.setNumericMode(true);

  std::sort(entries.begin(), entries.end(),
            [&](ImageListEntry const& lhs, ImageListEntry const& rhs) {
              if (lhs.modified_msecs_since_epoch !=
                  rhs.modified_msecs_since_epoch) {
                return lhs.modified_msecs_since_epoch >
                       rhs.modified_msecs_since_epoch;
              }

              const QString lhs_name = QFileInfo(lhs.path).fileName();
              const QString rhs_name = QFileInfo(rhs.path).fileName();
              const int name_order = collator.compare(lhs_name, rhs_name);
              if (name_order != 0) return name_order < 0;
              return lhs.path < rhs.path;
            });
}

QList<QString> ImagePaths(QList<ImageListEntry> entries) {
  QList<QString> paths;
  paths.reserve(entries.size());
  for (ImageListEntry& entry : entries) {
    paths.push_back(std::move(entry.path));
  }
  return paths;
}
