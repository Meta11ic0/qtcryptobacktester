#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "downloaddialog.h"

#include <QCandlestickSeries>
#include <QCandlestickSet>
#include <QChart>
#include <QChartView>
#include <QDateTime>
#include <QDateTimeAxis>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QScatterSeries>
#include <QSlider>
#include <QTextStream>
#include <QValueAxis>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , progress_dialog_(nullptr)
    , price_chart_(nullptr)
    , chart_view_(nullptr)
    , candle_series_(nullptr)
    , buy_series_(nullptr)
    , sell_series_(nullptr)
    , axis_x_(nullptr)
    , axis_y_(nullptr)
    , scroll_bar_(nullptr)
    , visible_count_(60) {
  ui->setupUi(this);
  initializeApplication();
}

MainWindow::~MainWindow() {
  candle_series_->clear();
  current_kline_data_.clear();
  signals_.clear();
  delete ui;
}

void MainWindow::onDataFileSelected(int index) {
  if (index >= 0 && index < all_data_files_.size()) {
    current_data_file_ = all_data_files_[index];
    showProgress(QString("正在加载: %1").arg(QFileInfo(current_data_file_).fileName()));
    if (loadKLineData(current_data_file_)) {
      clearProgress();
      buildChartBasic();
      setChartRange(0);
      statusBar()->showMessage(QString("已加载: %1 (%2条数据)")
                                   .arg(QFileInfo(current_data_file_).fileName())
                                   .arg(current_data_file_.size()),
                               3000);
    } else {
      clearProgress();
      showError("加载数据文件失败");
    }
  }
}

void MainWindow::onDownloadDataClicked() {
  DownloadDialog dialog(this);
  if (dialog.exec() != QDialog::Accepted) {
    return; // 用户取消
  }
  showProgress("正在配置下载参数...");
  QString exchange = dialog.getExchange();
  QString symbol = dialog.getSymbol();
  QString timeframe = dialog.getTimeframe();
  QDateTime startTime = dialog.getStartTime();
  QDateTime endTime = dialog.getEndTime();
  QString outputPath = dialog.getOutputPath();
  if (!checkPythonEnvironment()) {
    clearProgress();
    return;
  }
  int estimatedBars = calculateEstimatedBars(startTime, endTime, timeframe);
  bool needBatchDownload = (estimatedBars > 1000);
  showProgress(QString("下载数据量为：%1条").arg(estimatedBars));
  if (needBatchDownload) {
    // TODO: 实现分批下载
    // executeBatchDownload(exchange, symbol, timeframe, startTime, estimatedBars, outputPath);
    showError("暂不支持超过1000条！");
  } else {
    executeSingleDownload(exchange, symbol, timeframe, startTime, estimatedBars, outputPath);
  }
  clearProgress();
}

void MainWindow::onAddFileClicked() {
  QString defaultDir = getDataDirectory();
  QStringList fileNames = QFileDialog::getOpenFileNames(this,
                                                        "选择数据文件",
                                                        defaultDir,
                                                        "CSV Files (*.csv);;All Files (*)");
  if (fileNames.isEmpty()) {
    return;
  }
  int addedCount = 0;
  for (const QString &filePath : std::as_const(fileNames)) {
    addDataFileToComboBox(filePath);
    addedCount++;
  }
  if (addedCount > 0) {
    statusBar()->showMessage(QString("成功添加 %1 个数据文件").arg(addedCount), 3000);
  }
}

void MainWindow::onScrollChanged(int value) {
  setChartRange(value);
}

void MainWindow::initializeApplication() {
  // 初始化进度对话框
  progress_dialog_ = new QProgressDialog(this);
  progress_dialog_->setCancelButton(nullptr);
  progress_dialog_->setWindowModality(Qt::WindowModal);
  progress_dialog_->setMinimumDuration(0); // 立即显示
  progress_dialog_->setAutoClose(false);   // 不要自动关闭
  progress_dialog_->setAutoReset(false);   // 不要自动重置
  progress_dialog_->reset();               // 初始隐藏
  // 连接信号槽
  connect(ui->dataFileComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &MainWindow::onDataFileSelected);
  connect(ui->downloadDataButton, &QPushButton::clicked, this, &MainWindow::onDownloadDataClicked);
  connect(ui->addFileButton, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);

  initializeDataFiles();
  initializeStrategies();
  initializeChart();
  //如果由文件的话触发一次onDataSelected
  onDataFileSelected(0);
}

