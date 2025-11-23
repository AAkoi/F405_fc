// 主应用程序
class DroneVisualizerApp {
    constructor() {
        this.serialManager = new SerialManager();
        this.uiManager = new UIManager();
        this.recordingManager = new RecordingManager();
        this.orientationCalc = new OrientationCalculator();
        this.sceneManager = new SceneManager('canvas-container');
        this.droneModel = new DroneModel(this.sceneManager.getScene());
        
        this.init();
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

        // 视角按钮
        const viewButtons = document.querySelectorAll('.view-btn');
        viewButtons.forEach(btn => {
            btn.addEventListener('click', (e) => {
                const mode = e.target.dataset.mode;
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
            if (recordBtn) recordBtn.style.display = 'inline-block';
            
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
                recordBtn.innerText = '⏹ 停止录制';
                recordBtn.style.background = '#27ae60';
            }
            this.uiManager.setRecordingStatus(true);
        } else {
            // 停止录制
            this.recordingManager.stop();
            if (recordBtn) {
                recordBtn.innerText = '⏺ 开始录制';
                recordBtn.style.background = '#e74c3c';
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

        // 更新姿态
        const orientation = this.orientationCalc.update(sensorData);
        this.uiManager.updateOrientation(this.orientationCalc.getOrientation());
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
    window.app = new DroneVisualizerApp();
});

