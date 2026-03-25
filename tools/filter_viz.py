#!/usr/bin/env python3
"""
tools/filter_viz.py
生成滤波效果对比的静态 HTML 可视化页面
用法：python3 tools/filter_viz.py [--output filter_viz.html]
"""

import json
import math
import random
import argparse
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from maix.filter import (
    MovingAverage, MedianFilter, LowPassFilter,
    KalmanFilter1D, DeadZoneFilter, LimitFilter,
)

random.seed(42)

# ── 信号生成 ──────────────────────────────────────────────────────────────────

def sine_wave(n, freq=1.0, fs=100.0, amp=1.0):
    return [amp * math.sin(2 * math.pi * freq * i / fs) for i in range(n)]

def add_noise(signal, sigma=0.3):
    return [x + random.gauss(0, sigma) for x in signal]

def add_spikes(signal, n_spikes=8, amplitude=4.0):
    s = list(signal)
    for i in random.sample(range(len(s)), n_spikes):
        s[i] += amplitude * random.choice([-1, 1])
    return s

def build_datasets():
    N, FS = 300, 100.0
    t = [round(i / FS, 3) for i in range(N)]

    # ── 场景 1：高斯噪声 ──────────────────────────────────────────────────────
    clean1  = sine_wave(N, freq=2.0, fs=FS, amp=1.0)
    noisy1  = add_noise(clean1, sigma=0.35)

    filters1 = {
        "滑动平均 (w=8)":  MovingAverage(window=8),
        "低通滤波 (α=0.2)": LowPassFilter(alpha=0.2),
        "卡尔曼滤波":       KalmanFilter1D(q=1e-4, r=0.12),
    }
    out1 = {name: [f.update(x) for x in noisy1] for name, f in filters1.items()}

    # ── 场景 2：脉冲噪声 ──────────────────────────────────────────────────────
    clean2 = sine_wave(N, freq=1.5, fs=FS, amp=1.0)
    noisy2 = add_spikes(add_noise(clean2, sigma=0.1), n_spikes=12, amplitude=3.5)

    filters2 = {
        "中值滤波 (w=5)":  MedianFilter(window=5),
        "限幅滤波 (Δ=1.5)": LimitFilter(max_delta=1.5),
        "滑动平均 (w=8)":  MovingAverage(window=8),
    }
    out2 = {name: [f.update(x) for x in noisy2] for name, f in filters2.items()}

    # ── 场景 3：传感器抖动（死区 + 低通串联）────────────────────────────────
    base3  = [2.0 + 0.5 * math.sin(2 * math.pi * 0.5 * i / FS) for i in range(N)]
    noisy3 = add_noise(base3, sigma=0.08)

    dz  = DeadZoneFilter(threshold=0.15)
    lpf = LowPassFilter(alpha=0.25)
    chained = [lpf.update(dz.update(x)) for x in noisy3]

    lpf2 = LowPassFilter(alpha=0.25)
    lpf_only = [lpf2.update(x) for x in noisy3]

    out3 = {"死区+低通串联": chained, "仅低通": lpf_only}

    return t, clean1, noisy1, out1, clean2, noisy2, out2, base3, noisy3, out3


# ── HTML 模板 ─────────────────────────────────────────────────────────────────