void MainWindow::initializeDataFiles() {
  QString dataDir = getDataDirectory();
  QDir directory(dataDir);
  ui->dataFileComboBox->clear();
  all_data_files_.clear();
  QStringList filters;
  filters << "*.csv";
  QFileInfoList fileList = directory.entryInfoList(filters, QDir::Files, QDir::Time);
  for (const QFileInfo &fileInfo : std::as_const(fileList)) {
    QString fileName = fileInfo.fileName();
    ui->dataFileComboBox->addItem(fileName);
    all_data_files_.append(fileInfo.absoluteFilePath());
  }
  if (fileList.isEmpty()) {
    ui->dataFileComboBox->addItem("暂无数据文件，请先下载数据");
  }
  statusBar()->showMessage(QString("已加载 %1 个数据文件").arg(fileList.size()), 3000);
}

void MainWindow::initializeStrategies() {
  QString strategiesDir = getStrategiesDirectory();
  QDir directory(strategiesDir);
  ui->strategyComboBox->clear();
  all_strategy_files_.clear();
  QStringList filters;
  filters << "*.py";
  QFileInfoList fileList = directory.entryInfoList(filters, QDir::Files, QDir::Name);
  for (const QFileInfo &fileInfo : std::as_const(fileList)) {
    QString fileName = fileInfo.fileName();
    ui->strategyComboBox->addItem(fileName);
    all_strategy_files_.append(fileInfo.absoluteFilePath());
  }
  if (fileList.isEmpty()) {
    // TODO: 可以在这里创建默认策略文件
    ui->strategyComboBox->addItem("暂无策略文件");
  }
}

void MainWindow::initializeChart() {
  price_chart_ = new QChart();
  price_chart_->setTitle("K线图 + 策略信号");
  price_chart_->legend()->setVisible(true);

  candle_series_ = new QCandlestickSeries();
  candle_series_->setName("K线");
  candle_series_->setIncreasingColor(Qt::red);
  candle_series_->setDecreasingColor(Qt::green);
  candle_series_->setBodyWidth(0.8);
  price_chart_->addSeries(candle_series_);

  // 信号散点
  buy_series_ = new QScatterSeries();
  buy_series_->setName("买入信号");
  buy_series_->setMarkerShape(QScatterSeries::MarkerShapeCircle);
  buy_series_->setColor(Qt::blue);
  buy_series_->setMarkerSize(10);

  sell_series_ = new QScatterSeries();
  sell_series_->setName("卖出信号");
  sell_series_->setMarkerShape(QScatterSeries::MarkerShapeCircle);
  sell_series_->setColor(Qt::yellow);
  sell_series_->setMarkerSize(10);

  price_chart_->addSeries(buy_series_);
  price_chart_->addSeries(sell_series_);

  axis_x_ = new QDateTimeAxis();
  axis_y_ = new QValueAxis();
  axis_x_->setFormat("yyyy-MM-dd");
  axis_x_->setTitleText("日期");
  axis_y_->setTitleText("价格");

  price_chart_->addAxis(axis_x_, Qt::AlignBottom);
  price_chart_->addAxis(axis_y_, Qt::AlignLeft);

  candle_series_->attachAxis(axis_x_);
  candle_series_->attachAxis(axis_y_);
  buy_series_->attachAxis(axis_x_);
  buy_series_->attachAxis(axis_y_);
  sell_series_->attachAxis(axis_x_);
  sell_series_->attachAxis(axis_y_);

  chart_view_ = new QChartView(price_chart_);
  chart_view_->setRenderHints(QPainter::Antialiasing);

  scroll_bar_ = new QSlider(Qt::Horizontal);
  connect(scroll_bar_, &QSlider::valueChanged, this, &MainWindow::onScrollChanged);

  ui->chartLayout->addWidget(chart_view_);
  ui->chartLayout->addWidget(scroll_bar_);
}

void MainWindow::showError(const QString &message) {
  // 状态栏显示更长时间
  statusBar()->showMessage("错误: " + message, 10000); // 10秒
  // 创建更大、更清晰的消息框
  QMessageBox msgBox(this);
  msgBox.setWindowTitle("错误");
  msgBox.setIcon(QMessageBox::Critical);
  msgBox.setText(message);
  msgBox.setMinimumSize(400, 150); // 设置最小大小
  msgBox.exec();
}

void MainWindow::showProgress(const QString &message) {
  statusBar()->showMessage(message, 3000);
  progress_dialog_->setLabelText(message);
  progress_dialog_->show();
  QApplication::processEvents(); // 处理事件循环，确保UI更新
}

