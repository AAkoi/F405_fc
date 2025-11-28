// 主应用程序
class DroneVisualizerApp {
    constructor() {
        console.log('[App] Starting initialization...');
        
        try {
            this.serialManager = new SerialManager();
            console.log('[App] SerialManager created');
            
            this.uiManager = new UIManager();
            console.log('[App] UIManager created');
            
            this.recordingManager = new RecordingManager();
            console.log('[App] RecordingManager created');
            
            this.orientationCalc = new OrientationCalculator();
            console.log('[App] OrientationCalculator created');
            
            this.sceneManager = new SceneManager('canvas-container');
            console.log('[App] SceneManager created');
            
            this.droneModel = new DroneModel(this.sceneManager.getScene());
            console.log('[App] DroneModel created');

            // Console throttle state
            this.lastConsoleTs = 0;
            this.throttleSkipped = 0;
            this.throttleIntervalMs = 200;
            
            this.init();
        } catch (error) {
            console.error('[App] Initialization error:', error);
            alert('应用初始化失败: ' + error.message);
        }
    }

    init() {
        // 设置串口数据回调
        this.serialManager.onDataUpdate = (data) => {
            this.onDataUpdate(data);
        };
        this.serialManager.onLine = (line) => this.appendConsoleLine(line, 'log-rx');

        // 绑定按钮事件
        this.bindEvents();

        // 初始化视角
        this.setCamera('iso');

        // 启动渲染循环
        this.animate();
    }

    bindEvents() {
        // 连接按钮
        const connectBtn = document.getElementById('connectBtn');
        if (connectBtn) {
            connectBtn.addEventListener('click', () => {
                if (this.serialManager.port) {
                    this.disconnectSerial();
                } else {
                    this.connectSerial();
                }
            });
        }

        // 录制按钮
        const recordBtn = document.getElementById('recordBtn');
        if (recordBtn) {
            recordBtn.addEventListener('click', () => this.toggleRecording());
        }

        // 磁力计校准按钮
        const magCalibBtn = document.getElementById('magCalibBtn');
        if (magCalibBtn) {
            magCalibBtn.addEventListener('click', () => {
                if (window.openMagCalibModal) {
                    window.openMagCalibModal();
                }
            });
        }

        // 电机测试按钮
        const motorBtn = document.getElementById('motorTestBtn');
        if (motorBtn) {
            motorBtn.addEventListener('click', (e) => {
                e.preventDefault();
                if (typeof window.startMotorFlow === 'function') {
                    window.startMotorFlow();
                } else if (typeof window.openMotorModal === 'function') {
                    window.openMotorModal();
                }
            });
        }

    }

    async connectSerial() {
        try {
            await this.serialManager.connect();
            this.uiManager.setConnectionStatus(true);
            
            const connectBtn = document.getElementById('connectBtn');
            if (connectBtn) {
                connectBtn.innerHTML = '<span class="status-dot"></span><span>断开连接</span>';
                connectBtn.classList.remove('btn-primary');
                connectBtn.classList.add('btn-danger', 'connected');
                // Re-initialize icons
                if (typeof lucide !== 'undefined') {
                    lucide.createIcons();
                }
            }
            
            const recordBtn = document.getElementById('recordBtn');
            if (recordBtn) recordBtn.style.display = 'inline-flex';
            
            this.serialManager.startReading();
            this.appendConsoleLine('Serial port opened.', 'log-sys');
        } catch (e) {
            console.error('串口连接失败:', e);
            const msg = e?.message === 'No port selected by the user.' ? '未选择串口，连接已取消' : (e?.message || '串口连接失败');
            if (this.uiManager.alerts) {
                this.uiManager.alerts.toast({ type: 'error', title: 'Serial Error', message: msg });
            } else {
                alert(msg);
            }
            this.appendConsoleLine(`ERR: ${msg}`, 'log-err');
        }
    }

    async disconnectSerial() {
        try {
            await this.serialManager.disconnect();
        } catch (e) {
            console.warn('串口断开时异常', e);
        }
        this.uiManager.setConnectionStatus(false);
        const connectBtn = document.getElementById('connectBtn');
        if (connectBtn) {
            connectBtn.innerHTML = '<span class="status-dot"></span><span>连接串口</span>';
            connectBtn.classList.remove('btn-danger', 'connected');
            connectBtn.classList.add('btn-primary');
            // Re-initialize icons
            if (typeof lucide !== 'undefined') {
                lucide.createIcons();
            }
        }
        const recordBtn = document.getElementById('recordBtn');
        if (recordBtn) {
            recordBtn.style.display = 'none';
            recordBtn.innerHTML = '<i data-lucide="circle-dot"></i> 开始录制';
        }
        this.appendConsoleLine('Serial port closed.', 'log-sys');
    }

