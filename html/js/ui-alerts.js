class AlertManager {
    constructor() {
        this.toastContainer = document.getElementById('toast-container');
        this.hud = document.getElementById('hud-alert');
        this.hudText = document.getElementById('hud-text');
        this.modal = document.getElementById('critical-modal');
        this.modalTitle = document.getElementById('ec-title');
        this.modalSubtitle = document.getElementById('ec-subtitle');
        this.modalMsg = document.getElementById('ec-message');
        this.modalCode = document.getElementById('ec-code');
        this.hudTimer = null;
        this.onEmergencyStop = null;

        this.bindModalButtons();
    }

    showConnectionToast(connected) {
        const msg = connected ? '串口已连接，等待数据...' : '连接已断开';
        this.toast({
            type: connected ? 'success' : 'warn',
            title: connected ? 'Connected' : 'Disconnected',
            message: msg
        });
    }

    toast({ type = 'info', title = '', message = '', duration = 3000 }) {
        if (!this.toastContainer) return;
        // Limit to 5 toasts
        while (this.toastContainer.children.length >= 5) {
            this.toastContainer.removeChild(this.toastContainer.firstChild);
        }
        const toast = document.createElement('div');
        toast.className = `toast-card t-${type}`;
        toast.innerHTML = `
            <div class="t-icon">${this.getIcon(type)}</div>
            <div class="t-content">
                <div class="t-title">${title}</div>
                <div class="t-msg">${message}</div>
            </div>
        `;
        this.toastContainer.appendChild(toast);

        requestAnimationFrame(() => toast.classList.add('toast-enter'));

        setTimeout(() => {
            toast.style.opacity = '0';
            toast.style.transform = 'translateX(120%)';
            setTimeout(() => toast.remove(), 300);
        }, duration);
    }

    showHud(text, duration = 5000) {
        if (!this.hud || !this.hudText) return;
        this.hudText.innerText = text;
        this.hud.classList.add('active');

        if (this.hudTimer) clearTimeout(this.hudTimer);
        this.hudTimer = setTimeout(() => this.hideHud(), duration);
    }

    hideHud() {
        if (!this.hud) return;
        this.hud.classList.remove('active');
    }

    showCritical({ title, subtitle, message, code }) {
        if (!this.modal) return;
        if (this.modalTitle && title) this.modalTitle.innerText = title;
        if (this.modalSubtitle && subtitle) this.modalSubtitle.innerText = subtitle;
        if (this.modalMsg && message) this.modalMsg.innerText = message;
        if (this.modalCode && code !== undefined) this.modalCode.innerText = code;
        this.modal.classList.add('active');
    }

    hideCritical() {
        if (!this.modal) return;
        this.modal.classList.remove('active');
    }

    setEmergencyStopHandler(fn) {
        this.onEmergencyStop = fn;
    }

    bindModalButtons() {
        const ignoreBtn = document.getElementById('ec-ignore');
        const stopBtn = document.getElementById('ec-stop');

        if (ignoreBtn) ignoreBtn.addEventListener('click', () => this.hideCritical());
        if (stopBtn) {
            stopBtn.addEventListener('click', () => {
                this.hideCritical();
                if (typeof this.onEmergencyStop === 'function') {
                    this.onEmergencyStop();
                } else if (typeof window.handleEmergencyStop === 'function') {
                    window.handleEmergencyStop();
                }
            });
        }

        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                this.hideCritical();
            }
        });
    }

    getIcon(type) {
        const color = {
            info: '#3b82f6',
            success: '#22c55e',
            warn: '#f59e0b',
            error: '#ef4444'
        }[type] || '#3b82f6';

        const path = {
            info: '<circle cx="12" cy="12" r="10"></circle><line x1="12" y1="16" x2="12" y2="12"></line><line x1="12" y1="8" x2="12.01" y2="8"></line>',
            success: '<path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path><polyline points="22 4 12 14.01 9 11.01"></polyline>',
            warn: '<path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"></path><line x1="12" y1="9" x2="12" y2="13"></line><line x1="12" y1="17" x2="12.01" y2="17"></line>',
            error: '<circle cx="12" cy="12" r="10"></circle><line x1="15" y1="9" x2="9" y2="15"></line><line x1="9" y1="9" x2="15" y2="15"></line>'
        }[type] || '';

        return `<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="${color}" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">${path}</svg>`;
    }
}
