// 配置文件
const CONFIG = {
    serial: {
        baudRate: 115200,
        timeout: 1000
    },
    camera: {
        fov: 45,
        near: 0.1,
        far: 1000,
        distance: 12
    },
    drone: {
        armLength: 1.8,
        bodySize: { width: 0.8, height: 0.3, depth: 1.2 }
    },
    scene: {
        fogDensity: 0.02,
        fogColor: 0xeef2f3,
        gridSize: 30,
        gridDivisions: 30
    },
    controls: {
        enableDamping: true,
        dampingFactor: 0.05
    },
    sensors: {
        gyroScale: 1,
        accelScale: 1,
        magScale: 1
    }
};

// 导出配置
if (typeof module !== 'undefined' && module.exports) {
    module.exports = CONFIG;
}