    toggleRecording() {
        const recordBtn = document.getElementById('recordBtn');
        
        if (!this.recordingManager.getStatus()) {
            // 开始录制
            this.recordingManager.start();
            if (recordBtn) {
                recordBtn.innerHTML = '<i data-lucide="square"></i> 停止录制';
                recordBtn.style.background = '#059669';
                // Re-initialize icons after DOM change
                if (typeof lucide !== 'undefined') {
                    lucide.createIcons();
                }
            }
            this.uiManager.setRecordingStatus(true);
        } else {
            // 停止录制
            this.recordingManager.stop();
            if (recordBtn) {
                recordBtn.innerHTML = '<i data-lucide="circle-dot"></i> 开始录制';
                recordBtn.style.background = '#DC2626';
                // Re-initialize icons after DOM change
                if (typeof lucide !== 'undefined') {
                    lucide.createIcons();
                }
            }
            this.uiManager.setRecordingStatus(false);
            this.recordingManager.export();
        }
    }

    onDataUpdate(sensorData) {
        const updateType = sensorData.lastUpdateType || 'sensor';

        // 更新UI
        this.uiManager.updateSensorData(sensorData);

        // 录制数据
        if (this.recordingManager.getStatus() && (updateType === 'attitude' || updateType === 'sensor')) {
            this.recordingManager.addDataPoint(sensorData.gyr);
        }

        // 更新姿态
        if (updateType === 'attitude' || updateType === 'sensor') {
            const orientation = this.orientationCalc.update(sensorData);
            this.uiManager.updateOrientation(this.orientationCalc.getOrientation());
            
            // 调试：每秒打印一次姿态数据
            if (!this.lastDebugTime || Date.now() - this.lastDebugTime > 1000) {
                console.log('姿态角(度):', this.orientationCalc.getOrientation());
                console.log('姿态角(弧度):', orientation);
                this.lastDebugTime = Date.now();
            }
            
            this.droneModel.updateOrientation(orientation.pitch, orientation.roll, orientation.yaw);
        }
    }

    setCamera(mode) {
        // 更新按钮状态
        document.querySelectorAll('.view-btn').forEach(btn => {
            btn.classList.remove('active');
            if (btn.dataset.mode === mode) {
                btn.classList.add('active');
            }
        });

        // 设置相机
        this.sceneManager.setCamera(mode);
    }

    animate() {
        requestAnimationFrame(() => this.animate());
        
        // 动画螺旋桨
        this.droneModel.animatePropellers();
        
        // 渲染场景
        this.sceneManager.render();
    }

    appendConsoleLine(text, cls = '') {
        const out = document.getElementById('console-body');
        if (!out) return;
        const throttleEnabled = document.getElementById('chk-throttle')?.checked;
        const now = Date.now();
        if (throttleEnabled) {
            if (now - this.lastConsoleTs < this.throttleIntervalMs) {
                this.throttleSkipped += 1;
                return;
            }
            if (this.throttleSkipped > 0) {
                const info = document.createElement('div');
                info.className = 'log-line';
                info.innerHTML = `<span class="log-sys">[throttle] skipped ${this.throttleSkipped} lines</span>`;
                out.appendChild(info);
                this.throttleSkipped = 0;
            }
            this.lastConsoleTs = now;
        } else {
            this.throttleSkipped = 0;
            this.lastConsoleTs = now;
        }
        const line = document.createElement('div');
        line.className = 'log-line';
        const showTs = document.getElementById('chk-ts')?.checked;
        const autoScroll = document.getElementById('chk-scroll')?.checked;
        const ts = showTs ? `[${new Date().toLocaleTimeString()}] ` : '';
        line.innerHTML = `${ts}<span class="${cls}">${text}</span>`;
        out.appendChild(line);
        if (autoScroll) {
            out.scrollTop = out.scrollHeight;
        }
    }
}

// 页面加载完成后启动应用
document.addEventListener('DOMContentLoaded', () => {
    console.log('[App] DOM Content Loaded');
    console.log('[App] THREE available:', typeof THREE !== 'undefined');
    console.log('[App] OrbitControls available:', typeof THREE.OrbitControls !== 'undefined');
    
    try {
        window.app = new DroneVisualizerApp();
        console.log('[App] Application started successfully');
    } catch (error) {
        console.error('[App] Failed to start application:', error);
    }
});
