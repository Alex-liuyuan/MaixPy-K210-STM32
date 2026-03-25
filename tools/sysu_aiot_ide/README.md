# SYSU_AIOT_IDE

**中山大学 AIoT 集成开发环境** — 基于 Web 的 MaixPy-K210-STM32 开发工具

## 功能特性

### 核心功能
- ✅ **代码编辑器**
  - Python 语法高亮（关键字、内置函数、MaixPy API）
  - 智能代码补全（Ctrl+Space 触发）
  - 自动缩进、Tab 键缩进（4 空格）
  - 注释切换（Ctrl+/）
  - 行号显示、光标位置提示
  - 多标签页支持

- ✅ **文件管理**
  - 文件树浏览（支持展开/折叠）
  - 文件读取、保存、重命名、删除
  - 右键菜单（新建、重命名、删除）
  - 示例代码快速加载

- ✅ **代码执行**
  - 一键运行（F5）、停止（F6）
  - 平台选择（Linux 模拟 / STM32 / K210）
  - 实时终端输出（stdout/stderr）
  - 运行时间统计

- ✅ **图像预览**
  - 实时图像捕获（单帧 / 连续预览）
  - FPS 显示
  - 截图保存（JPEG 格式）
  - 动态渐变演示帧

- ✅ **变量监视**
  - 自动捕获 `[WATCH] var = value` 格式输出
  - 实时更新变量值
  - 手动添加/删除监视变量

- ✅ **串口通信**
  - 串口设备自动检测
  - 连接/断开控制
  - 数据发送/接收
  - 实时 RX 数据显示

- ✅ **设备信息**
  - 平台显示（Linux / STM32 / K210）
  - 版本信息（1.0.0）
  - 运行状态（空闲 / 运行中）
  - 运行时长统计

### 快捷键

| 快捷键 | 功能 |
|--------|------|
| `F5` | 运行代码 |
| `F6` | 停止运行 |
| `Ctrl+S` | 保存文件 |
| `Ctrl+N` | 新建文件 |
| `Ctrl+O` | 打开文件 |
| `Ctrl+/` | 切换注释 |
| `Ctrl+Space` | 代码补全 |
| `Tab` | 缩进（4 空格）|
| `Enter` | 自动缩进 |

## 快速开始

### 1. 安装依赖

```bash
pip3 install websockets numpy pillow pyserial
```

### 2. 启动服务器

```bash
# 方式 1：直接运行
python3 tools/sysu_aiot_ide/server.py

# 方式 2：使用启动脚本（支持自定义端口）
python3 tools/sysu_aiot_ide/launch.py --port 8765 --ws-port 8766

# 方式 3：自定义端口（环境变量）
MAIXVISION_HTTP_PORT=9000 MAIXVISION_WS_PORT=9001 python3 tools/sysu_aiot_ide/server.py
```

### 3. 打开浏览器

访问 `http://localhost:8765`

## 架构设计

```
tools/sysu_aiot_ide/
├── server.py              # 后端服务器（HTTP + WebSocket）
├── launch.py              # 启动脚本
├── static/
│   ├── index.html         # 主界面
│   ├── style.css          # 样式表（2640+ 行）
│   ├── app.js             # 前端主逻辑（862 行）
│   └── editor.js          # 语法高亮 + 代码补全（482 行）
└── README.md              # 本文档
```

### 后端（server.py）

- **HTTP 服务**（端口 8765）
  - 静态文件服务（HTML/CSS/JS）
  - RESTful API（文件 CRUD、示例列表、串口列表）

- **WebSocket 服务**（端口 8766）
  - 代码执行引擎（子进程 + 实时输出流）
  - 串口通信（pyserial）
  - 图像捕获（numpy + PIL）
  - 双向实时通信

### 前端（app.js + editor.js）

- **app.js**：主逻辑
  - WebSocket 连接管理（自动重连）
  - 文件管理（CRUD + 标签页）
  - 代码执行控制
  - 图像预览（Canvas 渲染）
  - 变量监视（正则解析）
  - 串口通信
  - 终端输出

- **editor.js**：编辑器增强
  - 语法高亮（状态机 + 正则）
  - 代码补全（35+ API + 模板）
  - 光标坐标计算

## API 文档

### HTTP API

#### 文件管理

```http
GET  /api/files?dir=.                    # 列出目录
GET  /api/file?path=examples/hello.py    # 读取文件
POST /api/file/save                       # 保存文件
     Body: {"path": "test.py", "content": "print(1)"}
POST /api/file/rename                     # 重命名文件
     Body: {"old_path": "a.py", "new_path": "b.py"}
POST /api/file/delete                     # 删除文件
     Body: {"path": "test.py"}
```

#### 其他

```http
GET  /api/examples                        # 获取示例列表
GET  /api/serial/ports                    # 获取串口列表
POST /api/run/stop                        # 停止运行
```

### WebSocket API

```javascript
// 连接
ws = new WebSocket('ws://localhost:8766');

// 运行代码
ws.send(JSON.stringify({
  action: 'run',
  code: 'print("hello")',
  platform: 'linux'  // 'linux' | 'stm32' | 'k210'
}));

// 停止运行
ws.send(JSON.stringify({ action: 'stop' }));

// 图像捕获
ws.send(JSON.stringify({
  action: 'capture',
  width: 320,
  height: 240
}));

// 串口连接
ws.send(JSON.stringify({
  action: 'serial_connect',
  port: '/dev/ttyUSB0',
  baudrate: 115200
}));

// 串口发送
ws.send(JSON.stringify({
  action: 'serial_write',
  data: 'hello\n'
}));

// 心跳
ws.send(JSON.stringify({ action: 'ping' }));
```

