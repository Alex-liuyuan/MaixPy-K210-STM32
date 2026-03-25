/**
 * SYSU_AIOT_IDE — Python 语法高亮 + 代码补全
 * 轻量级实现，不依赖任何第三方库
 */

// ══════════════════════════════════════════════════════════════════
// 语法高亮（基于正则，覆盖 textarea）
// ══════════════════════════════════════════════════════════════════

const KEYWORDS = new Set([
  'False','None','True','and','as','assert','async','await',
  'break','class','continue','def','del','elif','else','except',
  'finally','for','from','global','if','import','in','is',
  'lambda','nonlocal','not','or','pass','raise','return',
  'try','while','with','yield',
]);

const BUILTINS = new Set([
  'print','len','range','int','float','str','bool','list','dict',
  'tuple','set','type','isinstance','hasattr','getattr','setattr',
  'enumerate','zip','map','filter','sorted','reversed','sum','min',
  'max','abs','round','open','input','super','object','Exception',
  'ValueError','TypeError','RuntimeError','KeyError','IndexError',
  'FileNotFoundError','ImportError','StopIteration',
]);

// MaixPy 专属 API
const MAIX_API = new Set([
  'camera','display','app','time','nn','GPIO','platform','version',
  'Camera','Display','Image','NeuralNetwork','Classifier','Detector',
  'NN','YOLOv5','YOLOv8','DetectionResult','BlobResult',
  'UART','SPI','I2C','ADC','PWM',
  'err','pinmap','filter',
  'check_raise','check_bool','HardwareError','MaixError',
  'set_pin_function','get_pin_function',
]);

/**
 * 将代码字符串转换为带 span 的 HTML（用于 overlay 层）
 * 注意：必须对 HTML 特殊字符转义
 */
function highlight(code) {
  // 逐字符状态机，处理字符串/注释/关键字
  const out = [];
  let i = 0;
  const n = code.length;

  function esc(s) {
    return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
  }

  while (i < n) {
    // 注释
    if (code[i] === '#') {
      let j = i;
      while (j < n && code[j] !== '\n') j++;
      out.push(`<span class="hl-comment">${esc(code.slice(i, j))}</span>`);
      i = j;
      continue;
    }
    // 三引号字符串
    if ((code[i] === '"' || code[i] === "'") &&
        code.slice(i, i+3) === code[i].repeat(3)) {
      const q = code[i].repeat(3);
      let j = i + 3;
      while (j < n && code.slice(j, j+3) !== q) j++;
      j += 3;
      out.push(`<span class="hl-string">${esc(code.slice(i, j))}</span>`);
      i = j;
      continue;
    }
    // 单引号字符串
    if (code[i] === '"' || code[i] === "'") {
      const q = code[i];
      let j = i + 1;
      while (j < n && code[j] !== q && code[j] !== '\n') {
        if (code[j] === '\\') j++;
        j++;
      }
      j++;
      out.push(`<span class="hl-string">${esc(code.slice(i, j))}</span>`);
      i = j;
      continue;
    }
    // 数字
    if (/[0-9]/.test(code[i]) ||
        (code[i] === '.' && i+1 < n && /[0-9]/.test(code[i+1]))) {
      let j = i;
      while (j < n && /[0-9a-fA-FxXoObB._]/.test(code[j])) j++;
      out.push(`<span class="hl-number">${esc(code.slice(i, j))}</span>`);
      i = j;
      continue;
    }
    // 标识符 / 关键字
    if (/[a-zA-Z_]/.test(code[i])) {
      let j = i;
      while (j < n && /[a-zA-Z0-9_]/.test(code[j])) j++;
      const word = code.slice(i, j);
      if (KEYWORDS.has(word)) {
        out.push(`<span class="hl-keyword">${esc(word)}</span>`);
      } else if (BUILTINS.has(word)) {
        out.push(`<span class="hl-builtin">${esc(word)}</span>`);
      } else if (MAIX_API.has(word)) {
        out.push(`<span class="hl-maix">${esc(word)}</span>`);
      } else if (j < n && code[j] === '(') {
        out.push(`<span class="hl-func">${esc(word)}</span>`);
      } else {
        out.push(esc(word));
      }
      i = j;
      continue;
    }
    // 装饰器
    if (code[i] === '@') {
      let j = i + 1;
      while (j < n && /[a-zA-Z0-9_.]/.test(code[j])) j++;
      out.push(`<span class="hl-decorator">${esc(code.slice(i, j))}</span>`);
      i = j;
      continue;
    }
    // 运算符
    if (/[+\-*/%=<>!&|^~]/.test(code[i])) {
      out.push(`<span class="hl-op">${esc(code[i])}</span>`);
      i++;
      continue;
    }
    out.push(esc(code[i]));
    i++;
  }
  return out.join('');
}

