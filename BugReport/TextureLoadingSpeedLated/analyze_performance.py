#!/usr/bin/env python3
"""
Phase1Performance.txtを解析し、各メトリクスの統計情報を計算するスクリプト
"""

import re
import os
from collections import defaultdict
from typing import Dict, List, Tuple

def parse_performance_file(file_path: str) -> Dict[str, List[float]]:
    """
    パフォーマンスファイルを解析し、各メトリクスの値を収集する
    
    Returns:
        Dict[str, List[float]]: メトリクス名をキー、値のリストを値とする辞書
    """
    metrics = defaultdict(list)
    
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    i = 0
    while i < len(lines):
        line = lines[i].rstrip('\n\r')  # 行末の改行のみ削除、行頭の空白は保持
        
        # [Texture Load] で始まる行はスキップ
        if line.strip().startswith('[Texture Load]'):
            i += 1
            continue
        
        # メトリクス行のパターン: "  Metric Name: value ms"
        # 行頭に空白があり、コロンで区切られた形式
        # より柔軟なパターン: 行頭の空白 + メトリクス名 + コロン + 値 + ms
        match = re.match(r'^\s+([^:]+?):\s+([\d.]+)\s+ms\s*$', line)
        if match:
            metric_name = match.group(1).strip()
            value = float(match.group(2))
            metrics[metric_name].append(value)
        
        i += 1
    
    return dict(metrics)

def calculate_statistics(values: List[float]) -> Tuple[float, float, float]:
    """
    値のリストから統計情報を計算する
    
    Returns:
        Tuple[float, float, float]: (min, max, average)
    """
    if not values:
        return 0.0, 0.0, 0.0
    
    return min(values), max(values), sum(values) / len(values)

def calculate_percentage(value: float, total: float) -> float:
    """
    全体に対する割合を計算する（パーセント）
    """
    if total == 0:
        return 0.0
    return (value / total) * 100.0

def format_table(metrics: Dict[str, List[float]]) -> str:
    """
    統計情報を表形式でフォーマットする
    """
    # 各メトリクスの統計を計算
    stats = {}
    total_avg = 0.0
    
    for metric_name, values in metrics.items():
        min_val, max_val, avg_val = calculate_statistics(values)
        stats[metric_name] = {
            'min': min_val,
            'max': max_val,
            'avg': avg_val,
            'count': len(values)
        }
        total_avg += avg_val
    
    # 表のヘッダー
    header = f"{'Metric':<35} {'Min (ms)':>12} {'Max (ms)':>12} {'Ave (ms)':>12} {'Count':>8} {'% of Total':>12}"
    separator = "-" * len(header)
    
    # 表の行を生成
    rows = [header, separator]
    
    # メトリクス名でソート
    sorted_metrics = sorted(stats.items(), key=lambda x: x[1]['avg'], reverse=True)
    
    for metric_name, stat in sorted_metrics:
        percentage = calculate_percentage(stat['avg'], total_avg)
        row = f"{metric_name:<35} {stat['min']:>12.3f} {stat['max']:>12.3f} {stat['avg']:>12.3f} {stat['count']:>8} {percentage:>11.2f}%"
        rows.append(row)
    
    # 合計行
    rows.append(separator)
    total_row = f"{'TOTAL (Average)':<35} {'':>12} {'':>12} {total_avg:>12.3f} {'':>8} {'100.00':>11}%"
    rows.append(total_row)
    
    return "\n".join(rows)

def merge_metrics(metrics1: Dict[str, List[float]], metrics2: Dict[str, List[float]]) -> Dict[str, List[float]]:
    """
    2つのメトリクス辞書をマージする
    """
    merged = defaultdict(list)
    
    # すべてのメトリクス名を収集
    all_metrics = set(metrics1.keys()) | set(metrics2.keys())
    
    for metric_name in all_metrics:
        if metric_name in metrics1:
            merged[metric_name].extend(metrics1[metric_name])
        if metric_name in metrics2:
            merged[metric_name].extend(metrics2[metric_name])
    
    return dict(merged)

def main():
    import sys
    
    # スクリプトと同じディレクトリのファイルを使用
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # コマンドライン引数からファイル名を取得、なければデフォルト
    if len(sys.argv) >= 2:
        input_files = sys.argv[1:]
    else:
        input_files = [
            os.path.join(script_dir, "Phase1Performance.txt"),
            os.path.join(script_dir, "Phase1Performance02.txt")
        ]
    
    output_file = os.path.join(script_dir, "Phase1Performance_Analysis.txt")
    
    # すべてのファイルからメトリクスを収集
    all_metrics = {}
    
    for input_file in input_files:
        if not os.path.exists(input_file):
            print(f"警告: ファイルが見つかりません: {input_file}")
            continue
        
        print(f"解析中: {input_file}")
        metrics = parse_performance_file(input_file)
        
        if not metrics:
            print(f"警告: {input_file} からメトリクスが見つかりませんでした")
            continue
        
        # メトリクスをマージ
        if not all_metrics:
            all_metrics = metrics
        else:
            all_metrics = merge_metrics(all_metrics, metrics)
    
    if not all_metrics:
        print("エラー: メトリクスが見つかりませんでした")
        return
    
    print(f"見つかったメトリクス: {len(all_metrics)}")
    for metric_name, values in all_metrics.items():
        print(f"  {metric_name}: {len(values)} 件")
    
    # 表を生成
    table = format_table(all_metrics)
    
    # 結果をファイルに出力
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("Phase1 Performance Analysis (Combined)\n")
        f.write("=" * 100 + "\n\n")
        f.write(f"解析対象ファイル: {', '.join(input_files)}\n\n")
        f.write(table)
        f.write("\n\n")
    
    # コンソールにも出力
    print("\n" + "=" * 100)
    print("解析結果:")
    print("=" * 100)
    print(table)
    print("\n")
    print(f"結果を {output_file} に保存しました")

if __name__ == "__main__":
    main()
