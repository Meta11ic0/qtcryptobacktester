#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "downloadconfigdialog.h"

#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile> // 添加这个头文件
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , progressDialog(new QProgressDialog(this)) {
  ui->setupUi(this);
  // 确保状态栏存在
  if (!statusBar()) {
    setStatusBar(new QStatusBar(this));
  }
  initializeApplication();
}

MainWindow::~MainWindow() {
  delete ui;
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
  progressDialog->setLabelText(message);
  progressDialog->show();
  QApplication::processEvents(); // 处理事件循环，确保UI更新
}

// 还需要一个清除进度的方法
void MainWindow::clearProgress() {
  statusBar()->clearMessage();
  if (progressDialog) {
    progressDialog->hide();
  }
}

void MainWindow::initializeApplication() {
  // 初始化进度对话框
  progressDialog->setCancelButton(nullptr);
  progressDialog->setWindowModality(Qt::WindowModal);
  progressDialog->setMinimumDuration(0); // 立即显示
  progressDialog->setAutoClose(false);   // 不要自动关闭
  progressDialog->setAutoReset(false);   // 不要自动重置
  progressDialog->reset();               // 初始隐藏
  //下载
  connect(ui->downloadDataButton, &QPushButton::clicked, this, &MainWindow::onDownloadDataClicked);
}

bool MainWindow::checkPythonEnvironment() {
  showProgress("检查Python环境和依赖...");

  QProcess process;

  // 检查Python是否可用
  process.start("python", {"--version"});
  if (!process.waitForFinished(5000) || process.exitCode() != 0) {
    showError("未找到Python环境，请先安装Python 3.7+");
    return false;
  }
  return true;
}

QString MainWindow::getScriptPath(const QString &scriptName) {
  QDir appDir(QCoreApplication::applicationDirPath());
  QString localScriptPath = appDir.absoluteFilePath("scripts/" + scriptName);
  if (QFile::exists(localScriptPath)) {
    return localScriptPath;
  }
  return QString();
}

QString MainWindow::getDataFilePath(const QString &dataFileName) {
  QDir appDir(QCoreApplication::applicationDirPath());
  QString dataFilePath = appDir.absoluteFilePath("data/" + dataFileName);
  if (QFile::exists(dataFilePath)) {
    return dataFilePath;
  }
  return QString();
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
  // 直接使用脚本文件路径，不再提取到临时文件
  QString scriptPath = getScriptPath("downloaddata.py");
  if (scriptPath.isEmpty()) {
    showError("找不到Python脚本文件: downloaddata.py");
    return false;
  }

  // 构建并执行Python命令
  QStringList scriptArgs;
  scriptArgs << "--exchange" << exchange << "--symbol" << symbol << "--timeframe" << timeframe
             << "--start" << startTime.toString("yyyy-MM-dd HH:mm:ss") << "--limit"
             << QString::number(estimatedBars) << "--output" << outputPath << "--proxy"
             << "http://127.0.0.1:7890";

  QString pythonPath = "D:\\Code\\DevEnv\\Python312\\python.exe";
  QProcess process;
  process.start(pythonPath, QStringList(scriptPath) + scriptArgs);

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
  // 阶段1：准备阶段
  showProgress("正在初始化下载任务...");

  // 阶段2：下载中
  showProgress("正在从交易所下载数据...");

  if (downloadDataChunk(exchange, symbol, timeframe, startTime, estimatedBars, outputPath)) {
    // 阶段3：完成提示
    clearProgress();

    // 使用模态对话框确保用户看到完成提示
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

void MainWindow::onDownloadDataClicked() {
  // 1. 弹出配置对话框
  DownloadConfigDialog dialog(this);
  if (dialog.exec() != QDialog::Accepted) {
    return; // 用户取消
  }

  // 2. 获取配置参数
  QString exchange = dialog.getExchange();
  QString symbol = dialog.getSymbol();
  QString timeframe = dialog.getTimeframe();
  QDateTime startTime = dialog.getStartTime();
  QDateTime endTime = dialog.getEndTime();
  QString outputPath = dialog.getOutputPath();

  // 3. 检查Python环境
  if (!checkPythonEnvironment()) {
    clearProgress();
    return;
  }

  // 4. 计算数据条数
  int estimatedBars = calculateEstimatedBars(startTime, endTime, timeframe);

  // 5. 决定是否分批
  bool needBatchDownload = (estimatedBars > 1000);

  if (needBatchDownload) {
    //executeBatchDownload(exchange, symbol, timeframe, startTime, estimatedBars, outputPath);
    showError("暂不支持！");
  } else {
    executeSingleDownload(exchange, symbol, timeframe, startTime, estimatedBars, outputPath);
  }

  clearProgress();
}
