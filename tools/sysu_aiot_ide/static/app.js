/**
 * SYSU_AIOT_IDE — 前端主逻辑
 * 纯原生 JS，无依赖框架
 */

const WS_URL  = `ws://${location.hostname}:8766`;
const API     = (path) => `http://${location.hostname}:8765${path}`;

// ══════════════════════════════════════════════════════════════════
// 状态
// ══════════════════════════════════════════════════════════════════
const state = {
  ws:          null,
  wsReady:     false,
  running:     false,
  dirty:       false,
  currentFile: null,       // 当前文件相对路径
  tabs:        [],         // [{path, name, content, dirty}]
  activeTab:   -1,
  streaming:   false,
  streamTimer: null,
  fpsFrames:   0,
  fpsLast:     Date.now(),
  runStart:    0,
  runtimeTimer:null,
  watchVars:   {},         // {name: value}
};

// ══════════════════════════════════════════════════════════════════
// DOM 引用
// ══════════════════════════════════════════════════════════════════
const $ = (id) => document.getElementById(id);
const editor       = $('code-editor');
const lineNums     = $('line-numbers');
const terminal     = $('terminal');
const tabsBar      = $('editor-tabs');
const fileTree     = $('file-tree');
const examplesList = $('examples-list');
const previewCanvas= $('preview-canvas');
const previewOverlay=$('preview-overlay');
const ctx          = previewCanvas.getContext('2d');

// ══════════════════════════════════════════════════════════════════
// WebSocket
// ══════════════════════════════════════════════════════════════════
function wsConnect() {
  try {
    state.ws = new WebSocket(WS_URL);
  } catch(e) { scheduleReconnect(); return; }

  state.ws.onopen = () => {
    state.wsReady = true;
    setWsStatus(true);
    termInfo('已连接到 SYSU_AIOT_IDE 服务器');
    state.ws.send(JSON.stringify({action:'ping'}));
  };

  state.ws.onclose = () => {
    state.wsReady = false;
    setWsStatus(false);
    if (state.running) stopRunUI();
    scheduleReconnect();
  };

  state.ws.onerror = () => {
    state.wsReady = false;
    setWsStatus(false);
  };

  state.ws.onmessage = (ev) => {
    let msg;
    try { msg = JSON.parse(ev.data); } catch { return; }
    handleWsMsg(msg);
  };
}

function scheduleReconnect() {
  setTimeout(wsConnect, 2000);
}

function wsSend(obj) {
  if (state.wsReady) state.ws.send(JSON.stringify(obj));
}

function handleWsMsg(msg) {
  switch (msg.type) {
    case 'stdout':
      termAppend(msg.data, 'stdout');
      parseWatchVars(msg.data);
      break;
    case 'stderr':
      termAppend(msg.data, 'stderr');
      break;
    case 'run_start':
      startRunUI();
      break;
    case 'run_end':
      stopRunUI();
      if (msg.code === 0)
        termInfo(`✓ 程序正常退出 (耗时 ${((Date.now()-state.runStart)/1000).toFixed(2)}s)`);
      else if (msg.code !== -1)
        termAppend(`✗ 程序退出，返回码: ${msg.code}\n`, 'stderr');
      break;
    case 'frame':
      if (msg.data) renderFrame(msg.data);
      break;
    case 'serial_rx':
      termAppend(msg.data, 'stdout');
      parseWatchVars(msg.data);
      break;
    case 'serial_status':
      termInfo(msg.msg || (msg.connected ? '串口已连接' : '串口已断开'));
      if (msg.connected) {
        $('btn-send-serial').disabled = false;
        $('terminal-input').disabled  = false;
        $('btn-serial-connect').textContent = '断开';
        state.serialConnected = true;
      } else {
        $('btn-send-serial').disabled = true;
        $('terminal-input').disabled  = true;
        $('btn-serial-connect').textContent = '连接';
        state.serialConnected = false;
      }
      break;
    case 'pong':
      break;
  }
}

// ══════════════════════════════════════════════════════════════════
// 运行 / 停止
// ══════════════════════════════════════════════════════════════════
function runCode() {
  if (!state.wsReady) { termInfo('⚠ 未连接到服务器'); return; }
  const code     = editor.value;
  const platform = $('platform-select').value;
  terminal.innerHTML = '';
  termInfo(`▶ 运行中 [平台: ${platform}]`);
  wsSend({ action: 'run', code, platform });
}

function stopCode() {
  wsSend({ action: 'stop' });
  stopRunUI();
}

function runTests() {
  if (!state.wsReady) { termInfo('⚠ 未连接到服务器'); return; }
  terminal.innerHTML = '';
  termInfo('🧪 运行单元测试...');
  wsSend({ action: 'run_tests' });
}

