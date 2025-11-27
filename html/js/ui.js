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
            statusDot: document.getElementById('status-dot'),
            // RC / ELRS
            rcStickLeft: document.getElementById('rc-stick-left'),
            rcStickRight: document.getElementById('rc-stick-right'),
            rcBarThr: document.getElementById('rc-bar-thr'),
            rcBarRoll: document.getElementById('rc-bar-roll'),
            rcBarPitch: document.getElementById('rc-bar-pitch'),
            rcBarYaw: document.getElementById('rc-bar-yaw'),
            rcBarThrVal: document.getElementById('rc-bar-thr-val'),
            rcBarRollVal: document.getElementById('rc-bar-roll-val'),
            rcBarPitchVal: document.getElementById('rc-bar-pitch-val'),
            rcBarYawVal: document.getElementById('rc-bar-yaw-val'),
            rcAux: [
                document.getElementById('rc-aux-1'),
                document.getElementById('rc-aux-2'),
                document.getElementById('rc-aux-3'),
                document.getElementById('rc-aux-4'),
                document.getElementById('rc-aux-5'),
                document.getElementById('rc-aux-6'),
                document.getElementById('rc-aux-7'),
                document.getElementById('rc-aux-8')
            ],
            rcLinkState: document.getElementById('rc-link-state'),
            rcRssi: document.getElementById('rc-rssi'),
            rcLq: document.getElementById('rc-lq')
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

        // 更新 RC 显示
        this.updateRC(sensorData.rc);
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

    updateRC(rc) {
        if (!rc) return;
        const clamp = (v, lo, hi) => Math.min(Math.max(v, lo), hi);

        // 摇杆可视化
        const stickLeft = this.elements.rcStickLeft;
        if (stickLeft) {
            const x = clamp(rc.yaw, -1, 1) * 40;          // 左右
            const y = (0.5 - clamp(rc.throttle, 0, 1)) * 80; // 油门向上
            stickLeft.style.transform = `translate(calc(-50% + ${x}px), calc(-50% + ${y}px))`;
        }
        const stickRight = this.elements.rcStickRight;
        if (stickRight) {
            const x = clamp(rc.roll, -1, 1) * 40;
            const y = clamp(-rc.pitch, -1, 1) * 40; // 正pitch让摇杆向上
            stickRight.style.transform = `translate(calc(-50% + ${x}px), calc(-50% + ${y}px))`;
        }

        // 条形值
        const setCenteredBar = (el, val) => {
            if (!el) return;
            const v = clamp(val, -1, 1);
            const width = Math.abs(v) * 50; // 百分比
            const left = 50 + Math.min(v, 0) * 50;
            el.style.width = `${width}%`;
            el.style.left = `${left}%`;
        };
        const setPositiveBar = (el, val) => {
            if (!el) return;
            const v = clamp(val, 0, 1) * 100;
            el.style.width = `${v}%`;
            el.style.left = '0%';
        };

        setPositiveBar(this.elements.rcBarThr, rc.throttle);
        setCenteredBar(this.elements.rcBarRoll, rc.roll);
        setCenteredBar(this.elements.rcBarPitch, rc.pitch);
        setCenteredBar(this.elements.rcBarYaw, rc.yaw);

        if (this.elements.rcBarThrVal) {
            this.elements.rcBarThrVal.innerText = `${Math.round(clamp(rc.throttle,0,1)*100)}%`;
        }
        if (this.elements.rcBarRollVal) this.elements.rcBarRollVal.innerText = rc.roll.toFixed(2);
        if (this.elements.rcBarPitchVal) this.elements.rcBarPitchVal.innerText = rc.pitch.toFixed(2);
        if (this.elements.rcBarYawVal) this.elements.rcBarYawVal.innerText = rc.yaw.toFixed(2);

        // AUX
        if (this.elements.rcAux && Array.isArray(this.elements.rcAux)) {
            for (let i = 0; i < this.elements.rcAux.length; i++) {
                const el = this.elements.rcAux[i];
                if (el) {
                    const v = rc.aux && rc.aux[i] !== undefined ? clamp(rc.aux[i], 0, 1) : 0;
                    el.style.width = `${v * 100}%`;
                }
            }
        }

        // 链路状态
        if (this.elements.rcLinkState) {
            const alive = (Date.now() - rc.lastTs) < 600;
            this.elements.rcLinkState.innerText = `Link: ${alive ? 'Active' : '--'}`;
        }
        if (this.elements.rcRssi) {
            this.elements.rcRssi.innerText = `RSSI: ${rc.rssi !== null ? rc.rssi.toFixed(0) + ' dB' : '--'}`;
        }
        if (this.elements.rcLq) {
            this.elements.rcLq.innerText = `LQ: ${rc.lq !== null ? rc.lq.toFixed(0) + '%' : '--'}`;
        }
    }
}
