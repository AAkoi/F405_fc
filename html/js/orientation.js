// 姿态解算模块
class OrientationCalculator {
    constructor() {
        this.orientation = { roll: 0, pitch: 0, yaw: 0 };
    }

    update(sensorData) {
        const ax = sensorData.acc.x;
        const ay = sensorData.acc.y;
        const az = sensorData.acc.z;
        const mx = sensorData.mag.x;
        const my = sensorData.mag.y;
        const mz = sensorData.mag.z;
        
        // 计算 Pitch/Roll
        const pitchRad = Math.atan2(ay, Math.sqrt(ax*ax + az*az));
        const rollRad = Math.atan2(-ax, Math.sqrt(ay*ay + az*az));
        
        // 计算 Yaw (使用磁力计，带倾斜补偿)
        let yawRad = 0;
        if (mx !== 0 || my !== 0 || mz !== 0) {
            const cosRoll = Math.cos(rollRad);
            const sinRoll = Math.sin(rollRad);
            const cosPitch = Math.cos(pitchRad);
            const sinPitch = Math.sin(pitchRad);
            
            // 补偿后的磁力计读数
            const magX = mx * cosPitch + my * sinRoll * sinPitch + mz * cosRoll * sinPitch;
            const magY = my * cosRoll - mz * sinRoll;
            
            // 计算 yaw 角
            yawRad = Math.atan2(-magY, magX);
        }
        
        // 转换为角度并存储
        this.orientation.pitch = pitchRad * (180 / Math.PI);
        this.orientation.roll = rollRad * (180 / Math.PI);
        this.orientation.yaw = yawRad * (180 / Math.PI);

        return {
            pitch: pitchRad,
            roll: rollRad,
            yaw: yawRad
        };
    }

    getOrientation() {
        return this.orientation;
    }
}

