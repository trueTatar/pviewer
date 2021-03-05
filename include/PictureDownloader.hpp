#ifndef PICTUREDOWNLOADER_HPP
#define PICTUREDOWNLOADER_HPP

#include <QObject>
#include <QDir>
#include "global_path.hpp"

class QNetworkAccessManager;
class QNetworkReply;
class QLineEdit;
class QDir;
class DownloadingDialog;

class Folder : public QObject {
  Q_OBJECT
private:
  QString path_;
public:
  QString path()
  {
    if (path_.isEmpty()) {
      QString parentFolder = "Downloads";
      QDir root(g_basicPath + parentFolder);
      for (int i = 0; i != 100; ++i) {
        QString folderNumber = QString::number(i);
        if (!root.exists(folderNumber)) {
          root.mkpath(folderNumber);
          root.cd(folderNumber);
          path_ = root.absolutePath();
          emit folderIsPrepared(path_);
          break;
        }
      }
    }
    return path_;
    
  }
  void reset() { path_.clear(); }
signals:
  void folderIsPrepared(QString);
};

class PictureDownloader : public QObject {
  Q_OBJECT
private:
  Folder folder;
  QNetworkAccessManager* m_pnam;
  DownloadingDialog* dialog;
  QString linkPart;
  QString imageFormat;
  int currentLoad;
  int loadsInTotal;

  bool isDone();
  void createConnections();

public:
  PictureDownloader( QObject* = nullptr );

public slots:
  void postRequest();
  void getReply(QNetworkReply*);
  void setLinkPart(QString);
  void setImageFormat(QString);
  void setCurrentLoad(QString);
  void setLoadsInTotal(QString);
  void showDialog();

signals:
  void errorOccured();
  void imageIsDownloaded(QString);
};

#endif // PICTUREDOWNLOADER_HPP