void MainWindow::clearProgress() {
  statusBar()->clearMessage();
  if (progress_dialog_) {
    progress_dialog_->hide();
  }
}

QString MainWindow::getDataDirectory() {
  QDir appDir(QCoreApplication::applicationDirPath());
  QString dataDir = appDir.absoluteFilePath("data");
  if (!QDir().exists(dataDir)) {
    QDir().mkpath(dataDir);
  }
  return dataDir;
}

QString MainWindow::getDataFilePath(const QString &file_path) {
  QDir appDir(QCoreApplication::applicationDirPath());
  QString dataFilePath = appDir.absoluteFilePath("data/" + file_path);
  if (QFile::exists(dataFilePath)) {
    return dataFilePath;
  }
  return QString();
}

QString MainWindow::getStrategiesDirectory() {
  QDir appDir(QCoreApplication::applicationDirPath());
  QString strategiesDir = appDir.absoluteFilePath("strategies");
  if (!QDir().exists(strategiesDir)) {
    QDir().mkpath(strategiesDir);
  }
  return strategiesDir;
}

QString MainWindow::getStrategiesFilePath(const QString &file_path) {
  QDir appDir(QCoreApplication::applicationDirPath());
  QString dataFilePath = appDir.absoluteFilePath("strategies/" + file_path);
  if (QFile::exists(dataFilePath)) {
    return dataFilePath;
  }
  return QString();
}

void MainWindow::addDataFileToComboBox(const QString &file_path, bool userAdded) {
  QSignalBlocker blocker(ui->dataFileComboBox);
  QFileInfo fileInfo(file_path);
  if (!fileInfo.exists()) {
    showError(QString("文件不存在: %1").arg(file_path));
    return;
  }
  int empty_index = ui->dataFileComboBox->findText("暂无数据文件，请先下载数据");
  if (empty_index >= 0) {
    ui->dataFileComboBox->removeItem(empty_index);
  }
  QString file_name = fileInfo.fileName();
  int existingIndex = ui->dataFileComboBox->findText(file_name);

  if (existingIndex != -1) {
    if (userAdded) {
      auto reply = QMessageBox::question(this,
                                         "文件已存在",
                                         QString("文件 \"%1\" 已存在，是否替换？").arg(file_name),
                                         QMessageBox::Yes | QMessageBox::No);
      if (reply == QMessageBox::No)
        return;
    }
    // 替换路径
    all_data_files_[existingIndex] = file_path;
    ui->dataFileComboBox->setCurrentIndex(existingIndex);
    onDataFileSelected(existingIndex);
  } else {
    // 新文件插入到顶部
    ui->dataFileComboBox->insertItem(0, file_name);
    all_data_files_.insert(0, file_path);
    ui->dataFileComboBox->setCurrentIndex(0);
    onDataFileSelected(0);
  }
}

bool MainWindow::checkPythonEnvironment() {
  showProgress("检查Python环境和依赖...");
  QProcess process;
  process.start("python", {"--version"});
  if (!process.waitForFinished(5000) || process.exitCode() != 0) {
    showError("未找到Python环境，请先安装Python 3.7+");
    return false;
  }
  return true;
}

int MainWindow::calculateEstimatedBars(const QDateTime &start,
                                       const QDateTime &end,
                                       const QString &timeframe) {
  // 简单的计算：根据时间范围和时间周期估算数据条数
  qint64 totalSeconds = start.secsTo(end);
  // 定义每个时间周期的秒数
  QMap<QString, int> secondsPerBar
      = {{"1m", 60}, {"5m", 300}, {"15m", 900}, {"1h", 3600}, {"4h", 14400}, {"1d", 86400}};
  int secondsPerBarValue = secondsPerBar.value(timeframe, 86400);
  if (secondsPerBarValue == 0)
    return 1000; // 防止除零
  return totalSeconds / secondsPerBarValue + 1;
}

bool MainWindow::downloadDataChunk(const QString &exchange,
                                   const QString &symbol,
                                   const QString &timeframe,
                                   const QDateTime &startTime,
                                   int estimatedBars,
                                   const QString &outputPath) {
  QDir appDir(QCoreApplication::applicationDirPath());
  QString scriptPath = appDir.absoluteFilePath("scripts/downloaddata.py");
  if (scriptPath.isEmpty()) {
    showError("找不到Python脚本文件: downloaddata.py");
    return false;
  }

  QStringList scriptArgs;
  scriptArgs << "--exchange" << exchange << "--symbol" << symbol << "--timeframe" << timeframe
             << "--start" << startTime.toString("yyyy-MM-dd HH:mm:ss") << "--limit"
             << QString::number(estimatedBars) << "--output" << outputPath << "--proxy"
             << "http://127.0.0.1:7890";
  QProcess process;
  process.start("py", QStringList(scriptPath) + scriptArgs);
  if (!process.waitForFinished(30000)) {
    process.kill();
    showError("下载超时");
    return false;
  }
  if (process.exitCode() != 0) {
    QString errorOutput = process.readAllStandardError();
    showError("下载失败: " + errorOutput);
    return false;
  }
  return true;
}

