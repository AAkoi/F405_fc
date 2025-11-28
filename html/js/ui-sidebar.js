class TelemetrySidebar {
    constructor() {
        this.nodes = {
            gyro: null,
            acc: null,
            mag: null,
            attitude: {
                roll: document.getElementById('att-r'),
                pitch: document.getElementById('att-p'),
                yaw: document.getElementById('att-y'),
                rollBar: document.getElementById('vis-r'),
                pitchBar: document.getElementById('vis-p'),
                yawBar: document.getElementById('vis-y')
            },
            env: {
                temp: document.getElementById('val-temp'),
                baro: document.getElementById('val-baro')
            },
            tof: {
                panel: document.getElementById('tof-panel'),
                value: document.getElementById('tof-val'),
                bar: document.getElementById('tof-bar')
            }
        };

        this.lastTofTs = 0;
        this.tofTimeoutMs = 1500;

        this.setConnectionState(false);
    }

    updateSensors(sensorData) {
        if (!sensorData) return;

        // Environment
        const tempC = (sensorData.bar && typeof sensorData.bar.tempDeci === 'number')
            ? sensorData.bar.tempDeci / 10
            : (typeof sensorData.imuTempDeci === 'number' ? sensorData.imuTempDeci / 10 : null);
        if (this.nodes.env.temp && tempC !== null) {
            this.nodes.env.temp.innerText = tempC.toFixed(1);
        }
        const pressHpa = (sensorData.bar && typeof sensorData.bar.pressurePa === 'number')
            ? sensorData.bar.pressurePa / 100
            : null;
        if (this.nodes.env.baro && pressHpa !== null) {
            this.nodes.env.baro.innerText = pressHpa.toFixed(1);
        }

        // ToF (optional)
        const tof = sensorData.tof;
        if (tof && typeof tof.distanceMm === 'number' && Number.isFinite(tof.distanceMm) && tof.ts) {
            const fresh = Date.now() - tof.ts < this.tofTimeoutMs;
            if (fresh) {
                this.updateTof(tof.distanceMm, tof.ts);
            } else {
                this.checkTofTimeout();
            }
        } else {
            this.checkTofTimeout();
        }
    }

    updateOrientation(orientation) {
        if (!orientation) return;
        const { roll, pitch } = orientation;
        const yaw = this.normalizeYaw(orientation.yaw);

        if (this.nodes.attitude.roll) {
            this.nodes.attitude.roll.innerText = `${roll.toFixed(1)}°`;
        }
        if (this.nodes.attitude.pitch) {
            this.nodes.attitude.pitch.innerText = `${pitch.toFixed(1)}°`;
        }
        if (this.nodes.attitude.yaw) {
            this.nodes.attitude.yaw.innerText = `${yaw.toFixed(0)}°`;
        }

        // Bars
        this.setCenteredBar(this.nodes.attitude.rollBar, roll, 60);
        this.setCenteredBar(this.nodes.attitude.pitchBar, pitch, 60);
        if (this.nodes.attitude.yawBar) {
            const pct = Math.max(0, Math.min(1, yaw / 360));
            this.nodes.attitude.yawBar.style.width = `${pct * 100}%`;
            this.nodes.attitude.yawBar.style.left = '0';
        }
    }

    setConnectionState(connected) {
        if (!connected && this.nodes.tof.panel) {
            this.nodes.tof.panel.classList.add('is-offline');
            if (this.nodes.tof.value) this.nodes.tof.value.innerText = '--';
            this.lastTofTs = 0;
        }
    }

    updateTof(distanceMm, ts = Date.now()) {
        if (!this.nodes.tof.panel) return;
        this.nodes.tof.panel.classList.remove('is-offline');
        this.lastTofTs = ts;

        const d = Math.max(0, distanceMm);
        if (this.nodes.tof.value) {
            this.nodes.tof.value.innerText = d.toFixed(0);
        }
        if (this.nodes.tof.bar) {
            const pct = Math.min(1, d / 2000); // assume 0-2m range
            this.nodes.tof.bar.style.transform = `scaleX(${pct})`;
            this.nodes.tof.bar.style.backgroundColor = d < 300 ? '#ef4444' : '#f43f5e';
        }
    }

    checkTofTimeout() {
        if (!this.nodes.tof.panel) return;
        if (this.lastTofTs === 0) return;
        if (Date.now() - this.lastTofTs > this.tofTimeoutMs) {
            this.nodes.tof.panel.classList.add('is-offline');
            if (this.nodes.tof.value) this.nodes.tof.value.innerText = '--';
        }
    }

    setText(node, value, digits = 0) {
        if (!node || value === undefined || value === null || Number.isNaN(value)) return;
        node.innerText = Number(value).toFixed(digits);
    }

    setCenteredBar(node, value, range) {
        if (!node || range === 0) return;
        const pct = Math.max(-1, Math.min(1, value / range)) * 50;
        node.style.width = `${Math.abs(pct)}%`;
        node.style.left = `${50 + Math.min(pct, 0)}%`;
    }

    normalizeYaw(yaw) {
        const y = Number.isFinite(yaw) ? yaw : 0;
        return ((y % 360) + 360) % 360;
    }
}
