#pragma once

#include <QLatin1Char>
#include <QString>
#include <QStringList>

/*
 * Single source of truth for the image formats the viewer understands.
 * All matching is case-insensitive, so ".JPG", ".Png", etc. are accepted.
 */

inline const QStringList &SupportedImageSuffixes() {
  static const QStringList suffixes{QStringLiteral("jpg"),
                                    QStringLiteral("jpeg"),
                                    QStringLiteral("png")};
  return suffixes;
}

inline bool IsSupportedImageSuffix(const QString &suffix) {
  return SupportedImageSuffixes().contains(suffix, Qt::CaseInsensitive);
}

// Wildcard filters for QDir::setNameFilters / QFileDialog, e.g. "*.jpg".
inline QStringList SupportedImageNameFilters() {
  QStringList filters;
  filters.reserve(SupportedImageSuffixes().size());
  for (const QString &suffix : SupportedImageSuffixes())
    filters << (QStringLiteral("*.") + suffix);
  return filters;
}
