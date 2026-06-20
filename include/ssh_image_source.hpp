#pragma once

#include <QImage>
#include <QList>
#include <QString>

bool IsSshImageUrl(QString const& location);
QList<QString> ListSshImages(QString const& directory_url, QString* error);
QImage LoadSshImage(QString const& image_url, QString* error = nullptr);
