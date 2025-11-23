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
            imuTempDeci: 0
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

