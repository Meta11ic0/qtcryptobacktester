#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

class QChart;
class QChartView;
class QCandlestickSeries;
class QScatterSeries;
class QValueAxis;
class QDateTimeAxis;
class QSlider;
class QProgressDialog;

struct KLineData {
  qint64 timestamp;
  double open;
  double high;
  double low;
  double close;
  double volume;
};

enum class SignalType { Buy, Sell };

struct TradeSignal {
  qint64 timestamp;
  double price;
  SignalType type;
};

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

private slots:
  void onDataFileSelected(int index); // dataFileComboBox选择变化
  void onDownloadDataClicked();       // downloadDataButton点击
  void onAddFileClicked();            // addFileButton点击
  void onScrollChanged(int value);

private:
  //初始化函数
  void initializeApplication();
  void initializeDataFiles();
  void initializeStrategies();
  void initializeChart();

  //辅助信息展示
  void showError(const QString& message);    // 显示错误信息
  void showProgress(const QString& message); // 显示进度信息
  void clearProgress();                      // 清除进度显示

  //获取相关文件
  QString getDataDirectory();
  QString getDataFilePath(const QString& file_path);
  QString getStrategiesDirectory();
  QString getStrategiesFilePath(const QString& file_path);

  //图表展示相关
  bool loadKLineData(const QString& file_path);
  void buildChartBasic();
  void setChartRange(int value);

  //下载相关
  void addDataFileToComboBox(const QString& filePath, bool need_copied = true);
  bool checkPythonEnvironment();
  int calculateEstimatedBars(const QDateTime& start,
                             const QDateTime& end,
                             const QString& timeframe); // 计算预估数据条数
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
  QProgressDialog* progress_dialog_;

  QChart* price_chart_;
  QChartView* chart_view_;
  QCandlestickSeries* candle_series_;
  QScatterSeries* buy_series_;
  QScatterSeries* sell_series_;
  QDateTimeAxis* axis_x_;
  QValueAxis* axis_y_;
  QSlider* scroll_bar_;

  QVector<KLineData> current_kline_data_;
  QVector<TradeSignal> signals_;
  QStringList all_data_files_;
  QString current_data_file_;
  QStringList all_strategy_files_;
  QString current_strategy_file_;

  int visible_count_;
};

#endif // MAINWINDOW_H
