// 串口通信模块
class SerialManager {
    constructor() {
        this.port = null;
        this.reader = null;
        this.sensorData = {
            acc: {x: 0, y: 0, z: 0},
            gyr: {x: 0, y: 0, z: 0},
            mag: {x: 0, y: 0, z: 0},
            bar: {tempDeci: 0, pressurePa: 0, altDeci: 0},
            imuTempDeci: 0,
            // 单片机计算的姿态角（从 ATTITUDE_FULL 消息获取）
            attitude: {roll: 0, pitch: 0, yaw: 0, fromMCU: false}
        };
        this.onDataUpdate = null;
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
        
        // 优先解析 ATTITUDE_FULL 格式（单片机计算的姿态）
        // 格式: ATTITUDE_FULL,时间戳,Roll,Pitch,Yaw,ax,ay,az,gx,gy,gz,mx,my,mz
        if (line.startsWith('ATTITUDE_FULL,')) {
            const parts = line.split(',');
            if (parts.length >= 14) {
                const timestamp = parseInt(parts[1]);
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
                
                // 保存单片机计算的姿态角
                this.sensorData.attitude.roll = roll;
                this.sensorData.attitude.pitch = pitch;
                this.sensorData.attitude.yaw = yaw;
                this.sensorData.attitude.fromMCU = true;  // 标记来自单片机
                
                // 保存原始传感器数据（用于显示）
                this.sensorData.acc.x = ax;
                this.sensorData.acc.y = ay;
                this.sensorData.acc.z = az;
                this.sensorData.gyr.x = gx;
                this.sensorData.gyr.y = gy;
                this.sensorData.gyr.z = gz;
                this.sensorData.mag.x = mx;
                this.sensorData.mag.y = my;
                this.sensorData.mag.z = mz;
                
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
            }
        });
        
        if (this.onDataUpdate) {
            this.onDataUpdate(this.sensorData);
        }
    }

    getSensorData() {
        return this.sensorData;
    }

    async disconnect() {
        if (this.reader) {
            await this.reader.cancel();
        }
        if (this.port) {
            await this.port.close();
        }
    }
}

