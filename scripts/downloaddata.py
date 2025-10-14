#!/usr/bin/env python3
import ccxt
import pandas as pd
import argparse
import sys
import os
from datetime import datetime
import requests  # 用于测试网络连接

def test_proxy(proxy_url):
  """测试代理是否工作正常"""
  try:
    session = requests.Session()
    session.proxies = {
      'http': proxy_url,
      'https': proxy_url,
    }
    response = session.get('https://api.binance.com/api/v3/exchangeInfo', timeout=10)
    return response.status_code == 200
  except:
    return False

def main():
  parser = argparse.ArgumentParser(description='下载加密货币K线数据')
  parser.add_argument('--exchange', required=True)
  parser.add_argument('--symbol', required=True)
  parser.add_argument('--timeframe', required=True)
  parser.add_argument('--start', required=True)
  parser.add_argument('--limit', type=int, required=True)
  parser.add_argument('--output', required=True)
  parser.add_argument('--proxy', default='http://127.0.0.1:7890', help='代理服务器地址，例如 http://127.0.0.1:7890')

  args = parser.parse_args()

  try:
    # 测试代理连接
    print(f"测试代理连接: {args.proxy}")
    if not test_proxy(args.proxy):
      print(f"警告: 代理 {args.proxy} 可能无法正常工作，尝试直接连接")
      # 不退出，尝试直接连接

    # 创建交易所实例，设置代理
    exchange_config = {
      'timeout': 30000,
      'enableRateLimit': True,
    }

    # 只有在代理地址不为空时才设置代理
    if args.proxy and args.proxy != 'direct':
      exchange_config['proxies'] = {
        'http': args.proxy,
        'https': args.proxy,
      }

    exchange = getattr(ccxt, args.exchange)(exchange_config)

    since = int(datetime.strptime(args.start, '%Y-%m-%d %H:%M:%S').timestamp() * 1000)

    print(f"开始下载数据: {args.exchange}, {args.symbol}, {args.timeframe}")
    print(f"时间范围: {args.start}, 数量: {args.limit}")

    # 使用limit参数，而不是结束时间
    ohlcv = exchange.fetch_ohlcv(args.symbol, args.timeframe, since=since, limit=args.limit)

    if not ohlcv:
      print("没有获取到数据")
      sys.exit(1)

    df = pd.DataFrame(ohlcv, columns=['timestamp', 'open', 'high', 'low', 'close', 'volume'])
    df['datetime'] = pd.to_datetime(df['timestamp'], unit='ms')
    df = df.sort_values('timestamp')  # 确保数据按时间升序排列
    os.makedirs(os.path.dirname(args.output) if os.path.dirname(args.output) else '.', exist_ok=True)
    df.to_csv(args.output, index=False)
    print(f"成功下载 {len(df)} 条数据")
    print(f"数据已保存到: {args.output}")

  except ccxt.NetworkError as e:
    print(f"网络错误: {str(e)}", file=sys.stderr)
    print("请检查网络连接和代理设置", file=sys.stderr)
    sys.exit(1)
  except ccxt.ExchangeError as e:
    print(f"交易所错误: {str(e)}", file=sys.stderr)
    sys.exit(1)
  except Exception as e:
    print(f"未知错误: {str(e)}", file=sys.stderr)
    import traceback
    traceback.print_exc()
    sys.exit(1)

if __name__ == "__main__":
  main()
