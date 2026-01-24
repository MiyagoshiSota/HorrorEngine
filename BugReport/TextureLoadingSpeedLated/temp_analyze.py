#!/usr/bin/env python3
import re
from collections import defaultdict

def parse_file(filepath):
    metrics = defaultdict(list)
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            match = re.match(r'^\s+([^:]+?):\s+([\d.]+)\s+ms\s*$', line)
            if match:
                metric_name = match.group(1).strip()
                value = float(match.group(2))
                metrics[metric_name].append(value)
    return metrics

# 両方のファイルを解析
metrics1 = parse_file('Phase1Performance.txt')
metrics2 = parse_file('Phase1Performance02.txt')

# マージ
all_metrics = defaultdict(list)
for metric_name in set(list(metrics1.keys()) + list(metrics2.keys())):
    if metric_name in metrics1:
        all_metrics[metric_name].extend(metrics1[metric_name])
    if metric_name in metrics2:
        all_metrics[metric_name].extend(metrics2[metric_name])

# 統計計算
stats = {}
total_avg = 0.0
for metric_name, values in all_metrics.items():
    min_val = min(values)
    max_val = max(values)
    avg_val = sum(values) / len(values)
    stats[metric_name] = {'min': min_val, 'max': max_val, 'avg': avg_val, 'count': len(values)}
    total_avg += avg_val

# Markdown形式で出力
print("#### 定量評価\n")
print("**解析対象**: Phase1Performance.txt (107テクスチャ) + Phase1Performance02.txt (107テクスチャ) = 合計214サンプル\n")
print("| ステップ | 最小値 (ms) | 最大値 (ms) | 平均値 (ms) | サンプル数 | 全体に占める割合 |")
print("|---------|------------|------------|------------|----------|----------------|")

# 平均値でソート
sorted_stats = sorted(stats.items(), key=lambda x: x[1]['avg'], reverse=True)

for metric_name, stat in sorted_stats:
    percentage = (stat['avg'] / total_avg) * 100.0 if total_avg > 0 else 0.0
    print(f"| {metric_name} | {stat['min']:.3f} | {stat['max']:.3f} | {stat['avg']:.3f} | {stat['count']} | {percentage:.2f}% |")

print(f"| **Total** | | | **{total_avg:.3f}** | | **100.00%** |")
