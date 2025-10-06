#include "downloadconfigdialog.h"
#include "ui_downloadconfigdialog.h"

#include <QStandardPaths>

DownloadConfigDialog::DownloadConfigDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DownloadConfigDialog) {
  ui->setupUi(this);
  initializeApplication();
}

void DownloadConfigDialog::initializeApplication() {
  // 初始化交易所列表
  ui->exchangeComboBox->addItems({"binance", "okx", "huobi", "bybit"});
  // 初始化交易对列表
  ui->symbolComboBox->addItems({"BTC/USDT", "ETH/USDT", "BNB/USDT", "ADA/USDT"});
  // 初始化时间周期
  ui->timeframeComboBox->addItems({"1d"});
  // 设置默认时间范围（最近一年）
  ui->startDateTimeEdit->setTimeZone(QTimeZone::utc());
  ui->endDateTimeEdit->setTimeZone(QTimeZone::utc());
  ui->endDateTimeEdit->setDateTime(QDateTime::currentDateTimeUtc());
  ui->startDateTimeEdit->setDateTime(QDateTime::currentDateTimeUtc().addDays(-365));
  // 设置默认输出路径
  QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                        + "/crypto_data.csv";
  ui->outputPathLineEdit->setText(defaultPath);
}

DownloadConfigDialog::~DownloadConfigDialog() {
  delete ui;
}

QString DownloadConfigDialog::getExchange() const {
  return ui->exchangeComboBox->currentText();
}

QString DownloadConfigDialog::getSymbol() const {
  return ui->symbolComboBox->currentText();
}

QString DownloadConfigDialog::getTimeframe() const {
  return ui->timeframeComboBox->currentText();
}

QDateTime DownloadConfigDialog::getStartTime() const {
  return ui->startDateTimeEdit->dateTime();
}

QDateTime DownloadConfigDialog::getEndTime() const {
  return ui->endDateTimeEdit->dateTime();
}

QString DownloadConfigDialog::getOutputPath() const {
  return ui->outputPathLineEdit->text();
}
