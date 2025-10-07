#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressDialog>
#include <QString>
#include <QStringList>
#include <QVector>

// 前向声明
class QChart;
class QChartView;
class QCandlestickSeries;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// 基础数据结构
struct KLineData {
  QString timestamp;
  double open;
  double high;
  double low;
  double close;
  double volume;
};

struct TradeSignal {
  QString timestamp;
  QString type; // "BUY" or "SELL"
  double price;
};

struct BacktestResult {
  double initialCapital;
  double finalCapital;
  double totalReturn;
  int totalTrades;
  int winningTrades;
  double winRate;
  double maxDrawdown;
  QVector<TradeSignal> trades;
};

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

private slots:
  // 数据管理槽函数 - 对应UI中的按钮
  void onDataFileSelected(int index); // dataFileComboBox选择变化
  void onDownloadDataClicked();       // downloadDataButton点击
  //void onAddFileClicked();      // addFileButton点击

  // 策略管理槽函数 - 对应UI中的按钮
  void onStrategySelected(int index); // strategyComboBox选择变化
  //void onCreateStrategyClicked();     // createStrategyButton点击
  //void onEditStrategyClicked();       // editStrategyButton点击

  // 回测执行槽函数 - 对应UI中的按钮
  //void onStartBacktestClicked(); // startBacktestButton点击

private:
  // 初始化函数 - 程序启动时需要执行
  void initializeApplication(); // 整体初始化
  void loadDataFiles();         // 加载数据文件列表到dataFileComboBox
  void loadStrategies();        // 加载策略文件列表到strategyComboBox
  void initializeChart();       // 初始化K线图表

  // 数据管理函数 - 处理CSV数据和文件操作
  bool loadCSVData(const QString& filePath);           // 加载选定的CSV数据文件
  QString getSelectedDataFile() const;                 // 获取当前选中的数据文件路径
  void addDataFileToComboBox(const QString& filePath); // 添加数据文件到下拉框

  // 策略管理函数 - 处理Python策略文件
  QString getSelectedStrategy() const; // 获取当前选中的策略文件路径
  bool createNewStrategy();            // 创建新的策略模板文件
  bool editStrategyFile();             // 打开策略文件进行编辑

  // 图表函数 - 处理K线图显示
  void updateChart();                                    // 更新图表显示当前数据
  void clearChart();                                     // 清空图表
  void addTradeMarks(const QVector<TradeSignal>& marks); // 在图表上添加交易信号标记

  // 回测引擎函数 - 核心回测逻辑
  //BacktestResult runBacktest();                            // 执行回测并返回结果
  //void updateResultsDisplay(const BacktestResult& result); // 更新结果显示区域

  // 工具函数 - 辅助功能
  bool validateInputs();                     // 验证用户输入参数有效性
  void showError(const QString& message);    // 显示错误信息
  void showProgress(const QString& message); // 显示信息
  void clearProgress();
  bool checkPythonEnvironment(); //检查python环境
  QString getScriptPath(const QString& scriptName);
  QString getDataDirectory();       // 获取数据目录
  QString getStrategiesDirectory(); // 获取策略目录
  int calculateEstimatedBars(const QDateTime& start, const QDateTime& end, const QString& timeframe);
  bool downloadDataChunk(const QString& exchange,
                         const QString& symbol,
                         const QString& timeframe,
                         const QDateTime& startTime,
                         int estimatedBars,
                         const QString& outputPath);
  void executeSingleDownload(const QString& exchange,
                             const QString& symbol,
                             const QString& timeframe,
                             const QDateTime& startTime,
                             int estimatedBars,
                             const QString& outputPath);

private:
  Ui::MainWindow* ui;

  // 数据管理变量 - 存储数据文件相关信息
  QStringList dataFilePaths;      // 所有数据文件的路径列表
  QVector<KLineData> currentData; // 当前加载的K线数据
  QString currentDataFile;        // 当前选中的数据文件路径

  // 策略管理变量 - 存储策略文件相关信息
  QStringList strategyFiles;   // 所有策略文件的路径列表
  QString currentStrategyFile; // 当前选中的策略文件路径

  // 图表变量 - 图表组件指针
  QChart* priceChart;               // K线图表对象
  QChartView* chartView;            // 图表视图
  QCandlestickSeries* candleSeries; // K线序列

  QProgressDialog* progressDialog;
  // 回测状态变量 - 跟踪回测过程状态
  //bool backtestRunning;      // 回测是否正在进行中
  //BacktestResult lastResult; // 上一次回测的结果
};

#endif // MAINWINDOW_H