function startRunUI() {
  state.running  = true;
  state.runStart = Date.now();
  $('btn-run').disabled  = true;
  $('btn-stop').disabled = false;
  if ($('btn-test')) $('btn-test').disabled = true;
  // 启用 stdin 输入
  if ($('btn-send-stdin')) $('btn-send-stdin').disabled = false;
  if ($('terminal-input')) $('terminal-input').disabled = false;
  $('di-status').textContent = '运行中';
  $('di-status').style.color = 'var(--run)';
  $('sb-msg').textContent = '运行中...';
  state.runtimeTimer = setInterval(() => {
    const s = ((Date.now() - state.runStart) / 1000).toFixed(1);
    $('di-runtime').textContent = s + 's';
  }, 200);
}

function stopRunUI() {
  state.running  = false;
  $('btn-run').disabled  = false;
  $('btn-stop').disabled = true;
  if ($('btn-test')) $('btn-test').disabled = false;
  // 禁用 stdin（串口未连接时）
  if (!state.serialConnected) {
    if ($('btn-send-stdin'))  $('btn-send-stdin').disabled  = true;
    if ($('terminal-input'))  $('terminal-input').disabled  = true;
    if ($('btn-send-serial')) $('btn-send-serial').disabled = true;
  }
  $('di-status').textContent = '空闲';
  $('di-status').style.color = '';
  $('sb-msg').textContent = '就绪';
  clearInterval(state.runtimeTimer);
}

// ══════════════════════════════════════════════════════════════════
// 终端
// ══════════════════════════════════════════════════════════════════
function termAppend(text, cls) {
  const span = document.createElement('span');
  span.className = `term-${cls}`;
  span.textContent = text;
  terminal.appendChild(span);
  terminal.scrollTop = terminal.scrollHeight;
}

function termInfo(text) {
  termAppend(text + '\n', 'info');
}

// 从 stdout 解析 key=value 格式的变量监视
function parseWatchVars(text) {
  const re = /\[WATCH\]\s*(\w+)\s*=\s*(.+)/g;
  let m;
  while ((m = re.exec(text)) !== null) {
    state.watchVars[m[1]] = m[2].trim();
  }
  renderWatchList();
}

// ══════════════════════════════════════════════════════════════════
// 图像预览
// ══════════════════════════════════════════════════════════════════
function renderFrame(b64) {
  const img = new Image();
  img.onload = () => {
    previewCanvas.width  = img.width;
    previewCanvas.height = img.height;
    ctx.drawImage(img, 0, 0);
    previewOverlay.classList.add('hidden');
    $('preview-info').textContent =
      `${img.width} × ${img.height} · JPEG`;
    // FPS 计算
    state.fpsFrames++;
    const now = Date.now();
    if (now - state.fpsLast >= 1000) {
      $('fps-badge').textContent =
        (state.fpsFrames * 1000 / (now - state.fpsLast)).toFixed(1);
      state.fpsFrames = 0;
      state.fpsLast   = now;
    }
    // 保存最后一帧 b64（用于截图下载）
    state.lastFrameB64 = b64;
  };
  img.src = 'data:image/jpeg;base64,' + b64;
}

function captureOnce() {
  wsSend({ action: 'capture', width: 320, height: 240 });
}

function toggleStream() {
  if (state.streaming) {
    clearInterval(state.streamTimer);
    state.streaming = false;
    $('btn-stream').textContent = '▶';
    $('btn-stream').title = '连续预览';
  } else {
    state.streaming = true;
    $('btn-stream').textContent = '⏹';
    $('btn-stream').title = '停止预览';
    state.streamTimer = setInterval(captureOnce, 100);
  }
}

function saveSnapshot() {
  if (!state.lastFrameB64) { termInfo('⚠ 暂无图像可保存'); return; }
  const a = document.createElement('a');
  a.href     = 'data:image/jpeg;base64,' + state.lastFrameB64;
  a.download = `snapshot_${Date.now()}.jpg`;
  a.click();
  termInfo('📷 截图已保存');
}

// ══════════════════════════════════════════════════════════════════
// 编辑器 — 行号 & Tab 键
// ══════════════════════════════════════════════════════════════════
function updateLineNumbers() {
  const lines = editor.value.split('\n').length;
  let html = '';
  for (let i = 1; i <= lines; i++) html += i + '\n';
  lineNums.textContent = html;
  // 同步滚动
  lineNums.scrollTop = editor.scrollTop;
}

editor.addEventListener('input', () => {
  updateLineNumbers();
  markDirty();
  updateCursor();
});

editor.addEventListener('scroll', () => {
  lineNums.scrollTop = editor.scrollTop;
});