// ══════════════════════════════════════════════════════════════════
// 高亮 Overlay 层
// ══════════════════════════════════════════════════════════════════

let _hlScheduled = false;

function scheduleHighlight() {
  if (_hlScheduled) return;
  _hlScheduled = true;
  requestAnimationFrame(() => {
    _hlScheduled = false;
    const overlay = document.getElementById('hl-overlay');
    if (!overlay) return;
    const editor = document.getElementById('code-editor');
    overlay.innerHTML = highlight(editor.value);
    // 同步滚动
    overlay.scrollTop  = editor.scrollTop;
    overlay.scrollLeft = editor.scrollLeft;
  });
}

function initHighlight() {
  const editorWrap = document.querySelector('.editor-wrap');
  const editor     = document.getElementById('code-editor');

  // 创建 overlay div
  const overlay = document.createElement('div');
  overlay.id        = 'hl-overlay';
  overlay.className = 'highlight-layer';
  overlay.setAttribute('aria-hidden', 'true');
  editorWrap.style.position = 'relative';
  editorWrap.insertBefore(overlay, editor);

  // editor 透明（文字颜色透明，光标保留）
  editor.style.color       = 'transparent';
  editor.style.caretColor  = 'var(--fg0)';
  editor.style.position    = 'relative';
  editor.style.zIndex      = '1';
  editor.style.background  = 'transparent';

  // 同步滚动
  editor.addEventListener('scroll', () => {
    overlay.scrollTop  = editor.scrollTop;
    overlay.scrollLeft = editor.scrollLeft;
  });

  // 监听输入
  editor.addEventListener('input', scheduleHighlight);
  editor.addEventListener('keyup', scheduleHighlight);

  scheduleHighlight();
}

// ══════════════════════════════════════════════════════════════════
// 代码补全
// ══════════════════════════════════════════════════════════════════

