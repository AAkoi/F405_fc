class ImuCard {
    constructor(rootId = 'imu-card') {
        this.root = document.getElementById(rootId);
        if (!this.root) return;

        this.isChartView = false;
        this.chartContainer = this.root.querySelector('#chartViewContainer');
        this.sensorSelect = this.root.querySelector('#sensorSelector');
        this.iconChart = this.root.querySelector('#icon-chart');
        this.iconTable = this.root.querySelector('#icon-table');

        this.charts = {
            gyro: new LineChartMini('canvas-gyro'),
            acc: new LineChartMini('canvas-acc'),
            mag: new LineChartMini('canvas-mag')
        };

        this.bindEvents();
        window.addEventListener('resize', () => this.resizeCharts());
    }

    bindEvents() {
        const toggleBtn = this.root.querySelector('#toggleBtn');
        if (toggleBtn) {
            toggleBtn.addEventListener('click', () => {
                this.isChartView = !this.isChartView;
                if (this.isChartView) {
                    this.root.classList.add('view-mode-chart');
                    this.iconChart.style.display = 'none';
                    this.iconTable.style.display = 'block';
                    setTimeout(() => this.resizeCharts(), 80);
                } else {
                    this.root.classList.remove('view-mode-chart');
                    this.iconChart.style.display = 'block';
                    this.iconTable.style.display = 'none';
                }
            });
        }

        if (this.sensorSelect) {
            this.sensorSelect.addEventListener('change', (e) => {
                const filter = e.target.value;
                if (this.chartContainer) {
                    this.chartContainer.setAttribute('data-filter', filter);
                    setTimeout(() => this.resizeCharts(), 260);
                }
            });
        }
    }

    resizeCharts() {
        Object.values(this.charts).forEach((c) => c && c.resize());
    }

    update(sensorData) {
        if (!this.root || !sensorData) return;
        const gyr = [sensorData.gyr?.x, sensorData.gyr?.y, sensorData.gyr?.z];
        const acc = [sensorData.acc?.x, sensorData.acc?.y, sensorData.acc?.z];
        const mag = [sensorData.mag?.x, sensorData.mag?.y, sensorData.mag?.z];

        this.setTriplet('t-g', gyr);
        this.setTriplet('t-a', acc);
        this.setTriplet('t-m', mag);
        this.setTriplet('c-g', gyr, true);
        this.setTriplet('c-a', acc, true);
        this.setTriplet('c-m', mag, true);

        this.charts.gyro.update(gyr);
        this.charts.acc.update(acc);
        this.charts.mag.update(mag);
    }

    setTriplet(prefix, arr, small = false) {
        if (!arr) return;
        const fmt = (v) => (Number.isFinite(v) ? (small ? v.toFixed(2) : v.toFixed(2)) : '--');
        const ids = ['x', 'y', 'z'];
        ids.forEach((axis, i) => {
            const el = document.getElementById(`${prefix}-${axis}`);
            if (el) el.innerText = fmt(arr[i]);
        });
    }
}

class LineChartMini {
    constructor(canvasId, maxPoints = 80) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas?.getContext('2d');
        this.maxPoints = maxPoints;
        this.data = [[], [], []];
        this.colors = ['#ef4444', '#10b981', '#3b82f6'];
        this.width = 0;
        this.height = 0;
        this.rangeMin = null;
        this.rangeMax = null;
        this.rangeLerp = 0.15;
        this.minSpan = 1;
        this.resize();
    }

    resize() {
        if (!this.canvas || !this.ctx || !this.canvas.parentElement) return;
        const rect = this.canvas.parentElement.getBoundingClientRect();
        const dpr = window.devicePixelRatio || 1;
        this.canvas.width = rect.width * dpr;
        this.canvas.height = rect.height * dpr;
        this.canvas.style.width = rect.width + 'px';
        this.canvas.style.height = rect.height + 'px';
        this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
        this.width = rect.width;
        this.height = rect.height;
        this.draw();
    }

    update(values = []) {
        values.forEach((val, i) => {
            const num = Number.isFinite(val) ? val : 0;
            this.data[i].push(num);
            if (this.data[i].length > this.maxPoints) this.data[i].shift();
        });
        this.updateRange();
        this.draw();
    }

    updateRange() {
        const all = this.data.flat();
        if (!all.length) return;
        let max = Math.max(...all);
        let min = Math.min(...all);
        if (max === min) {
            max += 0.1;
            min -= 0.1;
        }
        const span = Math.max(Math.abs(max), Math.abs(min), this.minSpan);
        const targetMin = -span * 1.1;
        const targetMax = span * 1.1;
        if (this.rangeMin === null || this.rangeMax === null) {
            this.rangeMin = targetMin;
            this.rangeMax = targetMax;
        } else {
            this.rangeMin = this.rangeMin + this.rangeLerp * (targetMin - this.rangeMin);
            this.rangeMax = this.rangeMax + this.rangeLerp * (targetMax - this.rangeMax);
        }
    }

    draw() {
        if (!this.ctx) return;
        const { ctx, width, height, data } = this;
        ctx.clearRect(0, 0, width, height);
        ctx.beginPath();
        ctx.strokeStyle = '#e2e8f0';
        ctx.moveTo(0, height / 2);
        ctx.lineTo(width, height / 2);
        ctx.stroke();

        if (this.rangeMin === null || this.rangeMax === null) return;
        const min = this.rangeMin;
        const max = this.rangeMax;
        const range = max - min;
        const stepX = width / (this.maxPoints - 1);

        data.forEach((line, idx) => {
            if (line.length < 2) return;
            ctx.beginPath();
            ctx.strokeStyle = this.colors[idx];
            ctx.lineWidth = 1.5;
            line.forEach((v, i) => {
                const y = height - ((v - min) / range) * height;
                if (i === 0) ctx.moveTo(0, y);
                else ctx.lineTo(i * stepX, y);
            });
            ctx.stroke();
        });
    }
}
