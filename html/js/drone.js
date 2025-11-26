// 无人机模型模块
class DroneModel {
    constructor(scene) {
        console.log('[DroneModel] Creating drone model');
        
        if (!scene) {
            throw new Error('Scene is required for DroneModel');
        }
        
        this.scene = scene;
        this.droneGroup = new THREE.Group();
        this.propellers = [];
        
        this.init();
        console.log('[DroneModel] Drone model created, propellers:', this.propellers.length);
    }

    init() {
        // 添加飞控坐标系轴线（FLU坐标系）
        // 注意：这里显示的是飞控的逻辑坐标系，不是Three.js的坐标系
        this.addFLUAxes();
        
        // 材质定义
        const matBody = new THREE.MeshStandardMaterial({
            color: 0x34495e,
            roughness: 0.3,
            metalness: 0.5
        });
        const matAccent = new THREE.MeshStandardMaterial({ color: 0xff9f43 });
        const matProp = new THREE.MeshStandardMaterial({
            color: 0xff9f43,
            transparent: true,
            opacity: 0.6,
            side: THREE.DoubleSide
        });
        const matLedRed = new THREE.MeshBasicMaterial({ color: 0xff0000 });
        const matLedGreen = new THREE.MeshBasicMaterial({ color: 0x00ff00 });

        // 1. 机身核心
        const core = new THREE.Mesh(
            new THREE.BoxGeometry(0.8, 0.3, 1.2),
            matBody
        );
        core.castShadow = true;
        this.droneGroup.add(core);

        // 顶盖 (模拟隆起)
        const topCover = new THREE.Mesh(
            new THREE.CylinderGeometry(0.3, 0.4, 0.8, 16),
            matBody
        );
        topCover.rotation.x = Math.PI / 2;
        topCover.scale.set(1, 1, 0.4);
        topCover.position.y = 0.15;
        this.droneGroup.add(topCover);

        // 2. 相机云台
        const camGimbal = new THREE.Mesh(
            new THREE.SphereGeometry(0.25, 32, 32),
            new THREE.MeshStandardMaterial({
                color: 0x111111,
                metalness: 0.9,
                roughness: 0.1
            })
        );
        camGimbal.position.set(0, -0.15, 0.6);
        this.droneGroup.add(camGimbal);

        // 镜头（蓝色，指示机头方向）
        const lens = new THREE.Mesh(
            new THREE.CylinderGeometry(0.1, 0.1, 0.2),
            new THREE.MeshBasicMaterial({color: 0x00a8ff, emissive: 0x00a8ff})
        );
        lens.rotation.x = Math.PI / 2;
        lens.position.set(0, -0.15, 0.85);
        this.droneGroup.add(lens);
        
        // 添加更明显的机头指示箭头
        const arrowGeometry = new THREE.ConeGeometry(0.15, 0.4, 8);
        const arrowMaterial = new THREE.MeshBasicMaterial({
            color: 0x00ff00,
            emissive: 0x00ff00
        });
        const arrow = new THREE.Mesh(arrowGeometry, arrowMaterial);
        arrow.rotation.x = Math.PI / 2;
        arrow.position.set(0, 0, 1.1);
        this.droneGroup.add(arrow);

        // 3. 机臂和螺旋桨
        const armPositions = [
            {x: -1, z: 1, led: matLedRed},   // 左前
            {x: 1, z: 1, led: matLedRed},    // 右前
            {x: -1, z: -1, led: matLedGreen},// 左后
            {x: 1, z: -1, led: matLedGreen}  // 右后
        ];

        armPositions.forEach(pos => {
            this.addArm(pos, matBody, matProp, matAccent);
        });

        this.scene.add(this.droneGroup);
    }

