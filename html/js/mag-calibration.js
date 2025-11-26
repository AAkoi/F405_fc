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

    updateMagPlots();
}

/**
 * 开始校准
 */
function startMagCalibration() {
    console.log('[MagCalib] 开始校准...');
    
    // 隐藏步骤1，显示步骤2
    document.getElementById('calib-step-1').classList.add('hidden');
    document.getElementById('calib-step-2').classList.remove('hidden');
    
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
    
    // 更新进度条
    document.getElementById('calib-progress').style.width = progress + '%';
    document.getElementById('calib-time').textContent = Math.floor(elapsed / 1000);
    
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
    calibrationState.isCalibrating = false;
    
    // 计算校准参数
    calculateCalibrationParams();
    updateMagPlots();
    
    // 显示结果
    displayCalibrationResult();
    
    // 切换到步骤3
    document.getElementById('calib-step-2').classList.add('hidden');
    document.getElementById('calib-step-3').classList.remove('hidden');
    
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

// 数据缩放因子（将原始值缩放到合适的3D范围）
const MAG_SCALE_FACTOR = 0.05;  // 400 * 0.05 = 20，适合3D场景

// 理想球体（用于参考）
let idealSphereRaw, idealSphereCal;

function initMagPlot3D() {
    // 每次都重新初始化，避免旧数据残留
    if (magScene) {
        // 清理旧场景
        while(magScene.children.length > 0) { 
            magScene.remove(magScene.children[0]); 
        }
    }
    
    const canvas = document.getElementById('magPlot');
    const width = canvas.clientWidth || 420;
    const height = canvas.clientHeight || 420;

    // 创建场景
    magScene = new THREE.Scene();
    magScene.background = new THREE.Color(0x1a1a2e);  // 深蓝色背景
    
    // 相机
    magCamera = new THREE.PerspectiveCamera(50, width / height, 0.1, 2000);
    magCamera.position.set(40, 30, 40);
    magCamera.lookAt(0, 0, 0);

    // 渲染器
    magRenderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true });
    magRenderer.setSize(width, height);
    magRenderer.setPixelRatio(window.devicePixelRatio);

    // 控制器
    magControls = new THREE.OrbitControls(magCamera, magRenderer.domElement);
    magControls.enableDamping = true;
    magControls.dampingFactor = 0.1;
    magControls.minDistance = 20;
    magControls.maxDistance = 200;

    // 添加环境光和点光源
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.6);
    magScene.add(ambientLight);
    const pointLight = new THREE.PointLight(0xffffff, 0.8);
    pointLight.position.set(50, 50, 50);
    magScene.add(pointLight);

    // 坐标轴（更粗更长）
    const axesHelper = new THREE.AxesHelper(25);
    magScene.add(axesHelper);
    
    // 添加轴标签
    addAxisLabels();

    // 网格（XZ平面）
    const gridHelper = new THREE.GridHelper(50, 10, 0x444466, 0x333355);
    magScene.add(gridHelper);

    // 原始点云（红/橙色，较大的点）
    const rawGeo = new THREE.BufferGeometry();
    rawGeo.setAttribute('position', new THREE.Float32BufferAttribute([], 3));
    const rawMat = new THREE.PointsMaterial({ 
        color: 0xff6b35,  // 橙红色
        size: 4,          // 增大点尺寸
        sizeAttenuation: true,
        transparent: true,
        opacity: 0.9
    });
    magRawPoints = new THREE.Points(rawGeo, rawMat);
    magScene.add(magRawPoints);

    // 校准后点云（青色/绿色）
    const calGeo = new THREE.BufferGeometry();
    calGeo.setAttribute('position', new THREE.Float32BufferAttribute([], 3));
    const calMat = new THREE.PointsMaterial({ 
        color: 0x00ff88,  // 亮绿色
        size: 4,
        sizeAttenuation: true,
        transparent: true,
        opacity: 0.9
    });
    magCalPoints = new THREE.Points(calGeo, calMat);
    magScene.add(magCalPoints);

    // 理想球体参考线（校准后应该趋近于此）
    const sphereGeo = new THREE.SphereGeometry(15, 24, 24);
    const sphereMat = new THREE.MeshBasicMaterial({ 
        color: 0x00ff88, 
        wireframe: true, 
        transparent: true, 
        opacity: 0.15 
    });
    idealSphereCal = new THREE.Mesh(sphereGeo, sphereMat);
    idealSphereCal.visible = false;  // 校准完成后显示
    magScene.add(idealSphereCal);

    animateMagPlot();
    console.log('[MagCalib] 3D场景已初始化');
}

