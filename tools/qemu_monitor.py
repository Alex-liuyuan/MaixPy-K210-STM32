#!/usr/bin/env python3
"""
tools/qemu_monitor.py
启动 QEMU 仿真，实时读取 semihosting 输出，通过 WebSocket 推送到浏览器。

用法：
  python3 tools/qemu_monitor.py [--elf build/sim/SYSU_AIOTOS_sim.elf] [--port 8765]
  然后浏览器打开 http://localhost:8766
"""

import argparse
import asyncio
import json
import os
import re
import subprocess
import sys
import threading
import time
import http.server
import socketserver
import webbrowser
from pathlib import Path

# ── 内嵌前端 HTML ─────────────────────────────────────────────────────────────

DASHBOARD_HTML = r"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<title>RT-Thread QEMU 实时监控</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: 'Segoe UI', system-ui, sans-serif;
         background: #0f1117; color: #e2e8f0; min-height: 100vh; }

  header { display: flex; align-items: center; justify-content: space-between;
           padding: 16px 28px; border-bottom: 1px solid #1e2535; }
  header h1 { font-size: 1.1rem; font-weight: 600; color: #7dd3fc; }
  .status { display: flex; align-items: center; gap: 8px; font-size: 0.82rem; }
  .dot { width: 8px; height: 8px; border-radius: 50%; background: #ef4444; }
  .dot.connected { background: #4ade80; animation: pulse 1.5s infinite; }
  @keyframes pulse {
    0%,100% { opacity: 1; } 50% { opacity: 0.4; }
  }

  .grid { display: grid; grid-template-columns: 1fr 1fr;
          gap: 16px; padding: 20px 28px; }
  @media (max-width: 860px) { .grid { grid-template-columns: 1fr; } }

  .card { background: #161b27; border: 1px solid #1e2535;
          border-radius: 10px; padding: 16px; }
  .card h2 { font-size: 0.85rem; color: #64748b;
             margin-bottom: 12px; text-transform: uppercase;
             letter-spacing: 0.05em; }
  .card-full { grid-column: 1 / -1; }
  canvas { max-height: 220px; }

  /* 串口日志 */
  #log { font-family: 'Cascadia Code', 'Fira Code', monospace;
         font-size: 0.78rem; line-height: 1.6;
         background: #0a0d14; border-radius: 6px;
         padding: 10px 12px; height: 180px;
         overflow-y: auto; color: #4ade80; }
  #log .ts  { color: #334155; margin-right: 6px; }
  #log .err { color: #f87171; }

  /* 统计卡片 */
  .stats { display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; }
  .stat  { background: #0f1420; border-radius: 8px; padding: 12px 14px; }
  .stat .val { font-size: 1.5rem; font-weight: 700; color: #38bdf8; }
  .stat .lbl { font-size: 0.72rem; color: #475569; margin-top: 2px; }

  /* 滤波控制 */
  .controls { display: flex; gap: 10px; flex-wrap: wrap; margin-bottom: 12px; }
  .ctrl-btn { padding: 5px 14px; border-radius: 6px; border: 1px solid #1e2535;
              background: #1a2030; color: #94a3b8; cursor: pointer;
              font-size: 0.8rem; transition: all .15s; }
  .ctrl-btn:hover  { border-color: #38bdf8; color: #38bdf8; }
  .ctrl-btn.active { background: #0c2a3f; border-color: #38bdf8; color: #38bdf8; }
</style>
</head>
<body>

<header>
  <h1>🔴 RT-Thread QEMU 实时监控</h1>
  <div class="status">
    <div class="dot" id="dot"></div>
    <span id="status-text">未连接</span>
  </div>
</header>

<div class="grid">

  <!-- 统计 -->
  <div class="card card-full">
    <div class="stats">
      <div class="stat">
        <div class="val" id="s-count">0</div>
        <div class="lbl">收到消息数</div>
      </div>
      <div class="stat">
        <div class="val" id="s-fps">—</div>
        <div class="lbl">消息/秒</div>
      </div>
      <div class="stat">
        <div class="val" id="s-uptime">0s</div>
        <div class="lbl">运行时长</div>
      </div>
      <div class="stat">
        <div class="val" id="s-last">—</div>
        <div class="lbl">最新值</div>
      </div>
    </div>
  </div>

  <!-- 实时曲线 -->
  <div class="card">
    <h2>实时数值曲线</h2>
    <div class="controls">
      <button class="ctrl-btn active" onclick="setFilter('raw')">原始</button>
      <button class="ctrl-btn" onclick="setFilter('ma')">滑动平均</button>
      <button class="ctrl-btn" onclick="setFilter('lpf')">低通</button>
      <button class="ctrl-btn" onclick="setFilter('kalman')">卡尔曼</button>
    </div>
    <canvas id="chart-rt"></canvas>
  </div>

  <!-- 滤波对比 -->
  <div class="card">
    <h2>滤波器实时对比</h2>
    <canvas id="chart-cmp"></canvas>
  </div>

  <!-- 串口日志 -->
  <div class="card card-full">
    <h2>串口原始输出</h2>
    <div id="log"></div>
  </div>

</div>

<script>
// ── WebSocket ────────────────────────────────────────────────────────────────
const WS_PORT = __WS_PORT__;
const MAX_PTS = 120;

let ws, startTime = null, msgCount = 0, lastMsgTime = null;
let activeFilter = 'raw';

// ── 滤波器状态 ────────────────────────────────────────────────────────────────
const filters = {
  ma:     { buf: [], w: 8 },
  lpf:    { y: null, alpha: 0.2 },
  kalman: { x: null, p: 1.0, q: 1e-3, r: 0.5 },
};

function filterMA(x) {
  const f = filters.ma;
  f.buf.push(x);
  if (f.buf.length > f.w) f.buf.shift();
  return f.buf.reduce((a, b) => a + b, 0) / f.buf.length;
}
function filterLPF(x) {
  const f = filters.lpf;
  f.y = f.y === null ? x : f.alpha * x + (1 - f.alpha) * f.y;
  return f.y;
}
function filterKalman(x) {
  const f = filters.kalman;
  if (f.x === null) { f.x = x; return x; }
  const pPred = f.p + f.q;
  const k = pPred / (pPred + f.r);
  f.x = f.x + k * (x - f.x);
  f.p = (1 - k) * pPred;
  return f.x;
}

// ── Chart.js 初始化 ───────────────────────────────────────────────────────────
const CHART_OPTS = {
  animation: false,
  responsive: true,
  plugins: {
    legend: { labels: { color: '#94a3b8', font: { size: 11 }, boxWidth: 10 } },
    tooltip: { backgroundColor: '#1e2535', titleColor: '#94a3b8',
               bodyColor: '#cbd5e1' },
  },
  scales: {
    x: { ticks: { color: '#475569', maxTicksLimit: 8 },
         grid: { color: '#1a2030' } },
    y: { ticks: { color: '#475569' },
         grid: { color: '#1a2030' } },
  },
};

function makeDataset(label, color, dash) {
  return {
    label, borderColor: color, borderWidth: 2,
    pointRadius: 0, tension: 0.3, data: [],
    borderDash: dash || [],
  };
}

// 实时单曲线图
const ctxRT  = document.getElementById('chart-rt').getContext('2d');
const chartRT = new Chart(ctxRT, {
  type: 'line',
  data: { labels: [], datasets: [makeDataset('数值', '#38bdf8')] },
  options: { ...CHART_OPTS },
});

// 滤波对比图
const ctxCmp = document.getElementById('chart-cmp').getContext('2d');
const chartCmp = new Chart(ctxCmp, {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      makeDataset('原始',     '#ef4444', [3,2]),
      makeDataset('滑动平均', '#38bdf8'),
      makeDataset('低通',     '#a78bfa'),
      makeDataset('卡尔曼',   '#34d399'),
    ],
  },
  options: { ...CHART_OPTS },
});

function pushPoint(label, raw) {
  const ma     = filterMA(raw);
  const lpf    = filterLPF(raw);
  const kalman = filterKalman(raw);

  const filtered = { raw, ma, lpf, kalman };
  const display  = filtered[activeFilter];

  // 实时图
  chartRT.data.labels.push(label);
  chartRT.data.datasets[0].data.push(display);
  if (chartRT.data.labels.length > MAX_PTS) {
    chartRT.data.labels.shift();
    chartRT.data.datasets[0].data.shift();
  }
  chartRT.update('none');

  // 对比图
  chartCmp.data.labels.push(label);
  [raw, ma, lpf, kalman].forEach((v, i) => {
    chartCmp.data.datasets[i].data.push(v);
    if (chartCmp.data.datasets[i].data.length > MAX_PTS)
      chartCmp.data.datasets[i].data.shift();
  });
  if (chartCmp.data.labels.length > MAX_PTS) chartCmp.data.labels.shift();
  chartCmp.update('none');
}

// ── 滤波切换 ──────────────────────────────────────────────────────────────────
function setFilter(name) {
  activeFilter = name;
  document.querySelectorAll('.ctrl-btn').forEach(b => b.classList.remove('active'));
  event.target.classList.add('active');
  chartRT.data.datasets[0].label =
    { raw:'原始', ma:'滑动平均', lpf:'低通', kalman:'卡尔曼' }[name];
  chartRT.update('none');
}

// ── 串口日志 ──────────────────────────────────────────────────────────────────
const logEl = document.getElementById('log');
function appendLog(text, isErr) {
  const ts = new Date().toLocaleTimeString('zh-CN', { hour12: false });
  const div = document.createElement('div');
  div.innerHTML = `<span class="ts">[${ts}]</span>`
    + `<span class="${isErr ? 'err' : ''}">${escHtml(text)}</span>`;
  logEl.appendChild(div);
  if (logEl.children.length > 200) logEl.removeChild(logEl.firstChild);
  logEl.scrollTop = logEl.scrollHeight;
}
function escHtml(s) {
  return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

// ── 统计更新 ──────────────────────────────────────────────────────────────────
setInterval(() => {
  if (!startTime) return;
  const up = Math.floor((Date.now() - startTime) / 1000);
  document.getElementById('s-uptime').textContent =
    up >= 60 ? `${Math.floor(up/60)}m${up%60}s` : `${up}s`;

  if (lastMsgTime) {
    const fps = msgCount / ((Date.now() - startTime) / 1000);
    document.getElementById('s-fps').textContent = fps.toFixed(1);
  }
}, 1000);

// ── WebSocket 连接 ────────────────────────────────────────────────────────────
function connect() {
  ws = new WebSocket(`ws://localhost:${WS_PORT}`);

  ws.onopen = () => {
    document.getElementById('dot').classList.add('connected');
    document.getElementById('status-text').textContent = '已连接 QEMU';
    startTime = Date.now();
    appendLog('WebSocket 已连接');
  };

  ws.onmessage = e => {
    const msg = JSON.parse(e.data);
    msgCount++;
    lastMsgTime = Date.now();
    document.getElementById('s-count').textContent = msgCount;

    if (msg.type === 'serial') {
      appendLog(msg.text);
      // 尝试从文本中提取数字
      const nums = msg.text.match(/-?\d+(\.\d+)?/g);
      if (nums) {
        const val = parseFloat(nums[nums.length - 1]);
        if (!isNaN(val)) {
          const ts = new Date().toLocaleTimeString('zh-CN',
            { hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit' });
          pushPoint(ts, val);
          document.getElementById('s-last').textContent = val.toFixed(2);
        }
      }
    } else if (msg.type === 'error') {
      appendLog(msg.text, true);
    }
  };

  ws.onclose = () => {
    document.getElementById('dot').classList.remove('connected');
    document.getElementById('status-text').textContent = '连接断开，3s 后重连…';
    appendLog('连接断开', true);
    setTimeout(connect, 3000);
  };

  ws.onerror = () => {
    document.getElementById('status-text').textContent = 'WebSocket 错误';
  };
}

connect();
</script>
</body>
</html>
"""

# ── WebSocket 服务器 ───────────────────────────────────────────────────────────

clients = set()

async def ws_handler(websocket):
    clients.add(websocket)
    try:
        await websocket.wait_closed()
    finally:
        clients.discard(websocket)

async def broadcast(msg: dict):
    if not clients:
        return
    data = json.dumps(msg, ensure_ascii=False)
    await asyncio.gather(
        *[c.send(data) for c in list(clients)],
        return_exceptions=True,
    )

# ── QEMU 进程管理 ─────────────────────────────────────────────────────────────

def run_qemu(elf_path: str, loop: asyncio.AbstractEventLoop):
    """在子线程里启动 QEMU，读取 semihosting 输出并推送到 WebSocket"""
    cmd = [
        "qemu-system-arm",
        "-machine", "olimex-stm32-h405",
        "-nographic",
        "-semihosting",
        "-semihosting-config", "enable=on,target=native",
        "-kernel", elf_path,
        "-no-reboot",
    ]
    print(f"[QEMU] 启动: {' '.join(cmd)}")

    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
    except FileNotFoundError:
        asyncio.run_coroutine_threadsafe(
            broadcast({"type": "error", "text": "找不到 qemu-system-arm，请先安装"}),
            loop,
        )
        return

    for line in proc.stdout:
        line = line.rstrip()
        if not line:
            continue
        print(f"[QEMU] {line}")
        asyncio.run_coroutine_threadsafe(
            broadcast({"type": "serial", "text": line}),
            loop,
        )

    proc.wait()
    asyncio.run_coroutine_threadsafe(
        broadcast({"type": "error", "text": f"QEMU 已退出（code={proc.returncode}）"}),
        loop,
    )

# ── HTTP 服务器（提供 dashboard.html）────────────────────────────────────────

def serve_html(html: str, http_port: int):
    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(html.encode())

        def log_message(self, *_):
            pass  # 静默

    with socketserver.TCPServer(("", http_port), Handler) as httpd:
        httpd.serve_forever()

# ── 主入口 ────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="QEMU RT-Thread 实时监控")
    parser.add_argument(
        "--elf",
        default=str(Path(__file__).parent.parent / "build/sim/SYSU_AIOTOS_sim.elf"),
    )
    parser.add_argument("--ws-port",   type=int, default=8765)
    parser.add_argument("--http-port", type=int, default=8766)
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args()

    if not os.path.exists(args.elf):
        print(f"[错误] ELF 文件不存在：{args.elf}")
        print("请先编译：cmake --build build/sim --parallel 4")
        sys.exit(1)

    # 注入 WebSocket 端口到 HTML
    html = DASHBOARD_HTML.replace("__WS_PORT__", str(args.ws_port))

    # 启动 HTTP 服务
    t_http = threading.Thread(
        target=serve_html, args=(html, args.http_port), daemon=True
    )
    t_http.start()
    print(f"[HTTP] 仪表盘：http://localhost:{args.http_port}")

    # 启动 asyncio 事件循环
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

    # 启动 QEMU 线程
    t_qemu = threading.Thread(
        target=run_qemu, args=(args.elf, loop), daemon=True
    )
    t_qemu.start()

    # 打开浏览器
    if not args.no_browser:
        threading.Timer(1.0, lambda: webbrowser.open(
            f"http://localhost:{args.http_port}"
        )).start()

    # 启动 WebSocket 服务
    async def serve():
        try:
            import websockets
            async with websockets.serve(ws_handler, "localhost", args.ws_port):
                print(f"[WS]   WebSocket：ws://localhost:{args.ws_port}")
                await asyncio.Future()  # 永久运行
        except ImportError:
            print("[错误] 缺少 websockets 库，请运行：pip install websockets")
            # 降级：只保留 HTTP + QEMU，不推送实时数据
            await asyncio.Future()

    try:
        loop.run_until_complete(serve())
    except KeyboardInterrupt:
        print("\n[退出] 已停止")
    finally:
        # 取消所有挂起的协程，避免 GeneratorExit 警告
        pending = asyncio.all_tasks(loop)
        for task in pending:
            task.cancel()
        if pending:
            loop.run_until_complete(asyncio.gather(*pending, return_exceptions=True))
        loop.close()


if __name__ == "__main__":
    main()
