/**
 * 磁力计校准模块
 * 实现硬铁和软铁校准
 */

// 校准状态
let calibrationState = {
    isCalibrating: false,
    startTime: 0,
    duration: 60000, // 60秒（1分钟）
    samples: [],
    minX: Infinity, maxX: -Infinity,
    minY: Infinity, maxY: -Infinity,
    minZ: Infinity, maxZ: -Infinity,
};

// 校准结果
let calibrationResult = {
    offsetX: 0,
    offsetY: 0,
    offsetZ: 0,
    scaleX: 1,
    scaleY: 1,
    scaleZ: 1,
};

// 3D 绘图
let magScene, magCamera, magRenderer, magControls;
let magRawPoints, magCalPoints;

function setFinishBtnState(enabled) {
    const btn = document.getElementById('finishBtn');
    if (!btn) return;
    btn.disabled = !enabled;
}

function setActiveLayer(activeId) {
    ['calib-step-1', 'calib-step-2', 'calib-step-3'].forEach(id => {
        const el = document.getElementById(id);
        if (!el) return;
        if (id === activeId) {
            el.classList.add('layer-active');
            el.classList.remove('layer-inactive');
            el.classList.remove('hidden');
        } else {
            el.classList.remove('layer-active');
            el.classList.add('layer-inactive');
            el.classList.add('hidden');
        }
    });
}

/**
 * 打开校准模态框
 */
function openMagCalibModal() {
    const modal = document.getElementById('magCalibModal');
    modal.style.display = 'flex';
    resetMagCalibration();
    lucide.createIcons(); // 重新创建图标
}

/**
 * 关闭校准模态框
 */
function closeMagCalibModal() {
    const modal = document.getElementById('magCalibModal');
    modal.style.display = 'none';
    calibrationState.isCalibrating = false;
}

/**
 * 重置校准
 */
function resetMagCalibration() {
    calibrationState.isCalibrating = false;
    calibrationState.samples = [];
    calibrationState.minX = Infinity;
    calibrationState.maxX = -Infinity;
    calibrationState.minY = Infinity;
    calibrationState.maxY = -Infinity;
    calibrationState.minZ = Infinity;
    calibrationState.maxZ = -Infinity;

    // 显示步骤1
    document.getElementById('calib-step-1').classList.remove('hidden');
    document.getElementById('calib-step-2').classList.add('hidden');
    document.getElementById('calib-step-3').classList.add('hidden');
    setActiveLayer('calib-step-1');

    updateMagPlots();
    setFinishBtnState(false);
}

/**
 * 开始校准
 */
function startMagCalibration() {
    console.log('[MagCalib] 开始校准...');

    // 隐藏步骤1，显示步骤2
    document.getElementById('calib-step-1').classList.add('hidden');
    document.getElementById('calib-step-2').classList.remove('hidden');
    setActiveLayer('calib-step-2');

    // 重置数据
    calibrationState.isCalibrating = true;
    console.log('[MagCalib] isCalibrating 已设置为:', calibrationState.isCalibrating);
    calibrationState.startTime = Date.now();
    calibrationState.samples = [];
    calibrationState.minX = Infinity;
    calibrationState.maxX = -Infinity;
    calibrationState.minY = Infinity;
    calibrationState.maxY = -Infinity;
    calibrationState.minZ = Infinity;
    calibrationState.maxZ = -Infinity;

    // 初始化 3D 画布
    initMagPlot3D();

    // 开始更新进度
    updateCalibrationProgress();
    setFinishBtnState(false);
}

/**
 * 处理磁力计数据（从串口接收）
 */
function processMagDataForCalibration(mx, my, mz) {
    if (!calibrationState.isCalibrating) return;

    // 更新最大最小值
    if (mx < calibrationState.minX) calibrationState.minX = mx;
    if (mx > calibrationState.maxX) calibrationState.maxX = mx;
    if (my < calibrationState.minY) calibrationState.minY = my;
    if (my > calibrationState.maxY) calibrationState.maxY = my;
    if (mz < calibrationState.minZ) calibrationState.minZ = mz;
    if (mz > calibrationState.maxZ) calibrationState.maxZ = mz;

    // 保存样本
    calibrationState.samples.push({ x: mx, y: my, z: mz });

    // 更新显示
    document.getElementById('mx-min').textContent = calibrationState.minX;
    document.getElementById('mx-max').textContent = calibrationState.maxX;
    document.getElementById('my-min').textContent = calibrationState.minY;
    document.getElementById('my-max').textContent = calibrationState.maxY;
    document.getElementById('mz-min').textContent = calibrationState.minZ;
    document.getElementById('mz-max').textContent = calibrationState.maxZ;
    document.getElementById('calib-samples').textContent = calibrationState.samples.length;

    updateMagPlots();
}

