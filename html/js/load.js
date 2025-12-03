// 基础节点和状态
const triggerZone = document.getElementById('trigger-zone');
const core = document.getElementById('titan-core');
const shock = document.getElementById('shockwave');
const ringLayer = document.getElementById('layer-rings');
const statusText = document.getElementById('status-text');
const transitionVeil = document.getElementById('transition-veil');
const veilStatus = document.getElementById('veil-status');

let isConnected = false;
let algoInterval;
let redirectTimer;
let veilStatusTimer;

// 鼠标交互：视差 + 姿态仪控制
document.addEventListener('mousemove', (e) => {
  const layers = document.querySelectorAll('.cockpit-layer');
  const cx = window.innerWidth / 2;
  const cy = window.innerHeight / 2;
  const x = (e.clientX - cx) / cx;
  const y = (e.clientY - cy) / cy;
  
  layers.forEach(layer => {
    const d = layer.getAttribute('data-depth');
    let tr = `translate(${x*20*d}px, ${y*15*d}px)`;
    if (layer.id === 'layer-bg') tr += ' scale(1.1)';
    layer.style.transform = tr;
  });

  // 只有在未锁定状态下，内圈才跟随鼠标摆动
  if (!isConnected) {
    const innerRing = document.querySelector('.ring-inner');
    if (innerRing) {
      const rot = x * 15; 
      const pitch = 1 - (Math.abs(y) * 0.1); 
      innerRing.style.transform = `rotate(${rot}deg) scale(${pitch})`;
      innerRing.style.animation = 'none';
    }
  } else {
    const innerRing = document.querySelector('.ring-inner');
    if (innerRing) innerRing.style.transform = '';
    if (innerRing) innerRing.style.animation = '';
  }
});

// 模拟算法数据流
function startAlgoStream() {
  const chars = "01ABCDEFXYZ";
  clearInterval(algoInterval);
  algoInterval = setInterval(() => {
    const left = document.getElementById('algo-left');
    const right = document.getElementById('algo-right');
    if (!left || !right) return;
    left.innerHTML = Array(3).fill(0).map(() => Array(4).fill(0).map(()=>chars[Math.floor(Math.random()*chars.length)]).join('')).join('<br>');
    right.innerHTML = Array(3).fill(0).map(() => Array(4).fill(0).map(()=>chars[Math.floor(Math.random()*chars.length)]).join('')).join('<br>');
  }, 80);
}

function stopAlgoStream() {
  if (algoInterval) {
    clearInterval(algoInterval);
    algoInterval = null;
  }
}

function startRedirectSequence() {
  if (redirectTimer) return;

  document.body.classList.add('redirecting');

  if (transitionVeil) {
    transitionVeil.classList.add('active');
    if (veilStatusTimer) clearTimeout(veilStatusTimer);
    if (veilStatus) {
      veilStatus.textContent = '光阈开启...';
    }
    veilStatusTimer = setTimeout(() => {
      transitionVeil.classList.add('ready');
      if (veilStatus) {
        veilStatus.textContent = '';
      }
    }, 500);
  }

  redirectTimer = setTimeout(() => {
    if (transitionVeil) transitionVeil.classList.add('depart');
    window.location.href = 'index.html';
  }, 1500);
}

function toggleConnect() {
  if (redirectTimer) return;

  if (shock) {
    shock.classList.remove('active');
    void shock.offsetWidth; 
    shock.classList.add('active');
  }

  if (!isConnected) {
    isConnected = true;
    
    document.body.classList.add('mode-armed'); 
    if (core) core.classList.add('engaged');
    if (ringLayer) ringLayer.classList.add('engaged-rings');
    
    if (statusText) {
      statusText.innerText = "MIO: SYNCHRONIZED"; 
      statusText.style.background = "#00a8ff"; 
      statusText.style.color = "#fff";
      statusText.style.boxShadow = "0 0 15px rgba(0, 168, 255, 0.4)";
    }
    
    startAlgoStream();
    startRedirectSequence();

  } else {
    isConnected = false;
    document.body.classList.remove('mode-armed');
    if (core) core.classList.remove('engaged');
    if (ringLayer) ringLayer.classList.remove('engaged-rings');
    stopAlgoStream();
    
    if (statusText) {
      statusText.innerText = "SYSTEM STANDBY";
      statusText.style.background = "rgba(41, 128, 185, 0.1)";
      statusText.style.color = "#2980b9";
      statusText.style.boxShadow = "none";
    }
  }
}

