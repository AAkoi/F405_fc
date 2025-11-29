// 串口通信模块
class SerialManager {
    constructor() {
        this.port = null;
        this.reader = null;
        this.mockInterval = null;
        this.sensorData = {
            acc: {x: 0, y: 0, z: 0},
            gyr: {x: 0, y: 0, z: 0},
            mag: {x: 0, y: 0, z: 0},
            bar: {tempDeci: 0, pressurePa: 0, altDeci: 0},
            imuTempDeci: 0,
            tof: { distanceMm: null, ts: 0 },
            bat: { voltage: null },
            // 单片机计算的姿态角（从 ATTITUDE_FULL 消息获取）
            attitude: {roll: 0, pitch: 0, yaw: 0, fromMCU: false},
            // ELRS/CRSF RC 数据（归一化）
            rc: {
                roll: 0, pitch: 0, yaw: 0, throttle: 0,
                aux: [0, 0, 0, 0],
                rssi: null, lq: null,
                lastTs: 0
            },
            lastUpdateType: 'init'
        };
        this.onDataUpdate = null;
        this.onLine = null; // 原始文本行回调（用于控制台输出）
    }

    async connect() {
        if (!navigator.serial) {
            throw new Error("Browser not supported (Use Chrome/Edge)");
        }
        
        this.port = await navigator.serial.requestPort();
        await this.port.open({ baudRate: CONFIG.serial.baudRate });
        return true;
    }

    async startReading() {
        const decoder = new TextDecoderStream();
        this.port.readable.pipeTo(decoder.writable);
        this.reader = decoder.readable.getReader();
        
        let buffer = "";
        while (true) {
            const { value, done } = await this.reader.read();
            if (done) break;
            if (value) {
                buffer += value;
                buffer = this.processBuffer(buffer);
            }
        }
    }

    processBuffer(buffer) {
        let lines = buffer.split('\n');
        const leftover = lines.pop();
        
        for (let line of lines) {
            this.parseLine(line.trim());
        }
        
        return leftover;
    }