### WebSocket 消息类型

```javascript
// 服务器 → 客户端
{ type: 'pong' }                          // 心跳响应
{ type: 'run_start' }                     // 开始运行
{ type: 'run_end', code: 0 }              // 运行结束
{ type: 'stdout', data: 'hello\n' }       // 标准输出
{ type: 'stderr', data: 'error\n' }       // 标准错误
{ type: 'frame', data: '<base64_jpeg>' }  // 图像帧
{ type: 'serial_rx', data: 'data\n' }     // 串口接收
{ type: 'serial_status', connected: true, msg: '...' }  // 串口状态
```

## 代码补全

### 支持的 API

- **maix 核心**：`camera`, `display`, `app`, `time`, `GPIO`, `platform`, `version`
- **错误处理**：`err.check_raise`, `err.HardwareError`, `err.ERR_NONE`
- **引脚复用**：`pinmap.set_pin_function`, `pinmap.get_pin_function`
- **神经网络**：`nn.NN`, `nn.Classifier`, `nn.Detector`, `nn.YOLOv5`, `nn.YOLOv8`
- **外设**：`uart.UART`, `spi.SPI`, `i2c.I2C`, `adc.ADC`, `pwm.PWM`
- **图像处理**：`Image.draw_string`, `Image.draw_rectangle`, `Image.find_blobs`

### 代码模板

- 主循环模板（摄像头 + 显示）
- GPIO 闪烁模板
- AI 分类模板
- 变量监视模板（`[WATCH] var = value`）

## 变量监视

在代码中使用以下格式输出变量，IDE 会自动捕获并显示：

```python
print(f"[WATCH] fps = {fps:.1f}")
print(f"[WATCH] temp = {temp}")
print(f"[WATCH] status = OK")
```

## 技术栈

### 后端
- Python 3.12+
- 标准库：`asyncio`, `http.server`, `subprocess`, `threading`
- 第三方库：`websockets`, `numpy`, `pillow`, `pyserial`

### 前端
- 纯原生 JavaScript（无框架）
- HTML5 + CSS3
- WebSocket API
- Canvas API

## 浏览器兼容性

- ✅ Chrome 90+
- ✅ Firefox 88+
- ✅ Edge 90+
- ✅ Safari 14+

## 性能指标

- **启动时间**：< 3 秒
- **代码执行延迟**：< 100ms
- **图像预览帧率**：10 FPS（连续模式）
- **WebSocket 延迟**：< 50ms
- **内存占用**：< 100MB（服务器 + 浏览器）

## 故障排除

### 端口被占用

```bash
# 查看占用端口的进程
lsof -i :8765
lsof -i :8766

# 杀死进程
kill -9 <PID>

# 或使用 fuser
fuser -k 8765/tcp 8766/tcp
```

### WebSocket 连接失败

1. 检查防火墙设置
2. 确认服务器正在运行
3. 检查浏览器控制台错误信息
4. 尝试刷新页面（Ctrl+F5）

### 串口无法连接

1. 检查串口设备权限：`sudo chmod 666 /dev/ttyUSB0`
2. 确认串口未被其他程序占用
3. 检查波特率是否正确
4. 尝试重新插拔设备

### 代码运行无输出

1. 检查平台选择是否正确
2. 确认代码语法无误
3. 查看终端是否有错误信息
4. 尝试运行简单的 `print("test")` 测试

## 开发指南

### 添加新的 API 补全

编辑 `static/editor.js`，在 `COMPLETIONS` 数组中添加：

```javascript
{
  label: 'your_api(param1, param2)',
  kind: 'api',
  insert: 'your_api(param1, param2)'
}
```

### 添加新的语法高亮

编辑 `static/editor.js`，在 `MAIX_API` 集合中添加：

```javascript
const MAIX_API = new Set([
  // ... 现有 API
  'YourNewAPI',
]);
```

### 自定义主题

编辑 `static/style.css`，修改 `:root` 变量：

```css
:root {
  --bg0:      #1e1e2e;   /* 最深背景 */
  --accent:   #7c6af7;   /* 主色调 */
  --run:      #3dba6e;   /* 运行绿 */
  /* ... */
}
```

## 许可证

Apache 2.0 License

## 贡献者

- MaixPy-K210-STM32 项目团队
- 中山大学 AIoT 实验室

## 更新日志

### v1.0.0 (2024-03)
- ✅ 初始版本发布
- ✅ 完整的 Web IDE 功能
- ✅ 支持 K210 + STM32 + Linux 三平台
- ✅ 语法高亮 + 代码补全
- ✅ 图像预览 + 变量监视
- ✅ 串口通信 + 文件管理
- ✅ 22 项端到端测试全部通过

## 相关链接

- [MaixPy-K210-STM32 项目主页](../../README.md)
- [MaixPy 官方文档](https://wiki.sipeed.com/maixpy/)
- [K210 数据手册](https://canaan.io/product/kendryteai)
- [STM32F407 参考手册](https://www.st.com/en/microcontrollers-microprocessors/stm32f407.html)
