// 姿态解算模块
class OrientationCalculator {
    constructor() {
        this.orientation = { roll: 0, pitch: 0, yaw: 0 };
        this.useMCUCalculation = true;  // 优先使用单片机计算的姿态
    }

    update(sensorData) {
        // 优先使用单片机计算的姿态角（Mahony滤波器融合结果）
        if (this.useMCUCalculation && sensorData.attitude && sensorData.attitude.fromMCU) {
            // 直接使用单片机计算的欧拉角（度）
            let roll = sensorData.attitude.roll;
            let pitch = sensorData.attitude.pitch;
            let yaw = sensorData.attitude.yaw;
            
            // 应用姿态偏移（如果已复位）
            if (window.applyAttitudeOffset) {
                const adjusted = window.applyAttitudeOffset(roll, pitch, yaw);
                roll = adjusted.roll;
                pitch = adjusted.pitch;
                yaw = adjusted.yaw;
            }
            
            this.orientation.roll = roll;
            this.orientation.pitch = pitch;
            this.orientation.yaw = yaw;
            
            // 返回弧度值（供3D渲染使用）
            return {
                roll: roll * (Math.PI / 180),
                pitch: pitch * (Math.PI / 180),
                yaw: yaw * (Math.PI / 180)
            };
        }
        
        // 备用：上位机简单算法（仅使用加速度计和磁力计）
        const ax = sensorData.acc.x;
        const ay = sensorData.acc.y;
        const az = sensorData.acc.z;
        const mx = sensorData.mag.x;
        const my = sensorData.mag.y;
        const mz = sensorData.mag.z;
        
        // 计算 Pitch/Roll（简化算法，无滤波）
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
    
    // 切换计算模式
    setCalculationMode(useMCU) {
        this.useMCUCalculation = useMCU;
    }

    getOrientation() {
        return this.orientation;
    }
}

