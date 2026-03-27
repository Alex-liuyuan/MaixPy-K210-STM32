/**
 * SYSU Studio — 前端逻辑
 * 模块：BoardManager, Editor, Terminal, SerialBridge, BuildRunner, FileTree
 */

(function () {
    "use strict";

    // -----------------------------------------------------------------------
    // 状态
    // -----------------------------------------------------------------------
    const state = {
        boards: [],
        ports: [],
        selectedBoard: null,
        selectedPort: "",
        serialConnected: false,
        currentFile: "main.py",
        files: [],
        taskRunning: false,
    };

    // DOM 引用
    const $ = (sel) => document.querySelector(sel);
    const selBoard = $("#sel-board");
    const selPort = $("#sel-port");
    const btnRefreshPorts = $("#btn-refresh-ports");
    const btnBuild = $("#btn-build");
    const btnFlash = $("#btn-flash");
    const btnRunSim = $("#btn-run-sim");
    const btnSerialToggle = $("#btn-serial-toggle");
    const btnCancel = $("#btn-cancel");
    const btnNewFile = $("#btn-new-file");
    const editorContainer = $("#editor-container");
    const terminalContainer = $("#terminal-container");
    const fileTreeEl = $("#file-tree");
    const statusPlatform = $("#status-platform");
    const statusPort = $("#status-port");
    const statusTask = $("#status-task");

    // -----------------------------------------------------------------------
    // Editor（textarea 回退，无需 Node 构建）
    // -----------------------------------------------------------------------
    let editorTextarea = null;

    function initEditor() {
        editorTextarea = document.createElement("textarea");
        editorTextarea.spellcheck = false;
        editorTextarea.placeholder = "# 在此编写 Python 代码...";
        editorContainer.appendChild(editorTextarea);

        // Tab 键插入 4 空格
        editorTextarea.addEventListener("keydown", (e) => {
            if (e.key === "Tab") {
                e.preventDefault();
                const start = editorTextarea.selectionStart;
                const end = editorTextarea.selectionEnd;
                editorTextarea.value =
                    editorTextarea.value.substring(0, start) +
                    "    " +
                    editorTextarea.value.substring(end);
                editorTextarea.selectionStart = editorTextarea.selectionEnd = start + 4;
            }
        });
    }

    function getEditorContent() {
        return editorTextarea ? editorTextarea.value : "";
    }

    function setEditorContent(text) {
        if (editorTextarea) editorTextarea.value = text;
    }

    // -----------------------------------------------------------------------
    // Terminal（xterm.js）
    // -----------------------------------------------------------------------
    let term = null;

    function initTerminal() {
        if (typeof Terminal !== "undefined") {
            term = new Terminal({
                theme: {
                    background: "#1e1e2e",
                    foreground: "#cdd6f4",
                    cursor: "#89b4fa",
                },
                fontSize: 13,
                fontFamily: "'JetBrains Mono', 'Fira Code', monospace",
                cursorBlink: true,
                scrollback: 5000,
            });
            term.open(terminalContainer);
            term.writeln("\x1b[36mSYSU Studio 终端就绪\x1b[0m");
        } else {
            // 回退：纯文本 pre
            const pre = document.createElement("pre");
            pre.id = "term-fallback";
            pre.style.cssText =
                "width:100%;height:100%;overflow:auto;padding:8px;font-size:13px;color:#cdd6f4;background:#1e1e2e;font-family:monospace;white-space:pre-wrap;";
            terminalContainer.appendChild(pre);
        }
    }

    function termWrite(text) {
        if (term) {
            term.writeln(text);
        } else {
            const pre = $("#term-fallback");
            if (pre) {
                pre.textContent += text + "\n";
                pre.scrollTop = pre.scrollHeight;
            }
        }
    }

    function termClear() {
        if (term) {
            term.clear();
        } else {
            const pre = $("#term-fallback");
            if (pre) pre.textContent = "";
        }
    }

    // -----------------------------------------------------------------------
    // BoardManager
    // -----------------------------------------------------------------------
    async function loadBoards() {
        try {
            const res = await fetch("/api/boards");
            state.boards = await res.json();
            selBoard.innerHTML = "";
            state.boards.forEach((b) => {
                const opt = document.createElement("option");
                opt.value = b.platform;
                opt.textContent = `${b.name} (${b.platform})`;
                selBoard.appendChild(opt);
            });
            if (state.boards.length > 0) {
                state.selectedBoard = state.boards[0];
                updatePlatformStatus();
            }
        } catch (e) {
            selBoard.innerHTML = '<option value="">加载失败</option>';
        }
    }

    async function loadPorts() {
        try {
            const res = await fetch("/api/serial/ports");
            state.ports = await res.json();
            selPort.innerHTML = '<option value="">无</option>';
            state.ports.forEach((p) => {
                const opt = document.createElement("option");
                opt.value = p;
                opt.textContent = p;
                selPort.appendChild(opt);
            });
        } catch (e) {
            selPort.innerHTML = '<option value="">检测失败</option>';
        }
    }

    function updatePlatformStatus() {
        const board = state.boards.find((b) => b.platform === selBoard.value);
        state.selectedBoard = board || null;
        statusPlatform.textContent = board
            ? `平台: ${board.platform} (${board.arch})`
            : "平台: -";
    }

    // -----------------------------------------------------------------------
    // FileTree
    // -----------------------------------------------------------------------
    async function loadFileTree() {
        try {
            const res = await fetch("/api/project/files");
            state.files = await res.json();
            renderFileTree();
        } catch (e) {
            fileTreeEl.innerHTML = '<div class="file-item">加载失败</div>';
        }
    }

    function renderFileTree() {
        fileTreeEl.innerHTML = "";
        const files = state.files.filter((f) => !f.is_dir);
        files.forEach((f) => {
            const div = document.createElement("div");
            div.className = "file-item" + (f.path === state.currentFile ? " active" : "");
            div.innerHTML = `<span class="icon">\u{1F4C4}</span>${f.path}`;
            div.addEventListener("click", () => openFile(f.path));
            fileTreeEl.appendChild(div);
        });
    }

    async function openFile(path) {
        try {
            const res = await fetch(`/api/project/file?path=${encodeURIComponent(path)}`);
            const data = await res.json();
            setEditorContent(data.content);
            state.currentFile = path;

            // 更新 tab
            const tabs = $("#editor-tabs");
            tabs.innerHTML = `<span class="tab active" data-path="${path}">${path}</span>`;

            renderFileTree();
        } catch (e) {
            termWrite(`\x1b[31m打开文件失败: ${path}\x1b[0m`);
        }
    }

    async function saveCurrentFile() {
        if (!state.currentFile) return;
        try {
            await fetch("/api/project/file", {
                method: "PUT",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    path: state.currentFile,
                    content: getEditorContent(),
                }),
            });
        } catch (e) {
            termWrite(`\x1b[31m保存失败: ${e.message}\x1b[0m`);
        }
    }

    // -----------------------------------------------------------------------
    // BuildRunner + WebSocket 日志
    // -----------------------------------------------------------------------
    let buildLogWs = null;

    function connectBuildLog() {
        const proto = location.protocol === "https:" ? "wss:" : "ws:";
        buildLogWs = new WebSocket(`${proto}//${location.host}/ws/build-log`);

        buildLogWs.onmessage = (evt) => {
            try {
                const msg = JSON.parse(evt.data);
                if (msg.type === "log") {
                    const color = msg.stream === "stderr" ? "\x1b[33m" : "\x1b[37m";
                    termWrite(`${color}${msg.line}\x1b[0m`);
                } else if (msg.type === "done") {
                    const ok = msg.exit_code === 0;
                    termWrite(
                        ok
                            ? "\x1b[32m--- 完成 (exit 0) ---\x1b[0m"
                            : `\x1b[31m--- 失败 (exit ${msg.exit_code}) ---\x1b[0m`
                    );
                    setTaskRunning(false);
                }
            } catch (e) { /* ignore */ }
        };

        buildLogWs.onclose = () => {
            setTimeout(connectBuildLog, 3000);
        };
    }

    function setTaskRunning(running) {
        state.taskRunning = running;
        btnBuild.disabled = running;
        btnFlash.disabled = running;
        btnRunSim.disabled = running;
        btnCancel.disabled = !running;
        statusTask.textContent = running ? "任务运行中..." : "就绪";
    }

    async function doBuild() {
        await saveCurrentFile();
        termClear();
        termWrite("\x1b[36m--- 开始构建 ---\x1b[0m");
        setTaskRunning(true);
        try {
            const res = await fetch("/api/build", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ platform: selBoard.value || "rtthread" }),
            });
            if (!res.ok) {
                const err = await res.json();
                termWrite(`\x1b[31m${err.detail || "构建请求失败"}\x1b[0m`);
                setTaskRunning(false);
            }
        } catch (e) {
            termWrite(`\x1b[31m请求失败: ${e.message}\x1b[0m`);
            setTaskRunning(false);
        }
    }

    async function doFlash() {
        await saveCurrentFile();
        termClear();
        termWrite("\x1b[36m--- 开始烧录 ---\x1b[0m");
        setTaskRunning(true);
        try {
            const res = await fetch("/api/flash", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    platform: selBoard.value || "rtthread",
                    port: selPort.value || undefined,
                }),
            });
            if (!res.ok) {
                const err = await res.json();
                termWrite(`\x1b[31m${err.detail || "烧录请求失败"}\x1b[0m`);
                setTaskRunning(false);
            }
        } catch (e) {
            termWrite(`\x1b[31m请求失败: ${e.message}\x1b[0m`);
            setTaskRunning(false);
        }
    }

    async function doRunSim() {
        await saveCurrentFile();
        termClear();
        termWrite("\x1b[36m--- 模拟运行 ---\x1b[0m");
        setTaskRunning(true);
        try {
            const res = await fetch("/api/run-sim", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ code: getEditorContent() }),
            });
            if (!res.ok) {
                const err = await res.json();
                termWrite(`\x1b[31m${err.detail || "运行请求失败"}\x1b[0m`);
                setTaskRunning(false);
            }
        } catch (e) {
            termWrite(`\x1b[31m请求失败: ${e.message}\x1b[0m`);
            setTaskRunning(false);
        }
    }

    async function doCancel() {
        try {
            await fetch("/api/cancel", { method: "POST" });
            termWrite("\x1b[33m--- 已请求取消 ---\x1b[0m");
        } catch (e) { /* ignore */ }
    }

    // -----------------------------------------------------------------------
    // SerialBridge WebSocket
    // -----------------------------------------------------------------------
    let serialWs = null;

    function toggleSerial() {
        if (state.serialConnected) {
            // 断开
            if (serialWs && serialWs.readyState === WebSocket.OPEN) {
                serialWs.send(JSON.stringify({ action: "close" }));
            }
            state.serialConnected = false;
            statusPort.textContent = "串口: 未连接";
            btnSerialToggle.textContent = "串口";
            return;
        }

        const port = selPort.value;
        if (!port) {
            termWrite("\x1b[31m请先选择串口\x1b[0m");
            return;
        }

        const proto = location.protocol === "https:" ? "wss:" : "ws:";
        serialWs = new WebSocket(`${proto}//${location.host}/ws/serial`);

        serialWs.onopen = () => {
            serialWs.send(JSON.stringify({ action: "open", port: port, baud: 115200 }));
        };

        serialWs.onmessage = (evt) => {
            try {
                const msg = JSON.parse(evt.data);
                if (msg.type === "status") {
                    state.serialConnected = msg.connected;
                    statusPort.textContent = msg.connected
                        ? `串口: ${port} 已连接`
                        : "串口: 未连接";
                    btnSerialToggle.textContent = msg.connected ? "断开" : "串口";
                    if (msg.error) {
                        termWrite(`\x1b[31m串口错误: ${msg.error}\x1b[0m`);
                    }
                } else if (msg.type === "data") {
                    const bytes = atob(msg.data);
                    if (term) {
                        term.write(bytes);
                    } else {
                        termWrite(bytes);
                    }
                }
            } catch (e) { /* ignore */ }
        };

        serialWs.onclose = () => {
            state.serialConnected = false;
            statusPort.textContent = "串口: 未连接";
            btnSerialToggle.textContent = "串口";
        };

        // 终端输入转发到串口
        if (term) {
            term.onData((data) => {
                if (serialWs && serialWs.readyState === WebSocket.OPEN && state.serialConnected) {
                    serialWs.send(JSON.stringify({ action: "send", data: btoa(data) }));
                }
            });
        }
    }

    // -----------------------------------------------------------------------
    // 新建文件
    // -----------------------------------------------------------------------
    async function createNewFile() {
        const name = prompt("文件名（如 helper.py）:");
        if (!name || !name.trim()) return;
        try {
            await fetch("/api/project/file", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ path: name.trim(), content: "" }),
            });
            await loadFileTree();
            await openFile(name.trim());
        } catch (e) {
            termWrite(`\x1b[31m创建文件失败: ${e.message}\x1b[0m`);
        }
    }

    // -----------------------------------------------------------------------
    // Ctrl+S 保存
    // -----------------------------------------------------------------------
    document.addEventListener("keydown", (e) => {
        if ((e.ctrlKey || e.metaKey) && e.key === "s") {
            e.preventDefault();
            saveCurrentFile().then(() => termWrite("\x1b[32m已保存\x1b[0m"));
        }
    });

    // -----------------------------------------------------------------------
    // 事件绑定
    // -----------------------------------------------------------------------
    selBoard.addEventListener("change", updatePlatformStatus);
    btnRefreshPorts.addEventListener("click", loadPorts);
    btnBuild.addEventListener("click", doBuild);
    btnFlash.addEventListener("click", doFlash);
    btnRunSim.addEventListener("click", doRunSim);
    btnCancel.addEventListener("click", doCancel);
    btnSerialToggle.addEventListener("click", toggleSerial);
    btnNewFile.addEventListener("click", createNewFile);

    // -----------------------------------------------------------------------
    // 初始化
    // -----------------------------------------------------------------------
    initEditor();
    initTerminal();
    connectBuildLog();

    // 加载数据
    loadBoards();
    loadPorts();
    loadFileTree().then(() => openFile("main.py"));
})();
