// 电机测试模态逻辑
(function() {
    const modal = document.getElementById('motorModal');
    if (!modal) return;

    const masterBtn = document.getElementById('masterBtn');
    const motors = Array.from(modal.querySelectorAll('.motor-trigger'));
    const safetyModal = document.getElementById('motorSafetyModal');
    const safetyCard = document.getElementById('motorSafetyCard');
    const safetyCheck = document.getElementById('motorSafetyCheck');
    const safetyClose = document.getElementById('motorSafetyClose');
    const confirmBtn = document.getElementById('motorConfirmBtn');

    function toggleMotor(el) {
        el.classList.toggle('active');
        updateMasterBtnState();
    }

    function toggleMaster() {
        const isAnyActive = motors.some((m) => m.classList.contains('active'));
        setAllMotors(!isAnyActive);
    }

    function setAllMotors(active) {
        motors.forEach((m) => {
            m.classList.toggle('active', active);
        });
        updateMasterBtnState();
    }

    function updateMasterBtnState() {
        const isAnyActive = motors.some((m) => m.classList.contains('active'));
        if (isAnyActive) {
            masterBtn.textContent = '停止全部电机';
            masterBtn.className = 'btn-main btn-stop';
        } else {
            masterBtn.textContent = '启动全部电机';
            masterBtn.className = 'btn-main btn-start';
        }
    }

    function openModal() {
        modal.classList.add('active');
        modal.classList.remove('hidden');
        document.body.style.overflow = 'hidden';
        setAllMotors(false);
    }

    function closeModal() {
        modal.classList.remove('active');
        modal.classList.add('hidden');
        document.body.style.overflow = '';
        setAllMotors(false);
    }

    // 关闭按钮逻辑：直接关闭
    const closeBtn = modal.querySelector('.motor-close');
    if (closeBtn) {
        closeBtn.addEventListener('click', () => {
            closeModal();
        });
    }

    // 更新确认按钮状态
    function updateConfirmBtn() {
        if (safetyCheck && confirmBtn) {
            if (safetyCheck.checked) {
                confirmBtn.classList.add('active');
            } else {
                confirmBtn.classList.remove('active');
            }
        }
    }

    // 复选框改变时更新按钮
    if (safetyCheck) {
        safetyCheck.addEventListener('change', updateConfirmBtn);
    }

    // 安全确认流程
    function openSafety() {
        if (!safetyModal) {
            openModal();
            return;
        }
        safetyCheck.checked = false;
        updateConfirmBtn();
        safetyModal.classList.remove('hidden');
        setTimeout(() => safetyModal.classList.add('active'), 10);
        document.body.style.overflow = 'hidden';
    }

    function closeSafety() {
        if (!safetyModal) return;
        safetyModal.classList.remove('active');
        setTimeout(() => {
            safetyModal.classList.add('hidden');
            safetyCheck.checked = false;
            updateConfirmBtn();
        }, 400);
        document.body.style.overflow = '';
    }

    // 关闭按钮点击
    safetyClose?.addEventListener('click', () => {
        closeSafety();
    });

    // 确认按钮点击
    confirmBtn?.addEventListener('click', () => {
        if (!safetyCheck.checked) {
            // 震动提醒
            if (safetyCard) {
                safetyCard.classList.remove('shake');
                void safetyCard.offsetWidth;
                safetyCard.classList.add('shake');
            }
            return;
        }
        // 确认后进入电机测试
        closeSafety();
        setTimeout(() => openModal(), 500);
    });

    function startMotorFlow() {
        const armActive = window.app?.uiManager?.rcMonitor?.getArmState?.();
        if (armActive) {
            window.app?.uiManager?.alerts?.toast({
                type: 'warn',
                title: 'ARM 已开启',
                message: '请先关闭 ARM，再进行电机测试。'
            });
            return;
        }
        openSafety();
    }

    // 暴露到全局
    window.startMotorFlow = startMotorFlow;
    window.openMotorModal = openModal;
    window.closeMotorModal = closeModal;
    window.toggleMotor = toggleMotor;
    window.toggleMaster = toggleMaster;
})();
