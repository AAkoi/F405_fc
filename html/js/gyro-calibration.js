/**
 * 陀螺仪校准可视化（3D 点云）
 * - 采集原始陀螺仪数据，实时显示 3D 点云
 * - 计算零偏（均值），显示校准后点云（减去偏置）
 */

let gyroCalibState = {
    isCalibrating: false,
    samples: [],
    offsets: { x: 0, y: 0, z: 0 }
};

let gyroScene, gyroCamera, gyroRenderer, rawPoints, calPoints, gyroControls;

function initGyroPlot() {
    const canvas = document.getElementById('gyroPlot');
    const width = canvas.clientWidth;
    const height = canvas.clientHeight;

    gyroScene = new THREE.Scene();
    gyroCamera = new THREE.PerspectiveCamera(45, width / height, 0.1, 1000);
    gyroCamera.position.set(40, 40, 40);

    gyroRenderer = new THREE.WebGLRenderer({ canvas, antialias: true });
    gyroRenderer.setSize(width, height);

    gyroControls = new THREE.OrbitControls(gyroCamera, gyroRenderer.domElement);
    gyroControls.enableDamping = true;

    // 轴和网格
    gyroScene.add(new THREE.AxesHelper(20));
    gyroScene.add(new THREE.GridHelper(40, 20));

    // 原始点（红）
    const rawGeo = new THREE.BufferGeometry();
    rawGeo.setAttribute('position', new THREE.Float32BufferAttribute([], 3));
    const rawMat = new THREE.PointsMaterial({ color: 0xff3333, size: 0.6 });
    rawPoints = new THREE.Points(rawGeo, rawMat);
    gyroScene.add(rawPoints);

    // 校准后点（绿）
    const calGeo = new THREE.BufferGeometry();
    calGeo.setAttribute('position', new THREE.Float32BufferAttribute([], 3));
    const calMat = new THREE.PointsMaterial({ color: 0x33ff66, size: 0.6 });
    calPoints = new THREE.Points(calGeo, calMat);
    gyroScene.add(calPoints);

    animateGyroPlot();
}

function animateGyroPlot() {
    requestAnimationFrame(animateGyroPlot);
    if (gyroControls) gyroControls.update();
    if (gyroRenderer && gyroScene && gyroCamera) {
        gyroRenderer.render(gyroScene, gyroCamera);
    }
}

function updateGyroPoints() {
    if (!rawPoints || !calPoints) return;
    const rawPositions = [];
    const calPositions = [];
    const ox = gyroCalibState.offsets.x;
    const oy = gyroCalibState.offsets.y;
    const oz = gyroCalibState.offsets.z;

    gyroCalibState.samples.forEach(s => {
        rawPositions.push(s.x, s.y, s.z);
        calPositions.push(s.x - ox, s.y - oy, s.z - oz);
    });

    rawPoints.geometry.setAttribute('position', new THREE.Float32BufferAttribute(rawPositions, 3));
    rawPoints.geometry.computeBoundingSphere();
    calPoints.geometry.setAttribute('position', new THREE.Float32BufferAttribute(calPositions, 3));
    calPoints.geometry.computeBoundingSphere();
}

function openGyroCalibModal() {
    const modal = document.getElementById('gyroCalibModal');
    modal.style.display = 'flex';
    resetGyroCalibration();
    if (!gyroScene) {
        initGyroPlot();
    }
    lucide.createIcons();
}

function closeGyroCalibModal() {
    const modal = document.getElementById('gyroCalibModal');
    modal.style.display = 'none';
    gyroCalibState.isCalibrating = false;
}

function resetGyroCalibration() {
    gyroCalibState.isCalibrating = false;
    gyroCalibState.samples = [];
    gyroCalibState.offsets = { x: 0, y: 0, z: 0 };
    document.getElementById('gyro-samples').textContent = '0';
    document.getElementById('gyro-offset-x').textContent = '0.0';
    document.getElementById('gyro-offset-y').textContent = '0.0';
    document.getElementById('gyro-offset-z').textContent = '0.0';
    document.getElementById('gyro-code').textContent = 'Attitude_SetGyroBias(0.0f, 0.0f, 0.0f);';
    updateGyroPoints();
}

function startGyroCalibration() {
    gyroCalibState.isCalibrating = true;
    gyroCalibState.samples = [];
    updateGyroPoints();
}

function finishGyroCalibration() {
    if (gyroCalibState.samples.length === 0) return;
    const n = gyroCalibState.samples.length;
    let sx = 0, sy = 0, sz = 0;
    gyroCalibState.samples.forEach(s => { sx += s.x; sy += s.y; sz += s.z; });
    gyroCalibState.offsets = { x: sx / n, y: sy / n, z: sz / n };
    document.getElementById('gyro-offset-x').textContent = gyroCalibState.offsets.x.toFixed(2);
    document.getElementById('gyro-offset-y').textContent = gyroCalibState.offsets.y.toFixed(2);
    document.getElementById('gyro-offset-z').textContent = gyroCalibState.offsets.z.toFixed(2);
    document.getElementById('gyro-code').textContent =
        `Attitude_SetGyroBias(${gyroCalibState.offsets.x.toFixed(2)}f, ${gyroCalibState.offsets.y.toFixed(2)}f, ${gyroCalibState.offsets.z.toFixed(2)}f);`;
    gyroCalibState.isCalibrating = false;
    updateGyroPoints();
}

// 上位机实时回调
function processGyroDataForCalibration(gx, gy, gz) {
    if (!gyroCalibState.isCalibrating) return;
    gyroCalibState.samples.push({ x: gx, y: gy, z: gz });
    document.getElementById('gyro-samples').textContent = gyroCalibState.samples.length;
    updateGyroPoints();
}

window.openGyroCalibModal = openGyroCalibModal;
window.closeGyroCalibModal = closeGyroCalibModal;
window.startGyroCalibration = startGyroCalibration;
window.finishGyroCalibration = finishGyroCalibration;
window.resetGyroCalibration = resetGyroCalibration;
window.processGyroDataForCalibration = processGyroDataForCalibration;
