class UIManager {
    constructor() {
        this.sidebar = typeof TelemetrySidebar !== 'undefined' ? new TelemetrySidebar() : null;
        this.rcMonitor = typeof RcMonitor !== 'undefined' ? new RcMonitor() : null;
        this.alerts = typeof AlertManager !== 'undefined' ? new AlertManager() : null;
        this.imuCard = typeof ImuCard !== 'undefined' ? new ImuCard() : null;
        this.attWidget = typeof AttitudeWidget !== 'undefined' ? new AttitudeWidget() : null;
        this.motorMini = typeof MotorOutputMini !== 'undefined' ? new MotorOutputMini() : null;
        this.connectionState = null;

        this.elements = {
            statusText: null,
            statusDot: null
        };

        // 初始化为断开状态
        this.setConnectionStatus(false);
    }

    updateSensorData(sensorData) {
        if (!sensorData) return;

        const updateType = sensorData.lastUpdateType || 'sensor';

        if (this.sidebar) {
            this.sidebar.updateSensors(sensorData);
        }
        if (this.imuCard) {
            this.imuCard.update(sensorData);
        }

        // 将原始数据提供给校准模块（仅在包含传感器数据时）
        if (updateType === 'attitude' || updateType === 'sensor') {
            if (window.processMagDataForCalibration) {
                window.processMagDataForCalibration(
                    sensorData.mag.x,
                    sensorData.mag.y,
                    sensorData.mag.z
                );
            }
            if (window.processGyroDataForCalibration) {
                window.processGyroDataForCalibration(
                    sensorData.gyr.x,
                    sensorData.gyr.y,
                    sensorData.gyr.z
                );
            }
        }

        // 更新 RC 显示
        if (sensorData.rc) {
            this.updateRC(sensorData.rc);
        }
    }

    updateOrientation(orientation) {
        if (!orientation) return;

        if (this.sidebar) {
            this.sidebar.updateOrientation(orientation);
        }
        if (this.attWidget) {
            this.attWidget.update(orientation);
        }

        // 调试面板
        const debugRoll = document.getElementById('debug-roll');
        const debugPitch = document.getElementById('debug-pitch');
        const debugYaw = document.getElementById('debug-yaw');
        if (debugRoll) debugRoll.textContent = orientation.roll.toFixed(1);
        if (debugPitch) debugPitch.textContent = orientation.pitch.toFixed(1);
        if (debugYaw) debugYaw.textContent = orientation.yaw.toFixed(1);
    }

    setConnectionStatus(connected) {
        // 状态文本已移除，连接状态由按钮显示

        // 断开时同步 UI 状态
        if (!connected) {
            if (this.rcMonitor) this.rcMonitor.setConnection(false);
            if (this.sidebar) this.sidebar.setConnectionState(false);
        }

        if (this.alerts && this.connectionState !== null && this.connectionState !== connected) {
            this.alerts.showConnectionToast(connected);
        }

        this.connectionState = connected;
    }

    setRecordingStatus(recording) {
        if (this.elements.statusText) {
            this.elements.statusText.innerText = recording ? "Recording..." : "Connected";
        }
    }

    updateRC(rc) {
        if (this.rcMonitor) {
            this.rcMonitor.update(rc);
        }
        if (this.motorMini && this.rcMonitor) {
            this.motorMini.update(rc, this.rcMonitor.getArmState());
        }
    }

    toggleAxisInvertPanel(show) {
        const panel = document.getElementById('axisInvertPop');
        if (!panel) return;
        panel.classList.toggle('active', show);
    }
}
