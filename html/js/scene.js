// Three.js 场景管理模块
class SceneManager {
    constructor(containerId) {
        console.log('[SceneManager] Initializing with container:', containerId);
        
        this.container = document.getElementById(containerId);
        if (!this.container) {
            throw new Error('Canvas container not found: ' + containerId);
        }
        console.log('[SceneManager] Container found:', this.container);
        
        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(0xf3f4f6);
        console.log('[SceneManager] Scene created');
        
        this.camera = null;
        this.renderer = null;
        this.controls = null;
        
        this.init();
        console.log('[SceneManager] Initialization complete');
    }

    init() {
        // 雾效 - lighter for clean look
        this.scene.fog = new THREE.Fog(0xf3f4f6, 20, 100);

        // 相机
        this.camera = new THREE.PerspectiveCamera(
            CONFIG.camera.fov,
            this.container.clientWidth / this.container.clientHeight,
            CONFIG.camera.near,
            CONFIG.camera.far
        );

        // 渲染器 - Enable shadows and better tone mapping
        this.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
        this.renderer.setSize(this.container.clientWidth, this.container.clientHeight);
        this.renderer.shadowMap.enabled = true;
        this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
        this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
        this.renderer.toneMappingExposure = 1.2;
        this.container.appendChild(this.renderer.domElement);

        // 控制器
        this.controls = new THREE.OrbitControls(this.camera, this.renderer.domElement);
        this.controls.enableDamping = CONFIG.controls.enableDamping;
        this.controls.dampingFactor = CONFIG.controls.dampingFactor;

        // 添加灯光
        this.addLights();
        
        // 添加坐标轴和网格
        this.addAxesAndGrid();

        // 处理窗口大小变化
        window.addEventListener('resize', () => this.onResize());
    }

    addLights() {
        // 环境光
        const ambiLight = new THREE.AmbientLight(0xffffff, 0.6);
        this.scene.add(ambiLight);
        
        // 主光源
        const mainLight = new THREE.DirectionalLight(0xffffff, 1.0);
        mainLight.position.set(10, 20, 10);
        mainLight.castShadow = true;
        mainLight.shadow.mapSize.width = 2048;
        mainLight.shadow.mapSize.height = 2048;
        mainLight.shadow.camera.near = 0.5;
        mainLight.shadow.camera.far = 50;
        this.scene.add(mainLight);

        // 补光 (Cool blue tint)
        const fillLight = new THREE.DirectionalLight(0xdbeafe, 0.6);
        fillLight.position.set(-10, 5, -10);
        this.scene.add(fillLight);
    }

    addAxesAndGrid() {
        // 坐标轴 - Thicker and cleaner
        const axX = this.createAxis(0xef4444, "X"); // Red-500
        axX.rotation.z = -Math.PI / 2;
        this.scene.add(axX);

        const axY = this.createAxis(0x10b981, "Y"); // Emerald-500
        this.scene.add(axY);

        const axZ = this.createAxis(0x3b82f6, "Z"); // Blue-500
        axZ.rotation.x = Math.PI / 2;
        this.scene.add(axZ);
        
        // 网格 - Blue theme
        const grid = new THREE.GridHelper(
            CONFIG.scene.gridSize,
            CONFIG.scene.gridDivisions,
            0x94a3b8, // Center line: Slate-400
            0xe2e8f0  // Grid lines: Slate-200
        );
        this.scene.add(grid);
    }

    createAxis(color, label) {
        const g = new THREE.Group();
        // Stick
        const rod = new THREE.Mesh(
            new THREE.CylinderGeometry(0.06, 0.06, 6, 16),
            new THREE.MeshStandardMaterial({
                color: color, 
                metalness: 0.1, 
                roughness: 0.2
            })
        );
        rod.position.y = 3;
        rod.castShadow = true;
        
        // Arrow head
        const cone = new THREE.Mesh(
            new THREE.ConeGeometry(0.2, 0.5, 32),
            new THREE.MeshStandardMaterial({
                color: color,
                metalness: 0.1,
                roughness: 0.2
            })
        );
        cone.position.y = 6;
        cone.castShadow = true;
        
        // Text Label
        const canvas = document.createElement('canvas');
        canvas.width = 128;
        canvas.height = 128;
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = "#" + color.toString(16).padStart(6, '0');
        ctx.font = "bold 90px Arial"; // Sans-serif
        ctx.textAlign = "center";
        ctx.fillText(label, 64, 100);
        
        const sprite = new THREE.Sprite(new THREE.SpriteMaterial({
            map: new THREE.CanvasTexture(canvas),
            transparent: true
        }));
        sprite.position.y = 6.8;
        sprite.scale.set(1.5, 1.5, 1.5);

        g.add(rod, cone, sprite);
        return g;
    }

    setCamera(mode) {
        let pos = {x: 0, y: 0, z: 0};
        const dist = CONFIG.camera.distance;

        switch(mode) {
            case 'iso':
                pos = {x: 8, y: 6, z: 10};
                break;
            case 'top':
                pos = {x: 0, y: dist, z: 0};
                break;
            case 'front':
                pos = {x: 0, y: 0, z: dist};
                break;
            case 'side':
                pos = {x: dist, y: 0, z: 0};
                break;
        }

        this.camera.position.set(pos.x, pos.y, pos.z);
        this.controls.target.set(0, 0, 0);
        
        if(mode === 'top') {
            this.camera.up.set(0, 0, -1);
        } else {
            this.camera.up.set(0, 1, 0);
        }

        this.camera.lookAt(0, 0, 0);
    }

    onResize() {
        this.camera.aspect = this.container.clientWidth / this.container.clientHeight;
        this.camera.updateProjectionMatrix();
        this.renderer.setSize(this.container.clientWidth, this.container.clientHeight);
    }

    render() {
        this.controls.update();
        this.renderer.render(this.scene, this.camera);
    }

    getScene() {
        return this.scene;
    }
}