const COMPLETIONS = [
  // maix 模块
  { label: 'from maix import camera, display, app, time', kind: 'snippet',
    insert: 'from maix import camera, display, app, time' },
  { label: 'from maix import GPIO', kind: 'snippet',
    insert: 'from maix import GPIO' },
  { label: 'from maix import nn', kind: 'snippet',
    insert: 'from maix import nn' },
  { label: 'from maix.err import check_raise, HardwareError', kind: 'snippet',
    insert: 'from maix.err import check_raise, HardwareError' },
  { label: 'from maix import pinmap', kind: 'snippet',
    insert: 'from maix import pinmap' },

  // camera
  { label: 'camera.Camera(320, 240, "RGB888")', kind: 'api',
    insert: 'camera.Camera(320, 240, "RGB888")' },
  { label: 'cam.read()', kind: 'api', insert: 'cam.read()' },

  // display
  { label: 'display.Display(320, 240)', kind: 'api',
    insert: 'display.Display(320, 240)' },
  { label: 'disp.show(img)', kind: 'api', insert: 'disp.show(img)' },

  // image
  { label: 'img.draw_string(x, y, text, color)', kind: 'api',
    insert: 'img.draw_string(10, 10, "Hello", color=(255,255,255))' },
  { label: 'img.draw_rectangle(x, y, w, h, color)', kind: 'api',
    insert: 'img.draw_rectangle(10, 10, 100, 50, color=(255,0,0))' },
  { label: 'img.find_blobs(thresholds)', kind: 'api',
    insert: 'img.find_blobs([(200, 255, 0, 100, 0, 100)])' },

  // time
  { label: 'time.fps_start()', kind: 'api', insert: 'time.fps_start()' },
  { label: 'time.fps()', kind: 'api', insert: 'time.fps()' },
  { label: 'time.sleep_ms(ms)', kind: 'api', insert: 'time.sleep_ms(100)' },
  { label: 'time.ticks_ms()', kind: 'api', insert: 'time.ticks_ms()' },
  { label: 'time.ticks_diff(start)', kind: 'api', insert: 'time.ticks_diff(t0)' },

  // GPIO
  { label: 'GPIO(pin, GPIO.Mode.OUT)', kind: 'api',
    insert: 'GPIO(0, GPIO.Mode.OUT)' },
  { label: 'GPIO(pin, GPIO.Mode.IN)', kind: 'api',
    insert: 'GPIO(0, GPIO.Mode.IN)' },
  { label: 'led.on() / led.off() / led.toggle()', kind: 'api',
    insert: 'led.on()' },

  // nn
  { label: 'nn.Classifier(model_path, labels)', kind: 'api',
    insert: 'nn.Classifier("model.tflite", labels)' },
  { label: 'nn.Detector(model_path, labels, threshold)', kind: 'api',
    insert: 'nn.Detector("model.tflite", labels, threshold=0.5)' },
  { label: 'nn.NN(model_path)', kind: 'api',
    insert: 'nn.NN("model.tflite")' },
  { label: 'nn.YOLOv5(model, labels)', kind: 'api',
    insert: 'nn.YOLOv5("yolo.tflite", labels)' },
  { label: 'nn.YOLOv8(model, labels)', kind: 'api',
    insert: 'nn.YOLOv8("yolo.tflite", labels)' },

  // uart
  { label: 'uart.UART(id, baudrate)', kind: 'api',
    insert: 'uart.UART(1, 115200)' },

  // spi
  { label: 'spi.SPI(id, freq)', kind: 'api',
    insert: 'spi.SPI(1, freq=1_000_000)' },

  // adc
  { label: 'adc.ADC(id, resolution, vref)', kind: 'api',
    insert: 'adc.ADC(1, resolution=adc.RES_BIT_12, vref=3.3)' },

  // pwm
  { label: 'pwm.PWM(pwm_id, freq, duty)', kind: 'api',
    insert: 'pwm.PWM(pwm_id=1, freq=1000, duty=50.0)' },

  // err
  { label: 'check_raise(ret, msg)', kind: 'api',
    insert: 'check_raise(ret, "操作失败")' },

  // pinmap
  { label: 'pinmap.set_pin_function(pin, func)', kind: 'api',
    insert: 'pinmap.set_pin_function("PA9", "USART1_TX")' },

  // 常用模板
  { label: '主循环模板', kind: 'snippet',
    insert: `from maix import camera, display, app, time

cam  = camera.Camera(320, 240, "RGB888")
disp = display.Display(320, 240)

while not app.need_exit():
    time.fps_start()
    img = cam.read()
    img.draw_string(10, 10, f"FPS: {time.fps():.1f}", color=(255,255,255))
    disp.show(img)` },

  { label: 'GPIO 闪烁模板', kind: 'snippet',
    insert: `from maix import GPIO, time, app

led = GPIO(0, GPIO.Mode.OUT)

while not app.need_exit():
    led.toggle()
    time.sleep_ms(500)` },

  { label: 'AI 分类模板', kind: 'snippet',
    insert: `from maix import camera, display, app, time, nn

cam        = camera.Camera(224, 224, "RGB888")
disp       = display.Display()
labels     = ["cat", "dog", "person"]
classifier = nn.Classifier(None, labels)

while not app.need_exit():
    time.fps_start()
    img     = cam.read()
    results = classifier.classify(img)
    if results:
        _, conf, label = results[0]
        img.draw_string(10, 10, f"{label}: {conf:.2f}", color=(0,255,0))
    disp.show(img)` },

  { label: '[WATCH] 变量监视', kind: 'snippet',
    insert: 'print(f"[WATCH] var_name = {var_name}")' },
];

let _acList   = null;
let _acActive = -1;

function initAutocomplete() {
  const editor = document.getElementById('code-editor');

  // 创建补全列表
  _acList = document.createElement('div');
  _acList.id        = 'ac-list';
  _acList.className = 'ac-list';
  _acList.style.display = 'none';
  document.body.appendChild(_acList);

  editor.addEventListener('keydown', onEditorKeydown, true);
  editor.addEventListener('input',   onEditorInput);
  document.addEventListener('click', (e) => {
    if (!_acList.contains(e.target)) hideAC();
  });
}

function onEditorKeydown(e) {
  if (_acList.style.display === 'none') {
    if (e.key === ' ' && e.ctrlKey) { e.preventDefault(); triggerAC(); }
    return;
  }
  const items = _acList.querySelectorAll('.ac-item');
  if (e.key === 'ArrowDown') {
    e.preventDefault();
    _acActive = Math.min(_acActive + 1, items.length - 1);
    renderACActive(items);
  } else if (e.key === 'ArrowUp') {
    e.preventDefault();
    _acActive = Math.max(_acActive - 1, 0);
    renderACActive(items);
  } else if (e.key === 'Enter' || e.key === 'Tab') {
    if (_acActive >= 0 && items[_acActive]) {
      e.preventDefault();
      applyCompletion(items[_acActive].dataset.insert);
    }
  } else if (e.key === 'Escape') {
    hideAC();
  }
}