void MainWindow::executeSingleDownload(const QString &exchange,
                                       const QString &symbol,
                                       const QString &timeframe,
                                       const QDateTime &startTime,
                                       int estimatedBars,
                                       const QString &outputPath) {
  showProgress("正在下载...");
  if (downloadDataChunk(exchange, symbol, timeframe, startTime, estimatedBars, outputPath)) {
    clearProgress();
    addDataFileToComboBox(outputPath, false);
    ui->dataFileComboBox->setCurrentIndex(0);
    QMessageBox::information(this,
                             "下载完成",
                             QString("数据下载完成！\n"
                                     "交易所: %1\n"
                                     "交易对: %2\n"
                                     "时间周期: %3\n"
                                     "输出文件: %4")
                                 .arg(exchange)
                                 .arg(symbol)
                                 .arg(timeframe)
                                 .arg(outputPath));
  } else {
    clearProgress();
    // 错误已经在downloadDataChunk中显示
  }
}

bool MainWindow::loadKLineData(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "无法打开文件:" << filePath;
    return false;
  }
  QTextStream in(&file);
  current_kline_data_.clear();
  QString header = in.readLine();
  if (!header.contains("timestamp")) {
    qDebug() << "CSV文件缺少表头";
    return false;
  }

  while (!in.atEnd()) {
    QString line = in.readLine();
    if (line.trimmed().isEmpty())
      continue;
    QStringList f = line.split(',');
    if (f.size() < 6)
      continue;
    KLineData d;
    bool ok;
    d.timestamp = f[0].toLongLong(&ok);
    if (!ok)
      continue;
    d.open = f[1].toDouble();
    d.high = f[2].toDouble();
    d.low = f[3].toDouble();
    d.close = f[4].toDouble();
    d.volume = f[5].toDouble();
    current_kline_data_.append(d);
  }
  file.close();
  std::sort(current_kline_data_.begin(), current_kline_data_.end(), [](auto &a, auto &b) {
    return a.timestamp < b.timestamp;
  });
  return !current_kline_data_.isEmpty();
}

void MainWindow::buildChartBasic() {
  candle_series_->clear();
  if (current_kline_data_.isEmpty())
    return;
  for (auto &d : current_kline_data_) {
    auto *set = new QCandlestickSet(d.open, d.high, d.low, d.close, d.timestamp);
    candle_series_->append(set);
  }

  int maxIndex = qMax(0, current_kline_data_.size() - visible_count_);
  scroll_bar_->setRange(0, maxIndex);
  scroll_bar_->setPageStep(visible_count_);
  scroll_bar_->setSingleStep(1);
  setChartRange(0);
}

void MainWindow::setChartRange(int value) {
  if (current_kline_data_.isEmpty())
    return;
  int maxStart = qMax(0, current_kline_data_.size() - visible_count_);
  int start_index = qBound(0, value, maxStart);
  int end_index = qMin(start_index + visible_count_, current_kline_data_.size());

  qint64 interval = (current_kline_data_.size() >= 2)
                        ? (current_kline_data_[1].timestamp - current_kline_data_[0].timestamp)
                        : (24 * 3600 * 1000);
  qint64 start_ms = current_kline_data_[start_index].timestamp - interval / 2;
  qint64 end_ms = current_kline_data_[end_index - 1].timestamp + interval / 2;
  axis_x_->setRange(QDateTime::fromMSecsSinceEpoch(start_ms),
                    QDateTime::fromMSecsSinceEpoch(end_ms));

  double min_p = current_kline_data_[start_index].low,
         max_p = current_kline_data_[start_index].high;
  for (int i = start_index + 1; i < end_index; i++) {
    min_p = qMin(min_p, current_kline_data_[i].low);
    max_p = qMax(max_p, current_kline_data_[i].high);
  }
  double margin = (max_p - min_p) * 0.1;
  axis_y_->setRange(min_p - margin, max_p + margin);
}
