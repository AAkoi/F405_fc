class TelemetrySidebar {
    constructor() {
        this.nodes = {
            gyro: null,
            acc: null,
            mag: null,
            attitude: null,
            env: {
                temp: document.getElementById('nav-temp-val'),
                baro: document.getElementById('nav-press-val'),
                baroIcon: document.getElementById('nav-press-icon')
            },
            tof: {
                item: document.getElementById('nav-tof'),
                value: document.getElementById('nav-tof-val')
            },
            battery: {
                item: document.getElementById('nav-bat'),
                icon: document.getElementById('nav-bat-icon'),
                value: document.getElementById('nav-bat-val')
            }
        };

        this.lastTofTs = 0;
        this.tofTimeoutMs = 1500;

        this.setConnectionState(false);
    }

    updateSensors(sensorData) {
        if (!sensorData) return;

        // Environment - Temperature
        const tempC = (sensorData.bar && typeof sensorData.bar.tempDeci === 'number')
            ? sensorData.bar.tempDeci / 10
            : (typeof sensorData.imuTempDeci === 'number' ? sensorData.imuTempDeci / 10 : null);
        if (this.nodes.env.temp && tempC !== null) {
            this.nodes.env.temp.innerText = tempC.toFixed(1);
            const tempBox = document.getElementById('nav-temp');
            if (tempBox) {
                tempBox.classList.remove('temp-hot', 'temp-normal');
                tempBox.classList.add(tempC > 45 ? 'temp-hot' : 'temp-normal');
            }
        }
        
        // Barometer - Pressure with rotation animation
        const pressHpa = (sensorData.bar && typeof sensorData.bar.pressurePa === 'number')
            ? sensorData.bar.pressurePa / 100
            : null;
        if (this.nodes.env.baro && pressHpa !== null) {
            this.nodes.env.baro.innerText = pressHpa.toFixed(0);
        }
        if (this.nodes.env.baroIcon && pressHpa !== null) {
            const deg = (pressHpa - 1013) * 10;
            this.nodes.env.baroIcon.style.transform = `translateZ(0) rotate(${deg}deg)`;
            
            // Add press-anim class for smooth animation
            const pressBox = document.getElementById('nav-press');
            if (pressBox && !pressBox.classList.contains('press-anim')) {
                pressBox.classList.add('press-anim');
            }
        }

        // Battery (if provided)
        if (sensorData.bat && typeof sensorData.bat.voltage === 'number') {
            this.updateBattery(sensorData.bat.voltage);
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
        // Attitude UI handled elsewhere
        if (!orientation || !this.nodes.attitude) return;
    }

    setConnectionState(connected) {
        if (!connected) {
            this.setTofState(null);
            this.lastTofTs = 0;
        }
    }

    updateTof(distanceMm, ts = Date.now()) {
        this.lastTofTs = ts;
        this.setTofState(distanceMm);
    }

    checkTofTimeout() {
        if (this.lastTofTs === 0) return;
        if (Date.now() - this.lastTofTs > this.tofTimeoutMs) {
            this.setTofState(null);
        }
    }

    setTofState(distanceMm) {
        const item = this.nodes.tof.item;
        const valNode = this.nodes.tof.value;
        if (!item || !valNode) return;

        item.classList.remove('tof-danger', 'tof-far', 'tof-normal');
        if (distanceMm === null || Number.isNaN(distanceMm)) {
            valNode.innerText = '--';
            return;
        }
        const d = Math.max(0, distanceMm);
        valNode.innerText = d.toFixed(0);
        if (d < 100) {
            item.classList.add('tof-danger');
        } else if (d > 1800) {
            item.classList.add('tof-far');
        } else {
            item.classList.add('tof-normal');
        }
    }

    updateBattery(voltage) {
        const item = this.nodes.battery.item;
        const icon = this.nodes.battery.icon;
        const valNode = this.nodes.battery.value;
        if (!item || !icon || !valNode) return;

        valNode.innerText = voltage.toFixed(1);
        item.classList.remove('bat-good', 'bat-warn', 'bat-low');

        if (voltage <= 0) {
            valNode.innerText = '--';
            return;
        }

        if (voltage >= 16.0) {
            item.classList.add('bat-good');
            icon.className = 'fa-solid fa-battery-full icon';
        } else if (voltage >= 15.0) {
            item.classList.add('bat-good');
            icon.className = 'fa-solid fa-battery-three-quarters icon';
        } else if (voltage >= 14.0) {
            item.classList.add('bat-warn');
            icon.className = 'fa-solid fa-battery-half icon';
        } else if (voltage >= 13.0) {
            item.classList.add('bat-warn');
            icon.className = 'fa-solid fa-battery-quarter icon';
        } else {
            item.classList.add('bat-low');
            icon.className = 'fa-solid fa-battery-empty icon';
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