HTML_TEMPLATE = r"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>MaixPy 滤波算法可视化</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: 'Segoe UI', system-ui, sans-serif;
         background: #0f1117; color: #e2e8f0; min-height: 100vh; }
  header { padding: 24px 32px 12px;
           border-bottom: 1px solid #1e2535; }
  header h1 { font-size: 1.4rem; font-weight: 600; color: #7dd3fc; }
  header p  { font-size: 0.85rem; color: #64748b; margin-top: 4px; }
  .grid { display: grid; grid-template-columns: 1fr 1fr;
          gap: 20px; padding: 24px 32px; }
  @media (max-width: 900px) { .grid { grid-template-columns: 1fr; } }
  .card { background: #161b27; border: 1px solid #1e2535;
          border-radius: 12px; padding: 20px; }
  .card h2 { font-size: 0.95rem; font-weight: 600;
             color: #94a3b8; margin-bottom: 4px; }
  .card p  { font-size: 0.78rem; color: #475569; margin-bottom: 16px; }
  .card-full { grid-column: 1 / -1; }
  canvas { max-height: 280px; }
  .legend { display: flex; flex-wrap: wrap; gap: 10px;
            margin-top: 12px; font-size: 0.78rem; }
  .legend-item { display: flex; align-items: center; gap: 6px; }
  .legend-dot  { width: 10px; height: 10px; border-radius: 50%; flex-shrink: 0; }
  .rmse-table  { width: 100%; border-collapse: collapse;
                 font-size: 0.82rem; margin-top: 12px; }
  .rmse-table th { text-align: left; padding: 6px 10px;
                   color: #64748b; border-bottom: 1px solid #1e2535; }
  .rmse-table td { padding: 6px 10px; border-bottom: 1px solid #1a2030; }
  .rmse-table tr:last-child td { border-bottom: none; }
  .badge { display: inline-block; padding: 2px 8px; border-radius: 4px;
           font-size: 0.75rem; font-weight: 600; }
  .badge-best { background: #14532d; color: #4ade80; }
  .badge-raw  { background: #3b1f1f; color: #f87171; }
</style>
</head>
<body>
<header>
  <h1>MaixPy-K210-STM32 · 滤波算法可视化</h1>
  <p>三种典型噪声场景下各滤波器效果对比 · 数据由 maix.filter 实时生成</p>
</header>

<div class="grid">

  <!-- 场景 1 -->
  <div class="card">
    <h2>场景 1 · 高斯白噪声</h2>
    <p>正弦信号 2Hz / σ=0.35 · 适合低通 / 卡尔曼</p>
    <canvas id="c1"></canvas>
    <div id="rmse1"></div>
  </div>

  <!-- 场景 2 -->
  <div class="card">
    <h2>场景 2 · 脉冲噪声</h2>
    <p>正弦信号 1.5Hz + 随机尖峰 · 适合中值 / 限幅</p>
    <canvas id="c2"></canvas>
    <div id="rmse2"></div>
  </div>

  <!-- 场景 3 -->
  <div class="card card-full">
    <h2>场景 3 · 传感器抖动（死区 + 低通串联）</h2>
    <p>缓变基线 + 微小抖动 σ=0.08 · 死区先过滤抖动，低通再平滑</p>
    <canvas id="c3"></canvas>
    <div id="rmse3"></div>
  </div>

</div>

<script>
const DATA = __DATA__;

const PALETTE = {
  clean:  { border: '#334155', bg: 'rgba(51,65,85,0.15)',  label: '原始信号' },
  noisy:  { border: '#ef4444', bg: 'rgba(239,68,68,0.08)', label: '含噪信号' },
  colors: [
    '#38bdf8', '#a78bfa', '#34d399', '#fb923c',
    '#f472b6', '#facc15', '#60a5fa',
  ],
};

function rmse(a, b) {
  let s = 0;
  for (let i = 0; i < a.length; i++) s += (a[i] - b[i]) ** 2;
  return Math.sqrt(s / a.length);
}

function makeChart(id, t, clean, noisy, outputs, title) {
  const ctx = document.getElementById(id).getContext('2d');
  const datasets = [
    {
      label: '原始信号',
      data: clean,
      borderColor: PALETTE.clean.border,
      borderWidth: 1.5,
      pointRadius: 0,
      tension: 0.3,
      borderDash: [4, 3],
    },
    {
      label: '含噪信号',
      data: noisy,
      borderColor: PALETTE.noisy.border,
      borderWidth: 1,
      pointRadius: 0,
      tension: 0,
      backgroundColor: PALETTE.noisy.bg,
      fill: false,
    },
  ];

  const names = Object.keys(outputs);
  names.forEach((name, i) => {
    datasets.push({
      label: name,
      data: outputs[name],
      borderColor: PALETTE.colors[i % PALETTE.colors.length],
      borderWidth: 2,
      pointRadius: 0,
      tension: 0.3,
    });
  });

  new Chart(ctx, {
    type: 'line',
    data: { labels: t, datasets },
    options: {
      animation: false,
      responsive: true,
      interaction: { mode: 'index', intersect: false },
      plugins: {
        legend: {
          labels: { color: '#94a3b8', font: { size: 11 }, boxWidth: 12 },
        },
        tooltip: {
          backgroundColor: '#1e2535',
          titleColor: '#94a3b8',
          bodyColor: '#cbd5e1',
          callbacks: {
            label: ctx => ` ${ctx.dataset.label}: ${ctx.parsed.y.toFixed(3)}`,
          },
        },
      },
      scales: {
        x: {
          ticks: { color: '#475569', maxTicksLimit: 10,
                   callback: v => t[v] + 's' },
          grid: { color: '#1a2030' },
        },
        y: {
          ticks: { color: '#475569' },
          grid: { color: '#1a2030' },
        },
      },
    },
  });

  // RMSE 表格
  const rawRmse = rmse(noisy, clean);
  let html = '<table class="rmse-table"><tr><th>滤波器</th><th>RMSE</th><th>改善</th></tr>';
  html += `<tr><td>含噪信号</td><td>${rawRmse.toFixed(4)}</td>
           <td><span class="badge badge-raw">基准</span></td></tr>`;
  let bestRmse = Infinity, bestName = '';
  names.forEach(name => {
    const r = rmse(outputs[name], clean);
    if (r < bestRmse) { bestRmse = r; bestName = name; }
  });
  names.forEach(name => {
    const r = rmse(outputs[name], clean);
    const pct = ((rawRmse - r) / rawRmse * 100).toFixed(1);
    const isBest = name === bestName;
    html += `<tr>
      <td>${name}</td>
      <td>${r.toFixed(4)}</td>
      <td>${isBest
        ? `<span class="badge badge-best">↓${pct}% 最优</span>`
        : `↓${pct}%`}</td>
    </tr>`;
  });
  html += '</table>';
  document.getElementById(id.replace('c', 'rmse')).innerHTML = html;
}

const d = DATA;
makeChart('c1', d.t, d.clean1, d.noisy1, d.out1);
makeChart('c2', d.t, d.clean2, d.noisy2, d.out2);
makeChart('c3', d.t, d.base3,  d.noisy3, d.out3);
</script>
</body>
</html>
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default="filter_viz.html")
    args = parser.parse_args()

    t, clean1, noisy1, out1, clean2, noisy2, out2, base3, noisy3, out3 = build_datasets()

    data = {
        "t":      t,
        "clean1": [round(x, 4) for x in clean1],
        "noisy1": [round(x, 4) for x in noisy1],
        "out1":   {k: [round(x, 4) for x in v] for k, v in out1.items()},
        "clean2": [round(x, 4) for x in clean2],
        "noisy2": [round(x, 4) for x in noisy2],
        "out2":   {k: [round(x, 4) for x in v] for k, v in out2.items()},
        "base3":  [round(x, 4) for x in base3],
        "noisy3": [round(x, 4) for x in noisy3],
        "out3":   {k: [round(x, 4) for x in v] for k, v in out3.items()},
    }

    html = HTML_TEMPLATE.replace("__DATA__", json.dumps(data, ensure_ascii=False))
    out_path = os.path.join(os.path.dirname(__file__), "..", args.output)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(html)

    print(f"已生成：{os.path.abspath(out_path)}")
    print("用浏览器打开该文件即可查看。")


if __name__ == "__main__":
    main()