/**
 * 更新校准进度
 */
function updateCalibrationProgress() {
    if (!calibrationState.isCalibrating) return;

    const elapsed = Date.now() - calibrationState.startTime;
    const remaining = Math.max(0, calibrationState.duration - elapsed);
    const progress = Math.min(100, (elapsed / calibrationState.duration) * 100);
    const elapsedSeconds = Math.floor(elapsed / 1000);

    // 更新进度条
    document.getElementById('calib-progress').style.width = progress + '%';
    document.getElementById('calib-time').textContent = elapsedSeconds;
    const timerValue = document.getElementById('calib-timer-value');
    if (timerValue) {
        timerValue.textContent = elapsedSeconds;
    }
    setFinishBtnState(progress >= 100);

    if (remaining > 0) {
        requestAnimationFrame(updateCalibrationProgress);
    } else {
        // 校准完成
        finishCalibration();
    }
}

/**
 * 完成校准，计算参数
 */
function finishCalibration() {
    setFinishBtnState(false);
    calibrationState.isCalibrating = false;

    // 计算校准参数
    calculateCalibrationParams();
    updateMagPlots();

    // 显示结果
    displayCalibrationResult();

    // 切换到步骤3
    document.getElementById('calib-step-2').classList.add('hidden');
    document.getElementById('calib-step-3').classList.remove('hidden');
    setActiveLayer('calib-step-3');

    lucide.createIcons(); // 重新创建图标
}

/**
 * 计算校准参数
 */
function calculateCalibrationParams() {
    // 1. 硬铁偏置 = (max + min) / 2
    calibrationResult.offsetX = (calibrationState.maxX + calibrationState.minX) / 2;
    calibrationResult.offsetY = (calibrationState.maxY + calibrationState.minY) / 2;
    calibrationResult.offsetZ = (calibrationState.maxZ + calibrationState.minZ) / 2;

    // 2. 各轴半径
    const radiusX = (calibrationState.maxX - calibrationState.minX) / 2;
    const radiusY = (calibrationState.maxY - calibrationState.minY) / 2;
    const radiusZ = (calibrationState.maxZ - calibrationState.minZ) / 2;

    // 3. 平均半径
    const avgRadius = (radiusX + radiusY + radiusZ) / 3;

    // 4. 软铁缩放 = avg_radius / axis_radius
    calibrationResult.scaleX = radiusX > 0.01 ? (avgRadius / radiusX) : 1.0;
    calibrationResult.scaleY = radiusY > 0.01 ? (avgRadius / radiusY) : 1.0;
    calibrationResult.scaleZ = radiusZ > 0.01 ? (avgRadius / radiusZ) : 1.0;
}

/**
 * 显示校准结果
 */
function displayCalibrationResult() {
    // 显示数值
    document.getElementById('offset-x').textContent = calibrationResult.offsetX.toFixed(1);
    document.getElementById('offset-y').textContent = calibrationResult.offsetY.toFixed(1);
    document.getElementById('offset-z').textContent = calibrationResult.offsetZ.toFixed(1);
    document.getElementById('scale-x').textContent = calibrationResult.scaleX.toFixed(3);
    document.getElementById('scale-y').textContent = calibrationResult.scaleY.toFixed(3);
    document.getElementById('scale-z').textContent = calibrationResult.scaleZ.toFixed(3);

    // 生成代码
    const code = `mag_set_calibration(
    ${calibrationResult.offsetX.toFixed(1)}f, ${calibrationResult.offsetY.toFixed(1)}f, ${calibrationResult.offsetZ.toFixed(1)}f,    // offset_x, offset_y, offset_z
    ${calibrationResult.scaleX.toFixed(3)}f, ${calibrationResult.scaleY.toFixed(3)}f, ${calibrationResult.scaleZ.toFixed(3)}f     // scale_x, scale_y, scale_z
);`;
    document.getElementById('calib-code').textContent = code;

    // 评估校准质量
    evaluateCalibrationQuality();
}

/**
 * 评估校准质量
 */
