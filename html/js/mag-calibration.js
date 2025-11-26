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
    
    // 初始化画布
    initMagPlot();
    
    // 开始更新进度
    updateCalibrationProgress();
}

/**
 * 处理磁力计数据（从串口接收）
 */
function processMagDataForCalibration(mx, my, mz) {
    // 调试输出
    console.log('[MagCalib] 收到数据:', {mx, my, mz, isCalibrating: calibrationState.isCalibrating});
    
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
    
    // 更新画布
    drawMagSample(mx, my, mz);
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

/**
 * 初始化磁力计绘图画布
 */
function initMagPlot() {
    const canvas = document.getElementById('magPlot');
    const ctx = canvas.getContext('2d');
    
    // 清空画布
    ctx.fillStyle = '#1a1a2e';
    ctx.fillRect(0, 0, 400, 400);
    
    // 绘制坐标轴
    ctx.strokeStyle = '#444';
    ctx.lineWidth = 1;
    
    // X轴
    ctx.beginPath();
    ctx.moveTo(0, 200);
    ctx.lineTo(400, 200);
    ctx.stroke();
    
    // Y轴
    ctx.beginPath();
    ctx.moveTo(200, 0);
    ctx.lineTo(200, 400);
    ctx.stroke();
    
    // 圆形参考线
    ctx.strokeStyle = '#333';
    for (let r = 50; r <= 200; r += 50) {
        ctx.beginPath();
        ctx.arc(200, 200, r, 0, Math.PI * 2);
        ctx.stroke();
    }
}

/**
 * 绘制磁力计样本点
 */
function drawMagSample(mx, my, mz) {
    const canvas = document.getElementById('magPlot');
    const ctx = canvas.getContext('2d');
    
    // 将磁力计数据映射到画布坐标（XY平面）
    // 假设范围 ±500
    const scale = 200 / 500;
    const x = 200 + mx * scale;
    const y = 200 - my * scale;
    
    // 根据Z值设置颜色
    const zNorm = (mz + 500) / 1000; // 归一化到0-1
    const hue = zNorm * 240; // 蓝色到红色
    ctx.fillStyle = `hsl(${hue}, 70%, 60%)`;
    
    // 绘制点
    ctx.beginPath();
    ctx.arc(x, y, 2, 0, Math.PI * 2);
    ctx.fill();
}

// 导出函数供其他模块使用
window.openMagCalibModal = openMagCalibModal;
window.closeMagCalibModal = closeMagCalibModal;
window.resetMagCalibration = resetMagCalibration;
window.startMagCalibration = startMagCalibration;
window.processMagDataForCalibration = processMagDataForCalibration;
window.copyCalibCode = copyCalibCode;

