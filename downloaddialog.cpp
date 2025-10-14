#include "downloaddialog.h"
#include "ui_downloaddialog.h"

#include <QDir>
#include <QFileDialog>

DownloadDialog::DownloadDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DownloadDialog) {
  ui->setupUi(this);
  initializeApplication();
}

DownloadDialog::~DownloadDialog() {
  delete ui;
}

void DownloadDialog::initializeApplication() {
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
  QString defaultOutputPath = getDataDirectory() + "/crypto_data.csv";
  ui->outputPathLineEdit->setText(defaultOutputPath);
  // 连接浏览按钮的信号槽
  connect(ui->browseButton, &QPushButton::clicked, this, &DownloadDialog::onBrowseButtonClicked);
}

QString DownloadDialog::getDataDirectory() const {
  QDir appDir(QCoreApplication::applicationDirPath());
  QString dataDir = appDir.absoluteFilePath("data");
  if (!QDir().exists(dataDir)) {
    QDir().mkpath(dataDir);
  }
  return dataDir;
}

void DownloadDialog::onBrowseButtonClicked() {
  // 获取默认数据目录
  QString defaultDir = getDataDirectory();

  // 打开文件选择对话框
  QString fileName = QFileDialog::getSaveFileName(this,
                                                  "选择保存路径",
                                                  defaultDir + "/crypto_data.csv",   // 默认文件名
                                                  "CSV Files (*.csv);;All Files (*)" // 文件过滤器
  );

  // 如果用户选择了文件（没有取消）
  if (!fileName.isEmpty()) {
    // 确保文件以.csv结尾
    if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
      fileName += ".csv";
    }

    // 更新路径显示
    ui->outputPathLineEdit->setText(fileName);
  }
}

QString DownloadDialog::getExchange() const {
  return ui->exchangeComboBox->currentText();
}

QString DownloadDialog::getSymbol() const {
  return ui->symbolComboBox->currentText();
}

QString DownloadDialog::getTimeframe() const {
  return ui->timeframeComboBox->currentText();
}

QDateTime DownloadDialog::getStartTime() const {
  return ui->startDateTimeEdit->dateTime();
}

QDateTime DownloadDialog::getEndTime() const {
  return ui->endDateTimeEdit->dateTime();
}

QString DownloadDialog::getOutputPath() const {
  return ui->outputPathLineEdit->text();
}
