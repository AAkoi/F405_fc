// 无人机模型模块
class DroneModel {
    constructor(scene) {
        this.scene = scene;
        this.droneGroup = new THREE.Group();
        this.propellers = [];
        
        this.init();
    }

    init() {
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

        // 镜头
        const lens = new THREE.Mesh(
            new THREE.CylinderGeometry(0.1, 0.1, 0.1),
            new THREE.MeshBasicMaterial({color: 0x00a8ff})
        );
        lens.rotation.x = Math.PI / 2;
        lens.position.set(0, -0.15, 0.82);
        this.droneGroup.add(lens);

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
        // 线性插值平滑旋转
        this.droneGroup.rotation.x += (pitchRad - this.droneGroup.rotation.x) * 0.1;
        this.droneGroup.rotation.z += (rollRad - this.droneGroup.rotation.z) * 0.1;
        this.droneGroup.rotation.y += (yawRad - this.droneGroup.rotation.y) * 0.1;
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

