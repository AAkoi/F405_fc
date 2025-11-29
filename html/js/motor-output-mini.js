class MotorOutputMini {
    constructor() {
        this.root = document.getElementById('motor-mini');
        if (!this.root) return;
        this.badge = this.root.querySelector('.status-badge');
        this.motors = Array.from(this.root.querySelectorAll('.motor-col')).map((m) => ({
            el: m,
            fill: m.querySelector('.bar-fill'),
            val: m.querySelector('.pwm-val'),
            prop: m.querySelector('.micro-prop')
        }));
        this.isArmed = false;
    }

    setArm(armed) {
        if (!this.root) return;
        this.isArmed = armed;
        this.root.classList.toggle('armed', armed);
        if (this.badge) this.badge.innerText = armed ? 'ARMED' : 'DISARMED';
    }

    update(rc, armState = false) {
        if (!this.root) return;
        this.setArm(armState);

        const base = Math.max(0, Math.min(100, (rc?.throttle ?? 0) * 100));
        const mixVal = (r, p, y) => Math.max(0, Math.min(100, base + r + p + y));

        const roll = (rc?.roll ?? 0) * 20;
        const pitch = (rc?.pitch ?? 0) * 20;
        const yaw = (rc?.yaw ?? 0) * 10;

        const mixes = [
            mixVal(-roll + pitch - yaw, 0, 0), // M1
            mixVal(roll + pitch + yaw, 0, 0),  // M2
            mixVal(-roll - pitch + yaw, 0, 0), // M3
            mixVal(roll - pitch - yaw, 0, 0)   // M4
        ];

        mixes.forEach((pct, i) => {
            const m = this.motors[i];
            if (!m) return;
            const active = this.isArmed && pct > 5;
            m.el.classList.toggle('spinning', active);
            if (m.fill) {
                m.fill.style.height = `${pct}%`;
                const hue = pct < 50
                    ? 210 + (pct / 50) * 50
                    : 260 + ((pct - 50) / 50) * 100;
                m.fill.style.backgroundColor = `hsl(${hue}, 90%, 60%)`;
            }
            if (m.val) m.val.innerText = (1000 + Math.floor(pct * 10)).toString();
            if (m.prop) {
                m.prop.style.animationDuration = active ? `${Math.max(0.1, 0.4 - (pct / 300))}s` : '';
            }
        });
    }
}
