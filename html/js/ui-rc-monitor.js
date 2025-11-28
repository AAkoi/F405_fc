class RcMonitor {
    constructor() {
        this.card = document.getElementById('rc-monitor');
        this.nodes = {
            statusText: document.getElementById('rc-status-text'),
            hz: document.getElementById('hz-val'),
            sticks: {
                left: document.getElementById('stick-l'),
                right: document.getElementById('stick-r')
            },
            bars: {
                thr: { bar: document.getElementById('bar-thr'), text: document.getElementById('txt-thr') },
                yaw: { bar: document.getElementById('bar-yaw'), text: document.getElementById('txt-yaw') },
                pit: { bar: document.getElementById('bar-pit'), text: document.getElementById('txt-pit') },
                rol: { bar: document.getElementById('bar-rol'), text: document.getElementById('txt-rol') }
            },
            switches: [
                document.getElementById('sw1'),
                document.getElementById('sw2'),
                document.getElementById('sw3'),
                document.getElementById('sw4')
            ]
        };

        this.frameCount = 0;
        this.lastHzMark = Date.now();
        this.switchModes = ['2pos', '3pos', '3pos', '3pos']; // 左起：ARM(2段)，其余3段
        this.armActive = false;

        this.setConnection(false);
    }

    setConnection(connected) {
        if (this.card) {
            this.card.classList.toggle('offline', !connected);
        }
        if (this.nodes.statusText) {
            this.nodes.statusText.innerText = connected ? 'CONNECTED' : 'DISCONNECTED';
        }
        if (!connected) {
            if (this.nodes.hz) this.nodes.hz.innerText = '0';
            this.frameCount = 0;
            this.lastHzMark = Date.now();
            this.resetToDefault();
        }
    }

    resetToDefault() {
        this.renderChannels({
            thr: 1000,
            yaw: 1500,
            pit: 1500,
            rol: 1500,
            aux: [1000, 1000, 1000, 1000]
        });
    }

    update(rc) {
        if (!rc) {
            this.setConnection(false);
            return;
        }

        const now = Date.now();
        const alive = rc.lastTs && (now - rc.lastTs < 800);
        this.setConnection(alive);
        if (!alive) return;

        // 统计刷新率
        this.frameCount += 1;
        if (now - this.lastHzMark >= 1000) {
            if (this.nodes.hz) this.nodes.hz.innerText = this.frameCount.toString();
            this.frameCount = 0;
            this.lastHzMark = now;
        }

        const channels = this.mapChannels(rc);
        this.renderChannels(channels);
    }

    mapChannels(rc) {
        const clamp = (v, min, max) => Math.min(Math.max(v, min), max);
        const thr = 1000 + clamp(rc.throttle ?? 0, 0, 1) * 1000;
        const yaw = 1500 + clamp(rc.yaw ?? 0, -1, 1) * 500;
        const pit = 1500 + clamp(rc.pitch ?? 0, -1, 1) * 500;
        const rol = 1500 + clamp(rc.roll ?? 0, -1, 1) * 500;

        const aux = [];
        for (let i = 0; i < 4; i++) {
            const v = rc.aux && rc.aux[i] !== undefined ? rc.aux[i] : 0;
            aux.push(1000 + clamp(v, 0, 1) * 1000);
        }

        return { thr, yaw, pit, rol, aux };
    }

    renderChannels(ch, forceReset = false) {
        const clamp = (v, min, max) => Math.min(Math.max(v, min), max);
        const mapStick = (val, invert = false) => {
            let n = (val - 1500) / 500;
            if (invert) n = -n;
            return clamp(n, -1, 1) * 63;
        };

        // Sticks
        if (this.nodes.sticks.left) {
            const lx = mapStick(ch.yaw);
            const ly = mapStick(ch.thr, true);
            this.nodes.sticks.left.style.transform = `translate(calc(-50% + ${lx}px), calc(-50% + ${ly}px))`;
        }
        if (this.nodes.sticks.right) {
            const rx = mapStick(ch.rol);
            const ry = mapStick(ch.pit, true);
            this.nodes.sticks.right.style.transform = `translate(calc(-50% + ${rx}px), calc(-50% + ${ry}px))`;
        }

        // Bars
        this.updateBar(this.nodes.bars.thr, ch.thr);
        this.updateBar(this.nodes.bars.yaw, ch.yaw);
        this.updateBar(this.nodes.bars.pit, ch.pit);
        this.updateBar(this.nodes.bars.rol, ch.rol);

        // Switches
        for (let i = 0; i < this.nodes.switches.length; i++) {
            this.updateSwitch(this.nodes.switches[i], ch.aux[i] ?? 1000, this.switchModes[i], i === 0);
        }
    }

    updateBar(nodeGroup, val) {
        if (!nodeGroup || !nodeGroup.bar || !nodeGroup.text) return;
        const pct = Math.max(0, Math.min(100, (val - 1000) / 10));
        nodeGroup.bar.style.width = `${pct}%`;
        nodeGroup.text.innerText = Math.round(val);
    }

    updateSwitch(col, val, mode = '3pos', isArm = false) {
        if (!col) return;
        const segs = col.querySelectorAll('.led-seg');
        segs.forEach(s => s.classList.remove('active'));

        if (mode === '2pos') {
            const high = val > 1500;
            if (high) segs[0]?.classList.add('active'); else segs[2]?.classList.add('active');
            if (isArm) this.armActive = high;
        } else {
            if (val < 1300) {
                segs[2]?.classList.add('active'); // L
            } else if (val > 1700) {
                segs[0]?.classList.add('active'); // H
            } else {
                segs[1]?.classList.add('active'); // M
            }
            if (isArm) this.armActive = val > 1700;
        }
    }

    getArmState() {
        return !!this.armActive;
    }
}
