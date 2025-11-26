// UI 更新模块
class UIManager {
    constructor() {
        this.elements = {
            ax: document.getElementById('val-ax'),
            ay: document.getElementById('val-ay'),
            az: document.getElementById('val-az'),
            gx: document.getElementById('val-gx'),
            gy: document.getElementById('val-gy'),
            gz: document.getElementById('val-gz'),
            mx: document.getElementById('val-mx'),
            my: document.getElementById('val-my'),
            mz: document.getElementById('val-mz'),
            barTemp: document.getElementById('val-btemp'),
            barPress: document.getElementById('val-press'),
            barAlt: document.getElementById('val-alt'),
            imuTemp: document.getElementById('val-imu-temp'),
            roll: document.getElementById('val-roll'),
            pitch: document.getElementById('val-pitch'),
            yaw: document.getElementById('val-yaw'),
            statusText: document.getElementById('status-text'),
            statusDot: document.getElementById('status-dot')
        };
    }

    updateSensorData(sensorData) {
        if (!sensorData) return;

        // 更新加速度计
        if (this.elements.ax) this.elements.ax.innerText = sensorData.acc.x;
        if (this.elements.ay) this.elements.ay.innerText = sensorData.acc.y;
        if (this.elements.az) this.elements.az.innerText = sensorData.acc.z;

        // 更新陀螺仪
        if (this.elements.gx) this.elements.gx.innerText = sensorData.gyr.x;
        if (this.elements.gy) this.elements.gy.innerText = sensorData.gyr.y;
        if (this.elements.gz) this.elements.gz.innerText = sensorData.gyr.z;

        // 更新磁力计
        if (this.elements.mx) this.elements.mx.innerText = sensorData.mag.x;
        if (this.elements.my) this.elements.my.innerText = sensorData.mag.y;
        if (this.elements.mz) this.elements.mz.innerText = sensorData.mag.z;

        // 如果正在校准，传递磁力计数据给校准模块
        if (window.processMagDataForCalibration) {
            // console.log('[UI] 传递磁力计数据:', sensorData.mag); // 可以取消注释查看
            window.processMagDataForCalibration(
                sensorData.mag.x, 
                sensorData.mag.y, 
                sensorData.mag.z
            );
        }
        // 如果正在做陀螺校准，传递陀螺数据
        if (window.processGyroDataForCalibration) {
            window.processGyroDataForCalibration(
                sensorData.gyr.x,
                sensorData.gyr.y,
                sensorData.gyr.z
            );
        }

        // 更新气压计
        if (this.elements.barTemp) {
            this.elements.barTemp.innerText = (sensorData.bar.tempDeci / 10).toFixed(1);
        }
        if (this.elements.barPress) {
            this.elements.barPress.innerText = (sensorData.bar.pressurePa / 100).toFixed(1);
        }
        if (this.elements.barAlt) {
            this.elements.barAlt.innerText = (sensorData.bar.altDeci / 10).toFixed(1);
        }

        // 更新IMU温度
        if (this.elements.imuTemp) {
            this.elements.imuTemp.innerText = (sensorData.imuTempDeci / 10).toFixed(1);
        }
    }

    updateOrientation(orientation) {
        if (!orientation) return;

        if (this.elements.roll) {
            this.elements.roll.innerText = orientation.roll.toFixed(1);
        }
        if (this.elements.pitch) {
            this.elements.pitch.innerText = orientation.pitch.toFixed(1);
        }
        if (this.elements.yaw) {
            this.elements.yaw.innerText = orientation.yaw.toFixed(1);
        }
        
        // 更新调试面板的实时数据
        const debugRoll = document.getElementById('debug-roll');
        const debugPitch = document.getElementById('debug-pitch');
        const debugYaw = document.getElementById('debug-yaw');
        if (debugRoll) debugRoll.textContent = orientation.roll.toFixed(1);
        if (debugPitch) debugPitch.textContent = orientation.pitch.toFixed(1);
        if (debugYaw) debugYaw.textContent = orientation.yaw.toFixed(1);
    }

    setConnectionStatus(connected) {
        if (this.elements.statusText) {
            this.elements.statusText.innerText = connected ? "Connected" : "Disconnected";
        }
        if (this.elements.statusDot) {
            this.elements.statusDot.style.backgroundColor = connected ? "#2ecc71" : "#e74c3c";
        }
        
        // 显示/隐藏磁力计校准按钮
        const magCalibBtn = document.getElementById('magCalibBtn');
        if (magCalibBtn) {
            if (connected) {
                magCalibBtn.classList.remove('hidden');
            } else {
                magCalibBtn.classList.add('hidden');
            }
        }
        
        // 显示/隐藏姿态复位按钮
        const resetAttitudeBtn = document.getElementById('resetAttitudeBtn');
        if (resetAttitudeBtn) {
            if (connected) {
                resetAttitudeBtn.classList.remove('hidden');
            } else {
                resetAttitudeBtn.classList.add('hidden');
            }
        }
    }

    setRecordingStatus(recording) {
        if (this.elements.statusText) {
            this.elements.statusText.innerText = recording ? "Recording..." : "Connected";
        }
    }
}

