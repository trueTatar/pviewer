#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QMessageBox>
#include <QMetaEnum>
#include "PictureDownloader.hpp"
#include "DownloadingDialog.hpp"

void PictureDownloader::createConnections()
{
  connect(dialog, &DownloadingDialog::linkIsReady, this, &PictureDownloader::setLinkPart);
  connect(dialog, &DownloadingDialog::formatIsChanged, this, &PictureDownloader::setImageFormat);
  connect(dialog, &DownloadingDialog::currentIsUpdated, this, &PictureDownloader::setCurrentLoad);
  connect(dialog, &DownloadingDialog::totalIsUpdated, this, &PictureDownloader::setLoadsInTotal);
  connect(dialog, &DownloadingDialog::startDownloading, this, &PictureDownloader::postRequest);
  connect(dialog, &DownloadingDialog::dialogIsReset, &folder, &Folder::reset);
  connect(m_pnam, SIGNAL(finished(QNetworkReply*)), SLOT(getReply(QNetworkReply*)));
  connect(&folder, &Folder::folderIsPrepared, dialog, &DownloadingDialog::setDestinationFolder);
}

PictureDownloader::PictureDownloader(QObject* pobj) : QObject(pobj)
{
  setImageFormat(".jpg");
  setCurrentLoad("1");
  m_pnam = new QNetworkAccessManager(this);

  dialog = new DownloadingDialog(dynamic_cast<QWidget*>(pobj));
  dialog->prepareDialog();
  createConnections();
}

void PictureDownloader::postRequest()
{
  QUrl url(linkPart + QString::number(currentLoad) + imageFormat);
  QNetworkRequest request(url);
  QSslConfiguration conf = request.sslConfiguration();
  conf.setPeerVerifyMode(QSslSocket::VerifyNone);
  request.setSslConfiguration(conf);

  m_pnam->get(request);
}

void PictureDownloader::getReply(QNetworkReply* pnr)
{
  auto err = pnr->error();
  switch(err) {
    case QNetworkReply::NoError: {
      QString strFileName = folder.path() + "/" + QString::number(currentLoad) + imageFormat;
      QFile file(strFileName);
      if (file.open(QIODevice::WriteOnly)) {
        file.write(pnr->readAll());
        file.close();

        dialog->setProgressValue(currentLoad);
        emit imageIsDownloaded(QFileInfo(file).absoluteFilePath());
      }
    } break; // end of case QNetwordReply::NoError

    default: {
      QString message = QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(err);
      QMessageBox::critical(dynamic_cast<QWidget*>(parent()), "error", message);
      dialog->upgradeLink();
    } break; // end of defauld case
  }

  pnr->deleteLater();

  if (isDone()) {
    dialog->displaySuccess();
  }
  if (!isDone() && err == QNetworkReply::NoError) {
    ++currentLoad;
    postRequest();
  }
}

bool PictureDownloader::isDone() {
  return currentLoad == loadsInTotal;
}

void PictureDownloader::setLinkPart(QString value) {
  linkPart = value;
};

void PictureDownloader::setImageFormat(QString value) {
  imageFormat = value;
}

void PictureDownloader::setCurrentLoad(QString value) {
  currentLoad = value.toInt();
}

void PictureDownloader::setLoadsInTotal(QString value) {
  loadsInTotal = value.toInt();
}

void PictureDownloader::showDialog() {
  dialog->show();
}