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
            connectBtn.addEventListener('click', () => this.connectSerial());
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

        // 视角按钮
        const viewButtons = document.querySelectorAll('.view-btn');
        viewButtons.forEach(btn => {
            btn.addEventListener('click', (e) => {
                const mode = e.currentTarget.dataset.mode;
                if (mode) this.setCamera(mode);
            });
        });
    }

    async connectSerial() {
        try {
            await this.serialManager.connect();
            this.uiManager.setConnectionStatus(true);
            
            const connectBtn = document.getElementById('connectBtn');
            if (connectBtn) connectBtn.style.display = 'none';
            
            const recordBtn = document.getElementById('recordBtn');
            if (recordBtn) recordBtn.style.display = 'inline-flex';
            
            this.serialManager.startReading();
        } catch (e) {
            console.error('串口连接失败:', e);
            alert(e.message || '串口连接失败');
        }
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
        // 更新UI
        this.uiManager.updateSensorData(sensorData);

        // 录制数据
        if (this.recordingManager.getStatus()) {
            this.recordingManager.addDataPoint(sensorData.gyr);
        }

        // 磁力计校准数据回调
        if (window.processMagDataForCalibration) {
            window.processMagDataForCalibration(
                sensorData.mag.x, 
                sensorData.mag.y, 
                sensorData.mag.z
            );
        }

        // 陀螺仪校准数据回调
        if (window.processGyroDataForCalibration) {
            window.processGyroDataForCalibration(
                sensorData.gyr.x, 
                sensorData.gyr.y, 
                sensorData.gyr.z
            );
        }

        // 更新姿态
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

