#ifndef DOWNLOADDIALOG_H
#define DOWNLOADDIALOG_H

#include <QDialog>

namespace Ui {
class DownloadDialog;
}

class DownloadDialog : public QDialog {
  Q_OBJECT

public:
  explicit DownloadDialog(QWidget *parent = nullptr);
  ~DownloadDialog();

  QString getExchange() const;
  QString getSymbol() const;
  QString getTimeframe() const;
  QDateTime getStartTime() const;
  QDateTime getEndTime() const;
  QString getOutputPath() const;

private slots:
  void onBrowseButtonClicked();

private:
  void initializeApplication(); // 整体初始化
  QString getDataDirectory() const;

private:
  Ui::DownloadDialog *ui;
};

#endif // DOWNLOADDIALOG_H