    addFLUAxes() {
        // 创建飞控坐标系轴线（FLU: Forward-Left-Up）
        const axisLength = 2.5;
        const axisWidth = 0.05;
        
        // X轴 = 正面（前）= 蓝色箭头
        const xAxisGeometry = new THREE.CylinderGeometry(axisWidth, axisWidth, axisLength, 8);
        const xAxis = new THREE.Mesh(xAxisGeometry, new THREE.MeshBasicMaterial({
            color: 0x0000ff,  // 蓝色
            emissive: 0x0000ff
        }));
        xAxis.rotation.x = Math.PI / 2;  // 旋转使其沿Z轴（Three.js的前向）
        xAxis.position.z = axisLength / 2;
        this.droneGroup.add(xAxis);
        
        // X轴箭头
        const xArrow = new THREE.Mesh(
            new THREE.ConeGeometry(axisWidth * 3, 0.2, 8),
            new THREE.MeshBasicMaterial({ color: 0x0000ff, emissive: 0x0000ff })
        );
        xArrow.rotation.x = Math.PI / 2;
        xArrow.position.z = axisLength;
        this.droneGroup.add(xArrow);
        
        // Y轴 = 左边 = 红色箭头（指向Three.js的-X方向，即左）
        const yAxisGeometry = new THREE.CylinderGeometry(axisWidth, axisWidth, axisLength, 8);
        const yAxis = new THREE.Mesh(yAxisGeometry, new THREE.MeshBasicMaterial({
            color: 0xff0000,  // 红色
            emissive: 0xff0000
        }));
        yAxis.rotation.z = Math.PI / 2;  // 旋转90度使圆柱沿X方向
        yAxis.position.x = -axisLength / 2;  // 放在负X位置（左边）
        this.droneGroup.add(yAxis);
        
        // Y轴箭头（指向左边 = -X方向）
        const yArrow = new THREE.Mesh(
            new THREE.ConeGeometry(axisWidth * 3, 0.2, 8),
            new THREE.MeshBasicMaterial({ color: 0xff0000, emissive: 0xff0000 })
        );
        yArrow.rotation.z = -Math.PI / 2;  // 圆锥尖端朝-X方向
        yArrow.position.x = -axisLength;
        this.droneGroup.add(yArrow);
        
        // Z轴 = 上 = 绿色箭头
        const zAxisGeometry = new THREE.CylinderGeometry(axisWidth, axisWidth, axisLength, 8);
        const zAxis = new THREE.Mesh(zAxisGeometry, new THREE.MeshBasicMaterial({
            color: 0x00ff00,  // 绿色
            emissive: 0x00ff00
        }));
        zAxis.position.y = axisLength / 2;
        this.droneGroup.add(zAxis);
        
        // Z轴箭头
        const zArrow = new THREE.Mesh(
            new THREE.ConeGeometry(axisWidth * 3, 0.2, 8),
            new THREE.MeshBasicMaterial({ color: 0x00ff00, emissive: 0x00ff00 })
        );
        zArrow.position.y = axisLength;
        this.droneGroup.add(zArrow);
        
        // 添加轴标签（文字）
        console.log('[DroneModel] FLU坐标系: X(前/蓝) Y(左/红) Z(上/绿)');
    }

    addArm(pos, matBody, matProp, matAccent) {
        const armLength = CONFIG.drone.armLength;
        
        // 机臂杆
        const arm = new THREE.Mesh(
            new THREE.BoxGeometry(0.15, 0.1, armLength),
            matBody
        );
        const angle = Math.atan2(pos.x, pos.z);
        arm.rotation.y = angle;
        arm.position.y = 0;
        arm.position.x = pos.x * 0.6;
        arm.position.z = pos.z * 0.6;
        arm.castShadow = true;
        this.droneGroup.add(arm);

        // 电机座
        const motorBase = new THREE.Mesh(
            new THREE.CylinderGeometry(0.2, 0.2, 0.25, 16),
            matBody
        );
        motorBase.position.set(pos.x * 1.2, 0.1, pos.z * 1.2);
        motorBase.castShadow = true;
        this.droneGroup.add(motorBase);

        // 螺旋桨
        const propGeo = new THREE.BoxGeometry(0.15, 0.01, 1.6);
        const prop = new THREE.Mesh(propGeo, matProp);
        prop.position.set(pos.x * 1.2, 0.25, pos.z * 1.2);
        this.droneGroup.add(prop);
        this.propellers.push(prop);

        // 桨帽
        const cap = new THREE.Mesh(
            new THREE.ConeGeometry(0.08, 0.15, 16),
            matAccent
        );
        cap.position.set(pos.x * 1.2, 0.3, pos.z * 1.2);
        this.droneGroup.add(cap);

        // LED 灯
        const led = new THREE.Mesh(
            new THREE.SphereGeometry(0.08),
            pos.led
        );
        led.position.set(pos.x * 1.1, -0.1, pos.z * 1.1);
        this.droneGroup.add(led);
    }