if (triggerZone) {
  triggerZone.addEventListener('click', toggleConnect);
}

// ============================================
    // 1. 简易柏林噪声库 (Perlin Noise Helper)
    //    用于生成自然的随机波动
    // ============================================
    class Noise {
      constructor(seed) {
        this.grad3 = [[1,1,0],[-1,1,0],[1,-1,0],[-1,-1,0],[1,0,1],[-1,0,1],[1,0,-1],[-1,0,-1],[0,1,1],[0,-1,1],[0,1,-1],[0,-1,-1]];
        this.p = []; this.perm = []; 
        for(let i=0; i<256; i++) this.p[i] = Math.floor(seed * 256);
        for(let i=0; i<512; i++) this.perm[i] = this.p[i & 255];
      }
      dot(g, x, y) { return g[0]*x + g[1]*y; }
      mix(a, b, t) { return (1-t)*a + t*b; }
      fade(t) { return t*t*t*(t*(t*6-15)+10); }
      
      perlin2(x, y) {
        let X = Math.floor(x), Y = Math.floor(y);
        x = x - X; y = y - Y;
        X = X & 255; Y = Y & 255;
        let n00 = this.dot(this.grad3[this.perm[X+this.perm[Y]] % 12], x, y);
        let n01 = this.dot(this.grad3[this.perm[X+this.perm[Y+1]] % 12], x, y-1);
        let n10 = this.dot(this.grad3[this.perm[X+1+this.perm[Y]] % 12], x-1, y);
        let n11 = this.dot(this.grad3[this.perm[X+1+this.perm[Y+1]] % 12], x-1, y-1);
        let u = this.fade(x), v = this.fade(y);
        return this.mix(this.mix(n00, n10, u), this.mix(n01, n11, u), v);
      }
    }

    // ============================================
    // 2. AWaves 组件逻辑
    //    实现垂直线条随鼠标波动的效果
    // ============================================
    class AWaves extends HTMLElement {
      connectedCallback() {
        this.svg = this.querySelector('.js-svg');
        this.mouse = { x: -10, y: 0, lx: 0, ly: 0, sx: 0, sy: 0, v: 0, vs: 0, a: 0, set: false };
        this.lines = [];
        this.paths = [];
        this.noise = new Noise(Math.random());
        this.setSize();
        this.setLines();
        this.bindEvents();
        requestAnimationFrame(this.tick.bind(this));
      }

      bindEvents() {
        window.addEventListener('resize', this.onResize.bind(this));
        window.addEventListener('mousemove', this.onMouseMove.bind(this));
        window.addEventListener('touchmove', this.onTouchMove.bind(this));
      }

      onResize() {
        this.setSize();
        this.setLines();
      }

      onMouseMove(e) { this.updateMousePosition(e.pageX, e.pageY); }
      onTouchMove(e) {
        const touch = e.touches[0];
        this.updateMousePosition(touch.clientX, touch.clientY);
      }

      updateMousePosition(x, y) {
        const { mouse } = this;
        mouse.x = x - this.bounding.left;
        mouse.y = y - this.bounding.top + window.scrollY;
        if (!mouse.set) {
          mouse.sx = mouse.x; mouse.sy = mouse.y;
          mouse.lx = mouse.x; mouse.ly = mouse.y;
          mouse.set = true;
        }
      }

      setSize() {
        this.bounding = this.getBoundingClientRect();
        this.svg.style.width = `${this.bounding.width}px`;
        this.svg.style.height = `${this.bounding.height}px`;
      }

      // ★★★ 修改点 1：增加线条密度 ★★★
      setLines() {
        const { width, height } = this.bounding;
        this.lines = [];
        this.paths.forEach(p => p.remove());
        this.paths = [];

        // xGap: 横向间距。之前是60，改为 20，线条数量会增加3倍
        const xGap =15; 
        // yGap: 纵向采样点间距。改为 20，让曲线更细腻，不会有棱角
        const yGap =15; 

        const oWidth = width + 200;
        const oHeight = height + 30;
        const totalLines = Math.ceil(oWidth / xGap);
        const totalPoints = Math.ceil(oHeight / yGap);
        const xStart = (width - xGap * totalLines) / 2;
        const yStart = (height - yGap * totalPoints) / 2;

        for (let i = 0; i <= totalLines; i++) {
          const points = [];
          for (let j = 0; j <= totalPoints; j++) {
            points.push({
              x: xStart + xGap * i,
              y: yStart + yGap * j,
              wave: { x: 0, y: 0 },
              cursor: { x: 0, y: 0, vx: 0, vy: 0 },
            });
          }
          const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
          this.svg.appendChild(path);
          this.paths.push(path);
          this.lines.push(points);
        }
      }

      movePoints(time) {
        const { lines, mouse, noise } = this;
        lines.forEach(points => {
          points.forEach(p => {
            // ★ 调整：降低波动的幅度 (*8)，让它看起来像平静的水面，而不是狂风巨浪
            const move = noise.perlin2((p.x + time * 0.0125) * 0.002, (p.y + time * 0.005) * 0.0015) * 15;
            p.wave.x = Math.cos(move) * 16;
            p.wave.y = Math.sin(move) * 8;

            const dx = p.x - mouse.sx;
            const dy = p.y - mouse.sy;
            const d = Math.hypot(dx, dy);
            const l = Math.max(175, mouse.vs);

            if (d < l) {
              const s = 1 - d / l;
              const f = Math.cos(d * 0.001) * s;
              p.cursor.vx += Math.cos(mouse.a) * f * l * mouse.vs * 0.00065;
              p.cursor.vy += Math.sin(mouse.a) * f * l * mouse.vs * 0.00065;
            }

            p.cursor.vx += (0 - p.cursor.x) * 0.005;
            p.cursor.vy += (0 - p.cursor.y) * 0.005;
            p.cursor.vx *= 0.925;
            p.cursor.vy *= 0.925;
            p.cursor.x += p.cursor.vx * 2;
            p.cursor.y += p.cursor.vy * 2;
            p.cursor.x = Math.min(100, Math.max(-100, p.cursor.x));
            p.cursor.y = Math.min(100, Math.max(-100, p.cursor.y));
          });
        });
      }

      moved(point, withCursorForce = true) {
        const coords = {
          x: point.x + point.wave.x + (withCursorForce ? point.cursor.x : 0),
          y: point.y + point.wave.y + (withCursorForce ? point.cursor.y : 0),
        };
        coords.x = Math.round(coords.x * 10) / 10;
        coords.y = Math.round(coords.y * 10) / 10;
        return coords;
      }

      // ★★★ 修改点 2：使用贝塞尔曲线 (Q) 实现平滑连接 ★★★
      drawLines() {
        const { lines, moved, paths } = this;
        lines.forEach((points, lIndex) => {
          let p1 = moved(points[0], false);
          let d = `M ${p1.x} ${p1.y}`;

          points.forEach((p1, pIndex) => {
            const isLast = pIndex === points.length - 1;
            p1 = moved(p1, !isLast);
            
            // 获取下一个点，如果是最后一个点，就重用当前点
            const p2 = moved(
              points[pIndex + 1] || points[points.length - 1],
              !isLast
            );

            // ★ 核心改动：使用 Q (Quadratic Curve) 二次贝塞尔曲线
            // p1 是控制点，(p1+p2)/2 是终点
            // 这样可以画出非常顺滑的丝绸感线条
            d += `Q ${p1.x} ${p1.y} ${(p1.x + p2.x) / 2} ${(p1.y + p2.y) / 2} `;
          });

          paths[lIndex].setAttribute('d', d);
        });
      }

      tick(time) {
        const { mouse } = this;
        mouse.sx += (mouse.x - mouse.sx) * 0.1;
        mouse.sy += (mouse.y - mouse.sy) * 0.1;
        const dx = mouse.x - mouse.lx;
        const dy = mouse.y - mouse.ly;
        const d = Math.hypot(dx, dy);
        mouse.v = d;
        mouse.vs += (d - mouse.vs) * 0.1;
        mouse.vs = Math.min(100, mouse.vs);
        mouse.lx = mouse.x; mouse.ly = mouse.y;
        mouse.a = Math.atan2(dy, dx);
        this.style.setProperty('--x', `${mouse.sx}px`);
        this.style.setProperty('--y', `${mouse.sy}px`);
        this.movePoints(time);
        this.drawLines();
        requestAnimationFrame(this.tick.bind(this));
      }
    }
    customElements.define('a-waves', AWaves);