// 添加轴标签
function addAxisLabels() {
    // 使用 CSS2D 或简单的 Sprite 来显示轴标签
    const createLabel = (text, position, color) => {
        const canvas = document.createElement('canvas');
        canvas.width = 64;
        canvas.height = 32;
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = color;
        ctx.font = 'bold 24px Arial';
        ctx.textAlign = 'center';
        ctx.fillText(text, 32, 24);
        
        const texture = new THREE.CanvasTexture(canvas);
        const material = new THREE.SpriteMaterial({ map: texture, transparent: true });
        const sprite = new THREE.Sprite(material);
        sprite.position.copy(position);
        sprite.scale.set(4, 2, 1);
        return sprite;
    };
    
    magScene.add(createLabel('X', new THREE.Vector3(28, 0, 0), '#ff4444'));
    magScene.add(createLabel('Y', new THREE.Vector3(0, 28, 0), '#44ff44'));
    magScene.add(createLabel('Z', new THREE.Vector3(0, 0, 28), '#4488ff'));
}

function animateMagPlot() {
    requestAnimationFrame(animateMagPlot);
    if (magControls) magControls.update();
    if (magRenderer && magScene && magCamera) {
        magRenderer.render(magScene, magCamera);
    }
}

function updateMagPlots() {
    if (!magRawPoints || !magCalPoints) return;
    
    const rawPositions = [];
    const calPositions = [];
    const ox = calibrationResult.offsetX;
    const oy = calibrationResult.offsetY;
    const oz = calibrationResult.offsetZ;
    const sx = calibrationResult.scaleX;
    const sy = calibrationResult.scaleY;
    const sz = calibrationResult.scaleZ;

    // 缩放数据到合适的3D范围
    calibrationState.samples.forEach(s => {
        // 原始数据（缩放后）
        rawPositions.push(
            s.x * MAG_SCALE_FACTOR, 
            s.z * MAG_SCALE_FACTOR,  // Y-up，所以Z轴数据放到Y位置
            s.y * MAG_SCALE_FACTOR
        );
        // 校准后数据
        calPositions.push(
            (s.x - ox) * sx * MAG_SCALE_FACTOR,
            (s.z - oz) * sz * MAG_SCALE_FACTOR,
            (s.y - oy) * sy * MAG_SCALE_FACTOR
        );
    });

    // 更新点云
    magRawPoints.geometry.setAttribute('position', new THREE.Float32BufferAttribute(rawPositions, 3));
    magRawPoints.geometry.attributes.position.needsUpdate = true;
    magRawPoints.geometry.computeBoundingSphere();
    
    magCalPoints.geometry.setAttribute('position', new THREE.Float32BufferAttribute(calPositions, 3));
    magCalPoints.geometry.attributes.position.needsUpdate = true;
    magCalPoints.geometry.computeBoundingSphere();

    // 自动调整相机以适应数据范围
    if (calibrationState.samples.length > 10) {
        const bounds = magRawPoints.geometry.boundingSphere;
        if (bounds && bounds.radius > 0) {
            // 根据数据范围动态调整理想球体大小
            if (idealSphereCal) {
                const avgRadius = bounds.radius * 0.9;
                idealSphereCal.scale.setScalar(avgRadius / 15);
                
                // 校准进行中显示球体参考
                if (calibrationState.samples.length > 50) {
                    idealSphereCal.visible = true;
                }
            }
        }
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
        // CSV格式导出
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
        // JSON格式导出
        const exportData = {
            timestamp: new Date().toISOString(),
            sampleCount: calibrationState.samples.length,
            calibrationParams: {
                offset: {
                    x: calibrationResult.offsetX,
                    y: calibrationResult.offsetY,
                    z: calibrationResult.offsetZ
                },
                scale: {
                    x: calibrationResult.scaleX,
                    y: calibrationResult.scaleY,
                    z: calibrationResult.scaleZ
                }
            },
            range: {
                x: { min: calibrationState.minX, max: calibrationState.maxX },
                y: { min: calibrationState.minY, max: calibrationState.maxY },
                z: { min: calibrationState.minZ, max: calibrationState.maxZ }
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

    // 创建下载
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