    parseLine(line) {
        if (!line) return;
        if (this.onLine) {
            this.onLine(line);
        }
        
        // ToF 距离（毫米）：TOF,timestamp,distMm
        if (line.startsWith('TOF,')) {
            const parts = line.split(',');
            if (parts.length >= 3) {
                const dist = parseInt(parts[2]);
                if (!Number.isNaN(dist)) {
                    this.sensorData.tof.distanceMm = dist;
                    this.sensorData.tof.ts = Date.now();
                    this.sensorData.lastUpdateType = 'tof';
                    if (this.onDataUpdate) {
                        this.onDataUpdate(this.sensorData);
                    }
                    return;
                }
            }
        }

        // ELRS/CRSF RC 数据（归一化，roll/pitch/yaw -1..1，thr 0..1）
        // 格式: ELRS_RC,ts,roll,pitch,yaw,thr,aux1,aux2,aux3,aux4,rssi,lq
        if (line.startsWith('ELRS_RC,')) {
            const parts = line.split(',');
            if (parts.length >= 6) {
                const roll = parseFloat(parts[2]);
                const pitch = parseFloat(parts[3]);
                const yaw = parseFloat(parts[4]);
                const thr = parseFloat(parts[5]);
                this.sensorData.rc.roll = Number.isNaN(roll) ? 0 : roll;
                this.sensorData.rc.pitch = Number.isNaN(pitch) ? 0 : pitch;
                this.sensorData.rc.yaw = Number.isNaN(yaw) ? 0 : yaw;
                this.sensorData.rc.throttle = Number.isNaN(thr) ? 0 : thr;

                for (let i = 0; i < 4; i++) {
                    const idx = 6 + i;
                    if (parts.length > idx) {
                        const val = parseFloat(parts[idx]);
                        this.sensorData.rc.aux[i] = Number.isNaN(val) ? 0 : val;
                    } else {
                        this.sensorData.rc.aux[i] = 0;
                    }
                }
                if (parts.length > 10) {
                    const rssi = parseFloat(parts[10]);
                    const lq = parseFloat(parts[11]);
                    this.sensorData.rc.rssi = Number.isNaN(rssi) ? null : rssi;
                    this.sensorData.rc.lq = Number.isNaN(lq) ? null : lq;
                }
                this.sensorData.rc.lastTs = Date.now();
                this.sensorData.lastUpdateType = 'rc';

                if (this.onDataUpdate) {
                    this.onDataUpdate(this.sensorData);
                }
                return;
            }
        }
        
        // 优先解析 ATTITUDE_FULL 格式（单片机计算的姿态）
        // 格式: ATTITUDE_FULL,时间,Roll,Pitch,Yaw,ax,ay,az,gx,gy,gz,mx,my,mz
        if (line.startsWith('ATTITUDE_FULL,')) {
            const parts = line.split(',');
            if (parts.length >= 14) {
                const roll = parseFloat(parts[2]);
                const pitch = parseFloat(parts[3]);
                const yaw = parseFloat(parts[4]);
                const ax = parseFloat(parts[5]);
                const ay = parseFloat(parts[6]);
                const az = parseFloat(parts[7]);
                const gx = parseFloat(parts[8]);
                const gy = parseFloat(parts[9]);
                const gz = parseFloat(parts[10]);
                const mx = parseInt(parts[11]);
                const my = parseInt(parts[12]);
                const mz = parseInt(parts[13]);
                
                this.sensorData.attitude.roll = roll;
                this.sensorData.attitude.pitch = pitch;
                this.sensorData.attitude.yaw = yaw;
                this.sensorData.attitude.fromMCU = true;
                
                this.sensorData.acc.x = ax;
                this.sensorData.acc.y = ay;
                this.sensorData.acc.z = az;
                this.sensorData.gyr.x = gx;
                this.sensorData.gyr.y = gy;
                this.sensorData.gyr.z = gz;
                this.sensorData.mag.x = mx;
                this.sensorData.mag.y = my;
                this.sensorData.mag.z = mz;
                this.sensorData.lastUpdateType = 'attitude';
                
                if (this.onDataUpdate) {
                    this.onDataUpdate(this.sensorData);
                }
                return;
            }
        }
        
        // 兼容旧格式（管道分隔）
        const parts = line.split('|').map(s => s.trim());
        parts.forEach(p => {
            if (p.startsWith('ACC:')) {
                const nums = p.slice(4).trim().split(/\s+/).map(n => parseInt(n));
                if (nums.length === 3 && nums.every(n => !Number.isNaN(n))) {
                    [this.sensorData.acc.x, this.sensorData.acc.y, this.sensorData.acc.z] = nums;
                }
            } else if (p.startsWith('GYR:')) {
                const nums = p.slice(4).trim().split(/\s+/).map(n => parseInt(n));
                if (nums.length === 3 && nums.every(n => !Number.isNaN(n))) {
                    [this.sensorData.gyr.x, this.sensorData.gyr.y, this.sensorData.gyr.z] = nums;
                }
            } else if (p.startsWith('MAG:')) {
                const nums = p.slice(4).trim().split(/\s+/).map(n => parseInt(n));
                if (nums.length === 3 && nums.every(n => !Number.isNaN(n))) {
                    [this.sensorData.mag.x, this.sensorData.mag.y, this.sensorData.mag.z] = nums;
                }
            } else if (p.startsWith('BAR:')) {
                const nums = p.slice(4).trim().split(/\s+/).map(n => parseInt(n));
                if (nums.length === 3 && nums.every(n => !Number.isNaN(n))) {
                    [this.sensorData.bar.tempDeci, this.sensorData.bar.pressurePa, this.sensorData.bar.altDeci] = nums;
                }
            } else if (p.startsWith('T:')) {
                const val = parseInt(p.slice(2).trim());
                if (!Number.isNaN(val)) this.sensorData.imuTempDeci = val;
            } else if (p.startsWith('VBAT:') || p.startsWith('BAT:')) {
                const val = parseFloat(p.split(':')[1]);
                if (!Number.isNaN(val)) {
                    this.sensorData.bat.voltage = val;
                }
            }
        });

        this.sensorData.lastUpdateType = 'sensor';
        
        if (this.onDataUpdate) {
            this.onDataUpdate(this.sensorData);
        }
    }