    updateOrientation(pitchRad, rollRad, yawRad) {
        // 坐标系映射说明：
        // ================================================================
        // 飞控坐标系 (FLU - Forward/Left/Up)：
        //   X轴 = 正面（前）
        //   Y轴 = 左边
        //   Z轴 = 上
        //   Roll  = 绕X轴（前轴）旋转 - 左右倾斜
        //   Pitch = 绕Y轴（左轴）旋转 - 前后俯仰
        //   Yaw   = 绕Z轴（上轴）旋转 - 水平旋转
        //
        // Three.js坐标系 (RUF - Right/Up/Forward)：
        //   X轴 = 右
        //   Y轴 = 上
        //   Z轴 = 前（朝向观察者）
        //   模型默认机头朝+Z方向
        //
        // 映射关系：
        //   飞控X（前）→ Three.js Z（前）
        //   飞控Y（左）→ Three.js -X（左）
        //   飞控Z（上）→ Three.js Y（上）
        // ================================================================
        
        // ================================================================
        // 坐标系映射（根据实际测试调整）
        // ================================================================
        // 测试方法：
        //   1. Roll测试：将飞控向右倾斜 → 模型应该向右倾斜（红灯下沉）
        //   2. Pitch测试：将飞控机头抬起 → 模型应该机头抬起（绿箭头向上）
        //   3. Yaw测试：将飞控顺时针旋转 → 模型应该顺时针旋转（从上往下看）
        // ================================================================
        
        // Three.js使用的是欧拉角（XYZ内旋，即旋转顺序：绕X->绕Y->绕Z）
        this.droneGroup.rotation.order = 'YZX';  // 旋转顺序：Yaw->Roll->Pitch
        
        // 应用反转（根据调试面板设置）
        const invertSign = window.coordInvert || { roll: false, pitch: false, yaw: true };
        const rollSign = invertSign.roll ? -1 : 1;
        const pitchSign = invertSign.pitch ? -1 : 1;
        const yawSign = invertSign.yaw ? -1 : 1;
        
        // 飞控FLU (X前Y左Z上) → Three.js (X右Y上Z前)
        this.droneGroup.rotation.x = pitchSign * pitchRad;   // Pitch（绕Y左轴）→ 绕X轴
        this.droneGroup.rotation.y = yawSign * (-yawRad);    // Yaw反转（根据测试确定）
        this.droneGroup.rotation.z = rollSign * rollRad;     // Roll（绕X前轴）→ 绕Z轴
    }

    animatePropellers() {
        this.propellers.forEach((p, i) => {
            const dir = i % 2 === 0 ? 1 : -1;
            p.rotation.y += 0.8 * dir;
        });
    }

    getDroneGroup() {
        return this.droneGroup;
    }
}

// ================================================================
// 坐标系调试工具
// ================================================================
let coordInvert = { roll: false, pitch: false, yaw: false };

function toggleCoordDebug() {
    const panel = document.getElementById('coordDebugPanel');
    const btn = document.getElementById('coordDebugBtn');
    
    if (panel.classList.contains('hidden')) {
        panel.classList.remove('hidden');
        btn.style.display = 'none';
    } else {
        panel.classList.add('hidden');
        btn.style.display = 'flex';
    }
    
    if (typeof lucide !== 'undefined') {
        lucide.createIcons();
    }
}

// 监听复选框变化
document.addEventListener('DOMContentLoaded', () => {
    const invertRoll = document.getElementById('invertRoll');
    const invertPitch = document.getElementById('invertPitch');
    const invertYaw = document.getElementById('invertYaw');
    
    if (invertRoll) {
        invertRoll.addEventListener('change', (e) => {
            coordInvert.roll = e.target.checked;
            console.log('[CoordDebug] Roll反转:', coordInvert.roll);
        });
    }
    
    if (invertPitch) {
        invertPitch.addEventListener('change', (e) => {
            coordInvert.pitch = e.target.checked;
            console.log('[CoordDebug] Pitch反转:', coordInvert.pitch);
        });
    }
    
    if (invertYaw) {
        invertYaw.addEventListener('change', (e) => {
            coordInvert.yaw = e.target.checked;
            console.log('[CoordDebug] Yaw反转:', coordInvert.yaw);
        });
    }
});

// 导出函数
window.toggleCoordDebug = toggleCoordDebug;
window.coordInvert = coordInvert;
