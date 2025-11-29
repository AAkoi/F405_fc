class AttitudeWidget {
    constructor() {
        this.widget = document.getElementById('att-widget');
        if (!this.widget) return;

        this.nodes = {
            rollText: document.getElementById('att-roll-text'),
            pitchText: document.getElementById('att-pitch-text'),
            yawText: document.getElementById('att-yaw-text'),
            rollBar: document.getElementById('att-roll-bar'),
            pitchBar: document.getElementById('att-pitch-bar'),
            yawBar: document.getElementById('att-yaw-bar'),
            canvas: document.getElementById('att-canvas'),
            roll3d: document.getElementById('att-3d-roll'),
            pitch3d: document.getElementById('att-3d-pitch'),
            yaw3d: document.getElementById('att-3d-yaw')
        };

        this.ctx = this.nodes.canvas ? this.nodes.canvas.getContext('2d') : null;
        this.size = this.nodes.canvas
            ? { w: this.nodes.canvas.width, h: this.nodes.canvas.height }
            : { w: 0, h: 0 };
        this.last = { roll: 0, pitch: 0, yaw: 0 };

        this.widget.addEventListener('click', () => this.toggleView());
    }

    toggleView() {
        if (!this.widget) return;
        this.widget.classList.toggle('show-3d');
        if (this.is3dView()) {
            this.drawBall(this.last.roll, this.last.pitch);
        }
    }

    update(orientation) {
        if (!this.widget || !orientation) return;
        const roll = Number.isFinite(orientation.roll) ? orientation.roll : 0;
        const pitch = Number.isFinite(orientation.pitch) ? orientation.pitch : 0;
        const yaw = this.normalizeYaw(orientation.yaw);

        this.last = { roll, pitch, yaw };

        if (this.nodes.rollText) this.nodes.rollText.innerText = `${roll.toFixed(1)}°`;
        if (this.nodes.pitchText) this.nodes.pitchText.innerText = `${pitch.toFixed(1)}°`;
        if (this.nodes.yawText) this.nodes.yawText.innerText = `${yaw.toFixed(0)}°`;

        this.setCenteredBar(this.nodes.rollBar, roll, 90);
        this.setCenteredBar(this.nodes.pitchBar, pitch, 90);
        if (this.nodes.yawBar) {
            const pct = Math.max(0, Math.min(1, yaw / 360));
            this.nodes.yawBar.style.width = `${pct * 100}%`;
            this.nodes.yawBar.style.left = '0';
        }

        if (this.nodes.roll3d) this.nodes.roll3d.innerText = roll.toFixed(0);
        if (this.nodes.pitch3d) this.nodes.pitch3d.innerText = pitch.toFixed(0);
        if (this.nodes.yaw3d) this.nodes.yaw3d.innerText = yaw.toFixed(0);

        if (this.is3dView()) {
            this.drawBall(roll, pitch);
        }
    }

    is3dView() {
        return this.widget && this.widget.classList.contains('show-3d');
    }

    setCenteredBar(node, value, range) {
        if (!node || range === 0) return;
        const clamp = Math.max(-range, Math.min(range, value));
        const pct = (Math.abs(clamp) / range) * 50;
        node.style.width = `${pct}%`;
        node.style.left = clamp >= 0 ? '50%' : `${50 - pct}%`;
    }

    normalizeYaw(yaw) {
        const y = Number.isFinite(yaw) ? yaw : 0;
        return ((y % 360) + 360) % 360;
    }

    drawBall(roll, pitch) {
        if (!this.ctx || !this.nodes.canvas) return;
        const { w, h } = this.size;
        const ctx = this.ctx;

        ctx.clearRect(0, 0, w, h);
        ctx.save();
        ctx.beginPath();
        ctx.arc(w / 2, h / 2, w / 2, 0, Math.PI * 2);
        ctx.clip();

        ctx.translate(w / 2, h / 2);
        ctx.rotate(-roll * Math.PI / 180);
        ctx.translate(0, pitch * 2);

        ctx.fillStyle = '#4FC3F7';
        ctx.fillRect(-100, -100, 200, 100);
        ctx.fillStyle = '#8D6E63';
        ctx.fillRect(-100, 0, 200, 100);
        ctx.strokeStyle = 'white';
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.moveTo(-90, 0);
        ctx.lineTo(90, 0);
        ctx.stroke();

        ctx.fillStyle = 'rgba(255,255,255,0.8)';
        for (let i = 20; i <= 90; i += 20) {
            let y = -i * 2;
            ctx.fillRect(-15, y - 1, 30, 2);
            y = i * 2;
            ctx.fillRect(-15, y - 1, 30, 2);
        }
        ctx.restore();

        ctx.strokeStyle = '#FFD700';
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.moveTo(w / 2 - 30, h / 2);
        ctx.lineTo(w / 2 - 10, h / 2);
        ctx.moveTo(w / 2 + 30, h / 2);
        ctx.lineTo(w / 2 + 10, h / 2);
        ctx.moveTo(w / 2, h / 2);
        ctx.arc(w / 2, h / 2, 2, 0, Math.PI * 2);
        ctx.stroke();

        ctx.strokeStyle = 'rgba(0,0,0,0.1)';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.arc(w / 2, h / 2, w / 2 - 1, 0, Math.PI * 2);
        ctx.stroke();
    }
}