    getSensorData() {
        return this.sensorData;
    }

    startMockMode() {
        this.stopMockMode();
        this.port = { mock: true };
        let t = 0;
        this.mockInterval = setInterval(() => {
            t += 0.15;
            const now = Date.now();

            // Attitude + raw sensors
            const roll = Math.sin(t) * 35;
            const pitch = Math.cos(t * 0.8) * 25;
            const yaw = ((t * 30) % 360);
            const ax = Math.sin(t) * 0.5;
            const ay = Math.cos(t * 0.7) * 0.5;
            const az = 9.8 + Math.sin(t * 0.5) * 0.2;
            const gx = Math.sin(t * 1.3) * 120;
            const gy = Math.cos(t * 1.1) * 120;
            const gz = Math.sin(t * 0.9) * 180;
            const mx = Math.sin(t) * 300;
            const my = Math.cos(t * 0.6) * 300;
            const mz = Math.sin(t * 0.4) * 300;

            this.parseLine(`ATTITUDE_FULL,${now},${roll.toFixed(2)},${pitch.toFixed(2)},${yaw.toFixed(2)},${ax.toFixed(3)},${ay.toFixed(3)},${az.toFixed(3)},${gx.toFixed(2)},${gy.toFixed(2)},${gz.toFixed(2)},${Math.round(mx)},${Math.round(my)},${Math.round(mz)}`);

            // Baro + temp
            const tempDeci = 420 + Math.sin(t * 0.4) * 15; // around 42C
            const pressurePa = 101300 + Math.sin(t * 0.2) * 400;
            const altDeci = 120 + Math.sin(t * 0.3) * 20;
            this.parseLine(`BAR: ${Math.round(tempDeci)} ${Math.round(pressurePa)} ${Math.round(altDeci)}`);
            this.parseLine(`T:${Math.round(tempDeci)}`);

            // ToF distance
            const dist = 500 + Math.sin(t * 1.5) * 400 + (Math.random() * 80 - 40);
            this.parseLine(`TOF,${now},${Math.max(30, Math.round(dist))}`);

            // RC
            const rcRoll = Math.sin(t) * 0.5;
            const rcPitch = Math.cos(t * 0.9) * 0.5;
            const rcYaw = Math.sin(t * 0.7) * 0.5;
            const rcThr = (Math.sin(t * 0.5) + 1) / 2;
            this.parseLine(`ELRS_RC,${now},${rcRoll.toFixed(2)},${rcPitch.toFixed(2)},${rcYaw.toFixed(2)},${rcThr.toFixed(2)},0.2,0.6,0.9,0.1,90,99`);

            // Battery (simple saw wave)
            const vbat = 15.5 + Math.sin(t * 0.2) * 1.2;
            this.parseLine(`BAT:${vbat.toFixed(2)}`);

            if (this.onLine) {
                this.onLine(`[MOCK] tick ${now}`);
            }
        }, 150);
    }

    stopMockMode() {
        if (this.mockInterval) {
            clearInterval(this.mockInterval);
            this.mockInterval = null;
        }
        this.port = null;
    }

    async disconnect() {
        this.stopMockMode();
        if (this.reader) {
            await this.reader.cancel();
        }
        if (this.port) {
            try { await this.port.close(); } catch (e) { /* ignore close errors */ }
        }
        this.reader = null;
        this.port = null;
    }
}
