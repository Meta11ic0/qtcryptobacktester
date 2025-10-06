#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "downloadconfigdialog.h"

#include <QFile>
#include <QMessageBox>
#include <QProcess>
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
  // 1. 在状态栏显示简要错误信息（非阻塞）
  statusBar()->showMessage("错误: " + message, 5000); // 显示5秒

  // 2. 对于严重错误，弹出模态对话框
  if (message.contains("Python环境") || message.contains("下载失败")) {
    QMessageBox::critical(this, "严重错误", message);
  }
}

void MainWindow::showProgress(const QString &message) {
  // 1. 在状态栏显示进度信息
  statusBar()->showMessage(message);
  // 2. 对于长时间操作，显示进度对话框
  progressDialog->setCancelButton(nullptr); // 不可取消
  progressDialog->setWindowModality(Qt::WindowModal);
  progressDialog->setLabelText(message);
  progressDialog->show();
  // 处理事件循环，确保UI更新
  QApplication::processEvents();
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

  // 检查必要库是否安装
  process.start("python", {"-c", "import ccxt, pandas"});
  if (!process.waitForFinished(5000) || process.exitCode() != 0) {
    showError("缺少必要的Python库，请手动安装：\n\npip install ccxt pandas");
    return false;
  }

  return true;
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
  int limit = dialog.getLimit();
  QDateTime startTime = dialog.getStartTime();
  QDateTime endTime = dialog.getEndTime();
  QString outputPath = dialog.getOutputPath();

  // 3. 检查Python环境（合并后的单一检查）
  if (!checkPythonEnvironment()) {
    clearProgress();
    return;
  }

  // 4. 执行下载
  showProgress("正在下载数据...");

  QString appDir = QApplication::applicationDirPath();
  QString scriptPath = appDir + "/scripts/download_data.py";

  QFile scriptFile(scriptPath);
  if (!scriptFile.exists()) {
    showError(QString("脚本文件不存在: %1").arg(scriptPath));
    clearProgress();
    return;
  }

  // 构建Python脚本参数
  QStringList scriptArgs;
  scriptArgs << "--exchange" << exchange << "--symbol" << symbol << "--timeframe" << timeframe
             << "--limit" << QString::number(limit) << "--start"
             << startTime.toString("yyyy-MM-dd HH:mm:ss") << "--end"
             << endTime.toString("yyyy-MM-dd HH:mm:ss") << "--output" << outputPath;

  QProcess process;
  process.start("python", QStringList(scriptPath) + scriptArgs);

  if (!process.waitForFinished(30000)) {
    process.kill();
    showError("下载超时，请检查网络连接");
    clearProgress();
    return;
  }

  if (process.exitCode() != 0) {
    QString errorOutput = process.readAllStandardError();
    showError(QString("下载失败：%1").arg(errorOutput));
    clearProgress();
    return;
  }

  showProgress("数据下载完成");
  clearProgress();
}