editor.addEventListener('keydown', (e) => {
  // Tab → 4 spaces
  if (e.key === 'Tab') {
    e.preventDefault();
    const s = editor.selectionStart;
    const v = editor.value;
    editor.value = v.slice(0, s) + '    ' + v.slice(editor.selectionEnd);
    editor.selectionStart = editor.selectionEnd = s + 4;
    updateLineNumbers();
    markDirty();
    return;
  }
  // Auto-indent on Enter
  if (e.key === 'Enter') {
    e.preventDefault();
    const s   = editor.selectionStart;
    const v   = editor.value;
    const lineStart = v.lastIndexOf('\n', s - 1) + 1;
    const line      = v.slice(lineStart, s);
    const indent    = line.match(/^(\s*)/)[1];
    const extra     = line.trimEnd().endsWith(':') ? '    ' : '';
    const ins       = '\n' + indent + extra;
    editor.value = v.slice(0, s) + ins + v.slice(editor.selectionEnd);
    editor.selectionStart = editor.selectionEnd = s + ins.length;
    updateLineNumbers();
    markDirty();
    return;
  }
  // Ctrl+/ → toggle comment
  if ((e.ctrlKey || e.metaKey) && e.key === '/') {
    e.preventDefault();
    toggleComment();
    return;
  }
});

editor.addEventListener('keyup', updateCursor);
editor.addEventListener('click', updateCursor);

function updateCursor() {
  const s = editor.selectionStart;
  const v = editor.value.slice(0, s);
  const lines = v.split('\n');
  const row = lines.length;
  const col = lines[lines.length - 1].length + 1;
  $('sb-cursor').textContent = `行 ${row}, 列 ${col}`;
}