function onEditorInput() {
  const editor = document.getElementById('code-editor');
  const s      = editor.selectionStart;
  const v      = editor.value;
  const lineStart = v.lastIndexOf('\n', s - 1) + 1;
  const prefix    = v.slice(lineStart, s);
  const word      = prefix.match(/[\w.]*$/)?.[0] || '';

  if (word.length < 2) { hideAC(); return; }
  showAC(word, s);
}

function triggerAC() {
  const editor = document.getElementById('code-editor');
  const s      = editor.selectionStart;
  const v      = editor.value;
  const lineStart = v.lastIndexOf('\n', s - 1) + 1;
  const prefix    = v.slice(lineStart, s);
  const word      = prefix.match(/[\w.]*$/)?.[0] || '';
  showAC(word, s);
}

function showAC(word, cursorPos) {
  const lower   = word.toLowerCase();
  const matches = COMPLETIONS.filter(c =>
    c.label.toLowerCase().includes(lower) ||
    c.insert.toLowerCase().includes(lower)
  ).slice(0, 12);

  if (matches.length === 0) { hideAC(); return; }

  _acList.innerHTML = '';
  _acActive = 0;
  matches.forEach((c, i) => {
    const item = document.createElement('div');
    item.className   = 'ac-item' + (i === 0 ? ' ac-active' : '');
    item.dataset.insert = c.insert;
    const kindIcon = c.kind === 'snippet' ? '⬡' : '◆';
    const kindCls  = c.kind === 'snippet' ? 'ac-snippet' : 'ac-api';
    item.innerHTML = `<span class="ac-kind ${kindCls}">${kindIcon}</span>
      <span class="ac-label">${c.label}</span>`;
    item.addEventListener('mousedown', (e) => {
      e.preventDefault();
      applyCompletion(c.insert);
    });
    _acList.appendChild(item);
  });

  // 定位到光标
  const editor = document.getElementById('code-editor');
  const rect   = editor.getBoundingClientRect();
  const coords = getCaretCoords(editor, cursorPos);
  const x = rect.left + coords.left - editor.scrollLeft;
  const y = rect.top  + coords.top  - editor.scrollTop + 20;
  _acList.style.left    = Math.min(x, window.innerWidth - 320) + 'px';
  _acList.style.top     = Math.min(y, window.innerHeight - 200) + 'px';
  _acList.style.display = 'block';
}

function hideAC() {
  if (_acList) _acList.style.display = 'none';
  _acActive = -1;
}

function renderACActive(items) {
  items.forEach((el, i) => {
    el.classList.toggle('ac-active', i === _acActive);
    if (i === _acActive) el.scrollIntoView({ block: 'nearest' });
  });
}

function applyCompletion(insert) {
  const editor = document.getElementById('code-editor');
  const s      = editor.selectionStart;
  const v      = editor.value;
  const lineStart = v.lastIndexOf('\n', s - 1) + 1;
  const prefix    = v.slice(lineStart, s);
  const wordMatch = prefix.match(/[\w.]*$/);
  const wordStart = s - (wordMatch?.[0]?.length || 0);

  editor.value = v.slice(0, wordStart) + insert + v.slice(editor.selectionEnd);
  editor.selectionStart = editor.selectionEnd = wordStart + insert.length;
  hideAC();

  // 触发更新
  editor.dispatchEvent(new Event('input'));
  if (typeof updateLineNumbers === 'function') updateLineNumbers();
  if (typeof markDirty === 'function') markDirty();
}

// 计算光标在 textarea 中的像素坐标（简化版）
function getCaretCoords(textarea, pos) {
  const div = document.createElement('div');
  const style = window.getComputedStyle(textarea);
  ['fontFamily','fontSize','fontWeight','lineHeight','padding',
   'paddingTop','paddingLeft','border','whiteSpace','wordWrap',
   'overflowWrap','tabSize'].forEach(p => {
    div.style[p] = style[p];
  });
  div.style.position   = 'absolute';
  div.style.visibility = 'hidden';
  div.style.whiteSpace = 'pre-wrap';
  div.style.width      = textarea.offsetWidth + 'px';
  document.body.appendChild(div);

  const text = textarea.value.slice(0, pos);
  div.textContent = text;
  const span = document.createElement('span');
  span.textContent = '|';
  div.appendChild(span);

  const rect = span.getBoundingClientRect();
  const divRect = div.getBoundingClientRect();
  document.body.removeChild(div);
  return { left: rect.left - divRect.left, top: rect.top - divRect.top };
}

// ══════════════════════════════════════════════════════════════════
// 初始化（在 app.js 的 init() 之后调用）
// ══════════════════════════════════════════════════════════════════
document.addEventListener('DOMContentLoaded', () => {
  initHighlight();
  initAutocomplete();
});