function evaluateCalibrationQuality() {
    const radiusX = (calibrationState.maxX - calibrationState.minX) / 2;
    const radiusY = (calibrationState.maxY - calibrationState.minY) / 2;
    const radiusZ = (calibrationState.maxZ - calibrationState.minZ) / 2;

    const maxRadius = Math.max(radiusX, radiusY, radiusZ);
    const minRadius = Math.min(radiusX, radiusY, radiusZ);
    const radiusRatio = maxRadius / (minRadius + 0.01);

    const qualityDiv = document.getElementById('calib-quality');
    const tipsDiv = document.getElementById('calib-tips');

    if (radiusRatio < 1.2) {
        qualityDiv.textContent = '✅ 优秀';
        qualityDiv.className = 'quality-badge quality-excellent';
        tipsDiv.textContent = '数据分布均匀，校准质量很高！';
    } else if (radiusRatio < 1.5) {
        qualityDiv.textContent = '✔ 良好';
        qualityDiv.className = 'quality-badge quality-good';
        tipsDiv.textContent = '数据分布尚可，校准质量中等。';
    } else {
        qualityDiv.textContent = '⚠ 需改进';
        qualityDiv.className = 'quality-badge quality-poor';
        tipsDiv.textContent = '数据分布不均，建议重新校准并尽量覆盖更多方向。';
    }

    if (calibrationState.samples.length < 100) {
        tipsDiv.textContent += ' 样本数偏少，建议延长校准时间。';
    }
}

/**
 * 复制校准代码
 */
function copyCalibCode() {
    const code = document.getElementById('calib-code').textContent;
    navigator.clipboard.writeText(code).then(() => {
        const btn = event.target.closest('button');
        const originalText = btn.innerHTML;
        btn.innerHTML = '<i data-lucide="check"></i> 已复制';
        lucide.createIcons();
        setTimeout(() => {
            btn.innerHTML = originalText;
            lucide.createIcons();
        }, 2000);
    });
}

// ---------------- 3D 绘制 ----------------

// 数据缩放因子（将原始值缩放到合适的3D范围，适配半径120的球体）
const MAG_SCALE_FACTOR = 0.06;

function initMagPlot3D() {
    // 清理旧场景
    if (magScene) {
        while (magScene.children.length > 0) {
            magScene.remove(magScene.children[0]);
        }
    }

    const canvas = document.getElementById('magPlot');
    if (!canvas) return;

    const width = canvas.clientWidth || 600;
    const height = canvas.clientHeight || 480;

    // 创建场景 - 背景近白
    magScene = new THREE.Scene();
    magScene.background = new THREE.Color(0xfcfcfc);

    // 相机
    magCamera = new THREE.PerspectiveCamera(50, width / height, 1, 1000);
    magCamera.position.set(250, 200, 250);
    magCamera.lookAt(0, 0, 0);

    // 渲染器
    magRenderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true });
    magRenderer.setSize(width, height);
    magRenderer.setPixelRatio(window.devicePixelRatio);

    // 坐标轴
    const axes = new THREE.AxesHelper(100);
    magScene.add(axes);

    // 线框球体
    const sphereGeo = new THREE.SphereGeometry(120, 32, 32);
    const sphereMat = new THREE.MeshBasicMaterial({
        color: 0xe2e8f0,
        wireframe: true,
        transparent: true,
        opacity: 0.6
    });
    magScene.add(new THREE.Mesh(sphereGeo, sphereMat));

    // 原始点云 - 红色
    const rawGeo = new THREE.BufferGeometry();
    const maxPoints = 3000;
    const positions = new Float32Array(maxPoints * 3);
    rawGeo.setAttribute('position', new THREE.BufferAttribute(positions, 3));
    rawGeo.setDrawRange(0, 0);

    const rawMat = new THREE.PointsMaterial({
        color: 0xef4444,
        size: 4,
        sizeAttenuation: true
    });
    magRawPoints = new THREE.Points(rawGeo, rawMat);
    magScene.add(magRawPoints);

    // 校准后点云 (绿色)
    const calGeo = new THREE.BufferGeometry();
    const calPositions = new Float32Array(maxPoints * 3);
    calGeo.setAttribute('position', new THREE.BufferAttribute(calPositions, 3));
    calGeo.setDrawRange(0, 0);

    const calMat = new THREE.PointsMaterial({
        color: 0x00ff88,
        size: 4,
        sizeAttenuation: true,
        transparent: true,
        opacity: 0.8
    });
    magCalPoints = new THREE.Points(calGeo, calMat);
    magScene.add(magCalPoints);

    // 启动动画
    animateMagPlot();
    console.log('[MagCalib] 3D场景已初始化 (Visual Style: Reference)');
}

function animateMagPlot() {
    if (!calibrationState.isCalibrating && document.getElementById('magCalibModal').style.display === 'none') return;

    requestAnimationFrame(animateMagPlot);

    if (magScene && magRenderer && magCamera) {
        magScene.rotation.y += 0.003;
        magRenderer.render(magScene, magCamera);
    }
}

