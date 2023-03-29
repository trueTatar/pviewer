#pragma once

#include <QMessageBox>
#include <QTimer>

class MessageBox {
private:
  MessageBox() {
    mb_->setIcon(QMessageBox::NoIcon);
  }
  inline static QMessageBox* mb_;
public:
  static void inform(QString info, int forTime) {
    if (mb_ == nullptr) {
      mb_ = new QMessageBox();
    }
    mb_->setText(info);
    QTimer::singleShot(forTime, mb_, &QMessageBox::accept);
    mb_->exec();
  }
};