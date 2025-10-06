#ifndef DOWNLOADCONFIGDIALOG_H
#define DOWNLOADCONFIGDIALOG_H

#include <QDialog>
#include "ui_downloadconfigdialog.h"

namespace Ui {
class DownloadConfigDialog;
}

class DownloadConfigDialog : public QDialog {
  Q_OBJECT

public:
  explicit DownloadConfigDialog(QWidget *parent = nullptr);
  ~DownloadConfigDialog();

  QString getExchange() const;
  QString getSymbol() const;
  QString getTimeframe() const;
  int getLimit() const;
  QDateTime getStartTime() const;
  QDateTime getEndTime() const;
  QString getOutputPath() const;

private:
  void initializeApplication(); // 整体初始化

private:
  Ui::DownloadConfigDialog *ui;
};

#endif // DOWNLOADCONFIGDIALOG_H