function updateMagPlots() {
    if (!magRawPoints) return;

    const count = calibrationState.samples.length;
    const positions = magRawPoints.geometry.attributes.position.array;

    // 更新原始点云
    for (let i = 0; i < count; i++) {
        const s = calibrationState.samples[i];
        const idx = i * 3;
        // 简单映射：x->x, z->y, y->z
        positions[idx] = s.x * MAG_SCALE_FACTOR;
        positions[idx + 1] = s.z * MAG_SCALE_FACTOR;
        positions[idx + 2] = -s.y * MAG_SCALE_FACTOR;
    }

    magRawPoints.geometry.setDrawRange(0, count);
    magRawPoints.geometry.attributes.position.needsUpdate = true;

    // 更新校准后点云
    if (magCalPoints && calibrationResult.scaleX !== 1) {
        const calPositions = magCalPoints.geometry.attributes.position.array;
        const ox = calibrationResult.offsetX;
        const oy = calibrationResult.offsetY;
        const oz = calibrationResult.offsetZ;
        const sx = calibrationResult.scaleX;
        const sy = calibrationResult.scaleY;
        const sz = calibrationResult.scaleZ;

        for (let i = 0; i < count; i++) {
            const s = calibrationState.samples[i];
            const idx = i * 3;

            const cx = (s.x - ox) * sx;
            const cy = (s.y - oy) * sy;
            const cz = (s.z - oz) * sz;

            calPositions[idx] = cx * MAG_SCALE_FACTOR;
            calPositions[idx + 1] = cz * MAG_SCALE_FACTOR;
            calPositions[idx + 2] = -cy * MAG_SCALE_FACTOR;
        }
        magCalPoints.geometry.setDrawRange(0, count);
        magCalPoints.geometry.attributes.position.needsUpdate = true;
    }
}

/**
 * 导出磁力计数据
 * @param {string} format - 'csv' 或 'json'
 */
function exportMagData(format) {
    if (calibrationState.samples.length === 0) {
        alert('没有数据可导出！请先进行校准采集。');
        return;
    }

    const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
    let content, filename, mimeType;

    if (format === 'csv') {
        const header = 'index,mx_raw,my_raw,mz_raw,mx_cal,my_cal,mz_cal\n';
        const rows = calibrationState.samples.map((s, i) => {
            const mx_cal = (s.x - calibrationResult.offsetX) * calibrationResult.scaleX;
            const my_cal = (s.y - calibrationResult.offsetY) * calibrationResult.scaleY;
            const mz_cal = (s.z - calibrationResult.offsetZ) * calibrationResult.scaleZ;
            return `${i},${s.x},${s.y},${s.z},${mx_cal.toFixed(2)},${my_cal.toFixed(2)},${mz_cal.toFixed(2)}`;
        }).join('\n');

        content = header + rows;
        filename = `mag_calibration_${timestamp}.csv`;
        mimeType = 'text/csv';
    } else {
        const exportData = {
            timestamp: new Date().toISOString(),
            sampleCount: calibrationState.samples.length,
            calibrationParams: {
                offset: { x: calibrationResult.offsetX, y: calibrationResult.offsetY, z: calibrationResult.offsetZ },
                scale: { x: calibrationResult.scaleX, y: calibrationResult.scaleY, z: calibrationResult.scaleZ }
            },
            rawSamples: calibrationState.samples,
            calibratedSamples: calibrationState.samples.map(s => ({
                x: (s.x - calibrationResult.offsetX) * calibrationResult.scaleX,
                y: (s.y - calibrationResult.offsetY) * calibrationResult.scaleY,
                z: (s.z - calibrationResult.offsetZ) * calibrationResult.scaleZ
            }))
        };

        content = JSON.stringify(exportData, null, 2);
        filename = `mag_calibration_${timestamp}.json`;
        mimeType = 'application/json';
    }

    const blob = new Blob([content], { type: mimeType });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);

    console.log(`[MagCalib] 已导出 ${calibrationState.samples.length} 个样本到 ${filename}`);
}

// 导出函数供其他模块使用
window.openMagCalibModal = openMagCalibModal;
window.closeMagCalibModal = closeMagCalibModal;
window.resetMagCalibration = resetMagCalibration;
window.startMagCalibration = startMagCalibration;
window.processMagDataForCalibration = processMagDataForCalibration;
window.copyCalibCode = copyCalibCode;
window.exportMagData = exportMagData;
