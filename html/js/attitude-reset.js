/**
 * 姿态复位模块
 * 用于设置姿态角的零点偏移
 */

let attitudeOffset = {
    roll: 0,
    pitch: 0,
    yaw: 0,
    active: false
};

/**
 * 复位姿态（设置当前姿态为零点）
 */
function resetAttitude() {
    // 获取当前姿态角
    const currentRoll = parseFloat(document.getElementById('val-roll').textContent) || 0;
    const currentPitch = parseFloat(document.getElementById('val-pitch').textContent) || 0;
    const currentYaw = parseFloat(document.getElementById('val-yaw').textContent) || 0;
    
    // 设置偏移量
    attitudeOffset.roll = currentRoll;
    attitudeOffset.pitch = currentPitch;
    attitudeOffset.yaw = currentYaw;
    attitudeOffset.active = true;
    
    console.log('[AttitudeReset] 姿态已复位，零点偏移:', attitudeOffset);
    
    // 显示提示
    const btn = document.getElementById('resetAttitudeBtn');
    if (btn) {
        const originalHTML = btn.innerHTML;
        btn.innerHTML = '<i data-lucide="check"></i> 已复位';
        btn.classList.add('btn-success');
        lucide.createIcons();
        
        setTimeout(() => {
            btn.innerHTML = originalHTML;
            btn.classList.remove('btn-success');
            lucide.createIcons();
        }, 2000);
    }
    
    // 显示提示信息
    showToast('✅ 姿态已复位到零点！');
}

/**
 * 应用姿态偏移
 */
function applyAttitudeOffset(roll, pitch, yaw) {
    if (!attitudeOffset.active) {
        return { roll, pitch, yaw };
    }
    
    let adjustedRoll = roll - attitudeOffset.roll;
    let adjustedPitch = pitch - attitudeOffset.pitch;
    let adjustedYaw = yaw - attitudeOffset.yaw;
    
    // 处理Yaw的±180°边界
    while (adjustedYaw > 180) adjustedYaw -= 360;
    while (adjustedYaw < -180) adjustedYaw += 360;
    
    return {
        roll: adjustedRoll,
        pitch: adjustedPitch,
        yaw: adjustedYaw
    };
}

/**
 * 清除姿态偏移
 */
function clearAttitudeOffset() {
    attitudeOffset.active = false;
    attitudeOffset.roll = 0;
    attitudeOffset.pitch = 0;
    attitudeOffset.yaw = 0;
    console.log('[AttitudeReset] 姿态偏移已清除');
}

/**
 * 显示Toast提示
 */
function showToast(message) {
    // 创建toast元素
    const toast = document.createElement('div');
    toast.className = 'toast';
    toast.textContent = message;
    document.body.appendChild(toast);
    
    // 显示动画
    setTimeout(() => {
        toast.classList.add('show');
    }, 10);
    
    // 3秒后消失
    setTimeout(() => {
        toast.classList.remove('show');
        setTimeout(() => {
            document.body.removeChild(toast);
        }, 300);
    }, 3000);
}

// 导出函数
window.resetAttitude = resetAttitude;
window.applyAttitudeOffset = applyAttitudeOffset;
window.clearAttitudeOffset = clearAttitudeOffset;