function toggleComment() {
  const s = editor.selectionStart;
  const e2 = editor.selectionEnd;
  const v = editor.value;
  const lineStart = v.lastIndexOf('\n', s - 1) + 1;
  const lineEnd   = v.indexOf('\n', e2);
  const end       = lineEnd === -1 ? v.length : lineEnd;
  const line      = v.slice(lineStart, end);
  let newLine;
  if (line.trimStart().startsWith('# ')) {
    newLine = line.replace(/^(\s*)# /, '$1');
  } else if (line.trimStart().startsWith('#')) {
    newLine = line.replace(/^(\s*)#/, '$1');
  } else {
    const indent = line.match(/^(\s*)/)[1];
    newLine = indent + '# ' + line.slice(indent.length);
  }
  editor.value = v.slice(0, lineStart) + newLine + v.slice(end);
  editor.selectionStart = s + (newLine.length - line.length);
  editor.selectionEnd   = e2 + (newLine.length - line.length);
  updateLineNumbers();
  markDirty();
}

// ══════════════════════════════════════════════════════════════════
// 文件管理
// ══════════════════════════════════════════════════════════════════
function markDirty() {
  state.dirty = true;
  $('file-dirty').textContent = '●';
  if (state.activeTab >= 0) state.tabs[state.activeTab].dirty = true;
  renderTabs();
}

function clearDirty() {
  state.dirty = false;
  $('file-dirty').textContent = '';
  if (state.activeTab >= 0) state.tabs[state.activeTab].dirty = false;
  renderTabs();
}

async function loadFile(relPath) {
  try {
    const r = await fetch(API(`/api/file?path=${encodeURIComponent(relPath)}`));
    const d = await r.json();
    if (d.error) { termInfo('⚠ 无法读取文件: ' + d.error); return; }
    openTab(relPath, d.content);
  } catch(e) {
    termInfo('⚠ 网络错误: ' + e.message);
  }
}

async function saveFile() {
  if (!state.currentFile) { showNewFileModal(); return; }
  const content = editor.value;
  try {
    const r = await fetch(API('/api/file/save'), {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({ path: state.currentFile, content }),
    });
    const d = await r.json();
    if (d.ok) {
      clearDirty();
      termInfo(`💾 已保存: ${state.currentFile}`);
      $('sb-msg').textContent = '已保存';
      setTimeout(() => $('sb-msg').textContent = '就绪', 2000);
    } else {
      termInfo('⚠ 保存失败: ' + d.error);
    }
  } catch(e) {
    termInfo('⚠ 保存失败: ' + e.message);
  }
}

// ══════════════════════════════════════════════════════════════════
// 标签页
// ══════════════════════════════════════════════════════════════════
function openTab(path, content) {
  const name = path.split('/').pop();
  const idx  = state.tabs.findIndex(t => t.path === path);
  if (idx >= 0) {
    switchTab(idx);
    return;
  }
  state.tabs.push({ path, name, content, dirty: false });
  switchTab(state.tabs.length - 1);
}

function switchTab(idx) {
  // 保存当前 tab 内容
  if (state.activeTab >= 0 && state.tabs[state.activeTab]) {
    state.tabs[state.activeTab].content = editor.value;
  }
  state.activeTab    = idx;
  const tab          = state.tabs[idx];
  editor.value       = tab.content;
  state.currentFile  = tab.path;
  state.dirty        = tab.dirty;
  $('current-file').textContent = tab.name;
  $('file-dirty').textContent   = tab.dirty ? '●' : '';
  $('sb-platform').textContent  = $('platform-select').value;
  updateLineNumbers();
  updateCursor();
  renderTabs();
  // 高亮文件树
  document.querySelectorAll('.tree-item').forEach(el => {
    el.classList.toggle('active', el.dataset.path === tab.path);
  });
}

function closeTab(idx) {
  state.tabs.splice(idx, 1);
  if (state.tabs.length === 0) {
    state.activeTab   = -1;
    state.currentFile = null;
    editor.value      = '';
    $('current-file').textContent = '未命名.py';
    $('file-dirty').textContent   = '';
    updateLineNumbers();
  } else {
    const newIdx = Math.min(idx, state.tabs.length - 1);
    state.activeTab = -1;  // 强制重新加载
    switchTab(newIdx);
  }
  renderTabs();
}

function renderTabs() {
  tabsBar.innerHTML = '';
  state.tabs.forEach((tab, i) => {
    const el = document.createElement('div');
    el.className = 'editor-tab' + (i === state.activeTab ? ' active' : '');
    el.innerHTML = `<span>${tab.name}${tab.dirty ? ' ●' : ''}</span>
      <span class="tab-close" data-idx="${i}">×</span>`;
    el.addEventListener('click', (e) => {
      if (e.target.classList.contains('tab-close')) {
        closeTab(parseInt(e.target.dataset.idx));
      } else {
        switchTab(i);
      }
    });
    tabsBar.appendChild(el);
  });
}

// ══════════════════════════════════════════════════════════════════
// 文件树
// ══════════════════════════════════════════════════════════════════

// 右键菜单
let _ctxMenu = null;
function showContextMenu(x, y, items) {
  hideContextMenu();
  _ctxMenu = document.createElement('div');
  _ctxMenu.className = 'ctx-menu';
  _ctxMenu.style.left = x + 'px';
  _ctxMenu.style.top  = y + 'px';
  items.forEach(item => {
    if (item === '-') {
      const sep = document.createElement('div');
      sep.className = 'ctx-sep';
      _ctxMenu.appendChild(sep);
    } else {
      const el = document.createElement('div');
      el.className = 'ctx-item';
      el.textContent = item.label;
      el.addEventListener('mousedown', (e) => { e.preventDefault(); item.action(); hideContextMenu(); });
      _ctxMenu.appendChild(el);
    }
  });
  document.body.appendChild(_ctxMenu);
}
function hideContextMenu() {
  if (_ctxMenu) { _ctxMenu.remove(); _ctxMenu = null; }
}
document.addEventListener('click', hideContextMenu);
document.addEventListener('contextmenu', (e) => {
  if (!e.target.closest('.tree-item')) hideContextMenu();
});

async function deleteFile(path) {
  if (!confirm(`确认删除: ${path}?`)) return;
  try {
    const r = await fetch(API('/api/file/delete'), {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({ path }),
    });
    const d = await r.json();
    if (d.ok) {
      termInfo(`🗑 已删除: ${path}`);
      // 关闭对应标签页
      const idx = state.tabs.findIndex(t => t.path === path);
      if (idx >= 0) closeTab(idx);
      loadFileTree();
    } else {
      termInfo('⚠ 删除失败: ' + d.error);
    }
  } catch(e) { termInfo('⚠ 删除失败: ' + e.message); }
}

async function renameFile(oldPath) {
  const newName = prompt('新文件名:', oldPath.split('/').pop());
  if (!newName || newName === oldPath.split('/').pop()) return;
  const dir     = oldPath.includes('/') ? oldPath.slice(0, oldPath.lastIndexOf('/') + 1) : '';
  const newPath = dir + newName;
  try {
    const r = await fetch(API('/api/file/rename'), {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({ old_path: oldPath, new_path: newPath }),
    });
    const d = await r.json();
    if (d.ok) {
      termInfo(`✏ 已重命名: ${oldPath} → ${newPath}`);
      // 更新标签页
      const tab = state.tabs.find(t => t.path === oldPath);
      if (tab) { tab.path = newPath; tab.name = newName; renderTabs(); }
      loadFileTree();
    } else {
      termInfo('⚠ 重命名失败: ' + d.error);
    }
  } catch(e) { termInfo('⚠ 重命名失败: ' + e.message); }
}

async function loadFileTree(dir = '.', container = fileTree, depth = 0) {
  try {
    const r = await fetch(API(`/api/files?dir=${encodeURIComponent(dir)}`));
    const d = await r.json();
    if (depth === 0) container.innerHTML = '';
    d.entries.forEach(entry => {
      const item = document.createElement('div');
      item.className = 'tree-item' + (entry.type === 'dir' ? ' dir' : '');
      item.dataset.path = entry.path;
      item.style.paddingLeft = (10 + depth * 14) + 'px';
      const icon = entry.type === 'dir' ? '📁'
        : entry.ext === '.py' ? '🐍'
        : entry.ext === '.md' ? '📄'
        : entry.ext === '.json' ? '{}' : '📄';
      item.innerHTML = `<span class="tree-icon">${icon}</span>
        <span class="tree-name">${entry.name}</span>`;

      // 右键菜单
      item.addEventListener('contextmenu', (e) => {
        e.preventDefault();
        const menuItems = entry.type === 'dir'
          ? [
              { label: '📂 在此新建文件', action: () => {
                  const name = prompt('文件名:', 'new_script.py');
                  if (name) openTab(entry.path + '/' + name, getDefaultCode(name));
              }},
              '-',
              { label: '🗑 删除文件夹', action: () => deleteFile(entry.path) },
            ]
          : [
              { label: '📝 打开', action: () => loadFile(entry.path) },
              { label: '✏ 重命名', action: () => renameFile(entry.path) },
              '-',
              { label: '🗑 删除', action: () => deleteFile(entry.path) },
            ];
        showContextMenu(e.clientX, e.clientY, menuItems);
      });

      if (entry.type === 'dir') {
        let expanded = false;
        const children = document.createElement('div');
        children.className = 'tree-children';
        children.style.display = 'none';
        item.addEventListener('click', async () => {
          expanded = !expanded;
          children.style.display = expanded ? 'block' : 'none';
          item.querySelector('.tree-icon').textContent = expanded ? '📂' : '📁';
          if (expanded && children.children.length === 0) {
            await loadFileTree(entry.path, children, depth + 1);
          }
        });
        container.appendChild(item);
        container.appendChild(children);
      } else if (entry.ext === '.py') {
        item.addEventListener('click', () => loadFile(entry.path));
        container.appendChild(item);
      } else {
        container.appendChild(item);
      }
    });
  } catch(e) {
    container.innerHTML = `<div class="tree-item" style="color:var(--stop)">加载失败</div>`;
  }
}

async function loadExamples() {
  try {
    const r = await fetch(API('/api/examples'));
    const d = await r.json();
    examplesList.innerHTML = '';
    d.examples.forEach(ex => {
      const item = document.createElement('div');
      item.className = 'example-item';
      item.innerHTML = `<span class="example-cat">${ex.category}</span>
        <span>${ex.name}</span>`;
      item.addEventListener('click', () => loadFile(ex.path));
      examplesList.appendChild(item);
    });
  } catch(e) {}
}

// ══════════════════════════════════════════════════════════════════
// 串口
// ══════════════════════════════════════════════════════════════════
async function refreshSerialPorts() {
  try {
    const r = await fetch(API('/api/serial/ports'));
    const d = await r.json();
    const sel = $('serial-select');
    sel.innerHTML = '<option value="">-- 选择串口 --</option>';
    d.ports.forEach(p => {
      const opt = document.createElement('option');
      opt.value = p.device;
      opt.textContent = `${p.device} (${p.description})`;
      sel.appendChild(opt);
    });
    if (d.ports.length === 0) {
      termInfo('未发现串口设备');
    }
  } catch(e) {}
}

// ══════════════════════════════════════════════════════════════════
// 变量监视
// ══════════════════════════════════════════════════════════════════
function renderWatchList() {
  const list = $('watch-list');
  const keys = Object.keys(state.watchVars);
  if (keys.length === 0) {
    list.innerHTML = '<div class="watch-hint">运行时自动捕获 print() 输出<br>格式: [WATCH] var = value</div>';
    return;
  }
  list.innerHTML = '';
  keys.forEach(k => {
    const row = document.createElement('div');
    row.className = 'watch-row';
    row.innerHTML = `<span class="watch-key">${k}</span>
      <span class="watch-val">${state.watchVars[k]}</span>
      <span class="watch-del" data-key="${k}">×</span>`;
    row.querySelector('.watch-del').addEventListener('click', (e) => {
      delete state.watchVars[e.target.dataset.key];
      renderWatchList();
    });
    list.appendChild(row);
  });
}

// ══════════════════════════════════════════════════════════════════
// 新建文件 Modal
// ══════════════════════════════════════════════════════════════════
function showNewFileModal() {
  $('modal-title').textContent = '新建文件';
  $('modal-input').value = 'untitled.py';
  $('modal-overlay').style.display = 'flex';
  setTimeout(() => {
    $('modal-input').select();
    $('modal-input').focus();
  }, 50);
}

$('modal-cancel').addEventListener('click', () => {
  $('modal-overlay').style.display = 'none';
});

$('modal-ok').addEventListener('click', () => {
  const name = $('modal-input').value.trim();
  if (!name) return;
  const path = name.includes('/') ? name : name;
  openTab(path, getDefaultCode(name));
  $('modal-overlay').style.display = 'none';
});

$('modal-input').addEventListener('keydown', (e) => {
  if (e.key === 'Enter') $('modal-ok').click();
  if (e.key === 'Escape') $('modal-cancel').click();
});

function getDefaultCode(name) {
  return `"""
${name} — MaixPy-K210-STM32
"""
from maix import camera, display, app, time

cam  = camera.Camera(320, 240, "RGB888")
disp = display.Display(320, 240)

while not app.need_exit():
    time.fps_start()
    img = cam.read()
    img.draw_string(10, 10, "Hello MaixPy!", color=(255, 255, 255))
    disp.show(img)
    fps = time.fps()
    print(f"[WATCH] fps = {fps:.1f}")
`;
}

// ══════════════════════════════════════════════════════════════════
// 状态栏辅助
// ══════════════════════════════════════════════════════════════════
function setWsStatus(online) {
  const el = $('ws-status');
  el.className = 'ws-status ' + (online ? 'online' : 'offline');
  el.querySelector('.ws-label').textContent = online ? '在线' : '离线';
}

// ══════════════════════════════════════════════════════════════════
// 水平分隔条拖拽
// ══════════════════════════════════════════════════════════════════
(function initResizer() {
  const resizer  = $('h-resizer');
  const edArea   = $('editor-area');
  const termArea = $('terminal-area');
  let dragging = false, startY = 0, startEdH = 0, startTermH = 0;

  resizer.addEventListener('mousedown', (e) => {
    dragging   = true;
    startY     = e.clientY;
    startEdH   = edArea.offsetHeight;
    startTermH = termArea.offsetHeight;
    resizer.classList.add('dragging');
    document.body.style.cursor = 'row-resize';
    document.body.style.userSelect = 'none';
  });

  document.addEventListener('mousemove', (e) => {
    if (!dragging) return;
    const dy = e.clientY - startY;
    const newEdH   = Math.max(80, startEdH + dy);
    const newTermH = Math.max(60, startTermH - dy);
    edArea.style.flex   = 'none';
    edArea.style.height = newEdH + 'px';
    termArea.style.height = newTermH + 'px';
  });

  document.addEventListener('mouseup', () => {
    if (!dragging) return;
    dragging = false;
    resizer.classList.remove('dragging');
    document.body.style.cursor = '';
    document.body.style.userSelect = '';
  });
})();

// ══════════════════════════════════════════════════════════════════
// 键盘快捷键
// ══════════════════════════════════════════════════════════════════
document.addEventListener('keydown', (e) => {
  const ctrl = e.ctrlKey || e.metaKey;
  if (ctrl && e.key === 's') { e.preventDefault(); saveFile(); return; }
  if (ctrl && e.key === 'n') { e.preventDefault(); showNewFileModal(); return; }
  if (e.key === 'F5')        { e.preventDefault(); if (!state.running) runCode(); return; }
  if (e.key === 'F6')        { e.preventDefault(); if (state.running)  stopCode(); return; }
  if (e.key === 'F7')        { e.preventDefault(); runTests(); return; }
  if (e.key === 'F1')        { e.preventDefault(); showHelp(); return; }
  if (e.key === 'Escape')    { hideHelp(); return; }
});

// ══════════════════════════════════════════════════════════════════
// 快捷键帮助面板
// ══════════════════════════════════════════════════════════════════
function showHelp() {
  $('help-overlay').style.display = 'flex';
}
function hideHelp() {
  $('help-overlay').style.display = 'none';
}
$('help-close').addEventListener('click', hideHelp);
$('help-overlay').addEventListener('click', (e) => {
  if (e.target === $('help-overlay')) hideHelp();
});

// ══════════════════════════════════════════════════════════════════
// 按钮事件绑定
// ══════════════════════════════════════════════════════════════════
$('btn-run').addEventListener('click', runCode);
$('btn-stop').addEventListener('click', stopCode);
$('btn-save').addEventListener('click', saveFile);
$('btn-new').addEventListener('click', showNewFileModal);
$('btn-open').addEventListener('click', () => {
  const path = prompt('输入文件路径（相对于项目根目录）:');
  if (path) loadFile(path.trim());
});
$('btn-clear-term').addEventListener('click', () => { terminal.innerHTML = ''; });
$('btn-wrap-term').addEventListener('click', () => {
  terminal.classList.toggle('wrap');
});
$('btn-capture').addEventListener('click', captureOnce);
$('btn-stream').addEventListener('click', toggleStream);
$('btn-snapshot').addEventListener('click', saveSnapshot);
$('btn-refresh-tree').addEventListener('click', () => loadFileTree());
$('btn-serial-refresh').addEventListener('click', refreshSerialPorts);
$('btn-serial-connect').addEventListener('click', () => {
  if (state.serialConnected) {
    wsSend({ action: 'serial_disconnect' });
    return;
  }
  const port     = $('serial-select').value;
  const baudrate = parseInt($('baudrate-select').value) || 115200;
  if (!port) { termInfo('⚠ 请先选择串口'); return; }
  wsSend({ action: 'serial_connect', port, baudrate });
});
$('btn-add-watch').addEventListener('click', () => {
  const name = prompt('变量名:');
  if (name) { state.watchVars[name] = '--'; renderWatchList(); }
});
$('btn-send-serial').addEventListener('click', () => {
  const data = $('terminal-input').value;
  if (data) {
    wsSend({ action: 'serial_write', data });
    $('terminal-input').value = '';
  }
});
$('btn-send-stdin') && $('btn-send-stdin').addEventListener('click', () => {
  const data = $('terminal-input').value;
  if (data) {
    wsSend({ action: 'stdin', data });
    termAppend('> ' + data + '\n', 'info');
    $('terminal-input').value = '';
  }
});
$('terminal-input').addEventListener('keydown', (e) => {
  if (e.key === 'Enter') {
    // 运行中优先发 stdin，否则发串口
    if (state.running) {
      $('btn-send-stdin') && $('btn-send-stdin').click();
    } else {
      $('btn-send-serial').click();
    }
  }
});
$('btn-test') && $('btn-test').addEventListener('click', runTests);
$('platform-select').addEventListener('change', () => {
  const p = $('platform-select').value;
  $('sb-platform').textContent = p;
  $('di-platform').textContent = p;
  localStorage.setItem('sysu_platform', p);
});

// ══════════════════════════════════════════════════════════════════
// 主题切换
// ══════════════════════════════════════════════════════════════════
const THEMES = {
  dark:  { '--bg0':'#1e1e2e','--bg1':'#252535','--bg2':'#2d2d42','--bg3':'#363650',
           '--fg0':'#cdd6f4','--fg1':'#a6adc8','--fg2':'#6c7086','--accent':'#7c6af7',
           '--accent2':'#56cfb2','--run':'#3dba6e','--stop':'#e05c5c' },
  light: { '--bg0':'#f8f8fc','--bg1':'#ededf5','--bg2':'#e0e0ee','--bg3':'#c8c8dc',
           '--fg0':'#1e1e2e','--fg1':'#3a3a5c','--fg2':'#6c6c8a','--accent':'#5a4fcf',
           '--accent2':'#1a9e82','--run':'#2a9a52','--stop':'#c03030' },
};
let _theme = localStorage.getItem('sysu_theme') || 'dark';

function applyTheme(name) {
  const t = THEMES[name] || THEMES.dark;
  const root = document.documentElement;
  Object.entries(t).forEach(([k, v]) => root.style.setProperty(k, v));
  $('btn-theme').textContent = name === 'dark' ? '☀' : '🌙';
  $('btn-theme').title = name === 'dark' ? '切换亮色主题' : '切换暗色主题';
  localStorage.setItem('sysu_theme', name);
  _theme = name;
}

$('btn-theme').addEventListener('click', () => {
  applyTheme(_theme === 'dark' ? 'light' : 'dark');
});

// ══════════════════════════════════════════════════════════════════
// 终端搜索
// ══════════════════════════════════════════════════════════════════
let _termSearchActive = false;

function toggleTermSearch() {
  _termSearchActive = !_termSearchActive;
  let bar = $('term-search-bar');
  if (_termSearchActive) {
    if (!bar) {
      bar = document.createElement('div');
      bar.id = 'term-search-bar';
      bar.className = 'term-search-bar';
      bar.innerHTML = `<input id="term-search-input" class="terminal-input" placeholder="搜索终端..." style="flex:1">
        <span id="term-search-count" style="color:var(--fg2);font-size:11px;padding:0 6px"></span>
        <button class="btn btn-icon-sm" id="term-search-close">✕</button>`;
      $('terminal-area').insertBefore(bar, $('terminal'));
      $('term-search-close').addEventListener('click', () => {
        _termSearchActive = false;
        bar.remove();
        clearTermHighlight();
      });
      $('term-search-input').addEventListener('input', doTermSearch);
      $('term-search-input').addEventListener('keydown', (e) => {
        if (e.key === 'Escape') $('term-search-close').click();
      });
    }
    setTimeout(() => $('term-search-input') && $('term-search-input').focus(), 50);
  } else if (bar) {
    bar.remove();
    clearTermHighlight();
  }
}

function doTermSearch() {
  const q = $('term-search-input').value.trim();
  clearTermHighlight();
  if (!q) { $('term-search-count').textContent = ''; return; }
  const spans = terminal.querySelectorAll('span');
  let count = 0;
  spans.forEach(span => {
    const text = span.textContent;
    if (text.toLowerCase().includes(q.toLowerCase())) {
      span.innerHTML = span.textContent.replace(
        new RegExp(q.replace(/[.*+?^${}()|[\]\\]/g,'\\$&'), 'gi'),
        m => `<mark class="term-highlight">${m}</mark>`
      );
      count += span.querySelectorAll('.term-highlight').length;
    }
  });
  $('term-search-count').textContent = count ? `${count} 处` : '无匹配';
}

function clearTermHighlight() {
  terminal.querySelectorAll('.term-highlight').forEach(el => {
    el.replaceWith(document.createTextNode(el.textContent));
  });
}

// ══════════════════════════════════════════════════════════════════
// 图像缩放
// ══════════════════════════════════════════════════════════════════
let _previewZoom = 1.0;

function setPreviewZoom(z) {
  _previewZoom = Math.max(0.25, Math.min(4.0, z));
  previewCanvas.style.transform = `scale(${_previewZoom})`;
  previewCanvas.style.transformOrigin = 'top left';
  $('preview-info') && ($('preview-info').textContent =
    `${previewCanvas.width} × ${previewCanvas.height} · JPEG · ${Math.round(_previewZoom*100)}%`);
}

// 滚轮缩放
document.querySelector('.preview-wrap') && document.querySelector('.preview-wrap')
  .addEventListener('wheel', (e) => {
    e.preventDefault();
    setPreviewZoom(_previewZoom + (e.deltaY < 0 ? 0.1 : -0.1));
  }, { passive: false });

// ══════════════════════════════════════════════════════════════════
// 本地存储持久化
// ══════════════════════════════════════════════════════════════════
function saveSettings() {
  localStorage.setItem('sysu_platform', $('platform-select').value);
  localStorage.setItem('sysu_baudrate', $('baudrate-select').value);
  localStorage.setItem('sysu_theme',    _theme);
}

function loadSettings() {
  const platform = localStorage.getItem('sysu_platform');
  if (platform) {
    $('platform-select').value = platform;
    $('sb-platform').textContent = platform;
  }
  const baudrate = localStorage.getItem('sysu_baudrate');
  if (baudrate) $('baudrate-select').value = baudrate;
  const theme = localStorage.getItem('sysu_theme') || 'dark';
  applyTheme(theme);
}

// 定期保存设置
setInterval(saveSettings, 5000);

// ══════════════════════════════════════════════════════════════════
// 设备信息初始化
// ══════════════════════════════════════════════════════════════════
function initDeviceInfo() {
  const p = $('platform-select').value;
  $('di-platform').textContent = p;
  $('di-version').textContent  = '1.0.0';
  $('di-status').textContent   = '空闲';
  $('di-runtime').textContent  = '--';
}

// ══════════════════════════════════════════════════════════════════
// 启动
// ══════════════════════════════════════════════════════════════════

// 全局错误处理
window.onerror = (msg, src, line, col, err) => {
  termAppend(`[JS ERROR] ${msg} (${src}:${line})\n`, 'stderr');
  return false;
};
window.addEventListener('unhandledrejection', (e) => {
  termAppend(`[PROMISE ERROR] ${e.reason}\n`, 'stderr');
});

(async function init() {
  // 加载持久化设置
  loadSettings();

  // 默认打开一个新文件
  openTab('untitled.py', getDefaultCode('untitled.py'));
  updateLineNumbers();
  updateCursor();
  initDeviceInfo();

  // 加载文件树和示例
  await Promise.all([loadFileTree(), loadExamples()]);

  // 刷新串口
  await refreshSerialPorts();

  // 获取版本信息
  try {
    const r = await fetch(API('/api/version'));
    const d = await r.json();
    $('di-version').textContent  = d.version || '1.0.0';
    $('di-platform').textContent = $('platform-select').value;
  } catch(e) {}

  // 绑定新增按钮
  $('btn-help')  && $('btn-help').addEventListener('click', showHelp);
  $('btn-theme') && $('btn-theme').addEventListener('click', () => {
    applyTheme(_theme === 'dark' ? 'light' : 'dark');
  });

  // 终端搜索快捷键 Ctrl+F（在终端区域内）
  terminal.addEventListener('keydown', (e) => {
    if ((e.ctrlKey || e.metaKey) && e.key === 'f') {
      e.preventDefault();
      toggleTermSearch();
    }
  });

  // 图像预览滚轮缩放（延迟绑定，确保 DOM 就绪）
  const pw = document.querySelector('.preview-wrap');
  if (pw) {
    pw.addEventListener('wheel', (e) => {
      e.preventDefault();
      setPreviewZoom(_previewZoom + (e.deltaY < 0 ? 0.1 : -0.1));
    }, { passive: false });
  }

  // 连接 WebSocket
  wsConnect();

  termInfo('SYSU_AIOT_IDE 已就绪');
  termInfo('快捷键: F5 运行 | F6 停止 | Ctrl+S 保存 | Ctrl+/ 注释 | F1 帮助');
})();
