// Three.js 场景管理模块
class SceneManager {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.scene = new THREE.Scene();
        this.camera = null;
        this.renderer = null;
        this.controls = null;
        
        this.init();
    }

    init() {
        // 雾效
        this.scene.fog = new THREE.FogExp2(CONFIG.scene.fogColor, CONFIG.scene.fogDensity);

        // 相机
        this.camera = new THREE.PerspectiveCamera(
            CONFIG.camera.fov,
            this.container.clientWidth / this.container.clientHeight,
            CONFIG.camera.near,
            CONFIG.camera.far
        );

        // 渲染器
        this.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
        this.renderer.setSize(this.container.clientWidth, this.container.clientHeight);
        this.renderer.shadowMap.enabled = true;
        this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
        this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
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
        const ambiLight = new THREE.AmbientLight(0xffffff, 0.7);
        this.scene.add(ambiLight);
        
        // 主光源
        const mainLight = new THREE.DirectionalLight(0xffffff, 0.8);
        mainLight.position.set(10, 20, 10);
        mainLight.castShadow = true;
        mainLight.shadow.mapSize.width = 2048;
        mainLight.shadow.mapSize.height = 2048;
        this.scene.add(mainLight);

        // 补光
        const fillLight = new THREE.DirectionalLight(0x8e9eab, 0.5);
        fillLight.position.set(-10, 0, -10);
        this.scene.add(fillLight);
    }

    addAxesAndGrid() {
        // 坐标轴
        const axX = this.createAxis(0xe74c3c, "X");
        axX.rotation.z = -Math.PI / 2;
        this.scene.add(axX);

        const axY = this.createAxis(0x2ecc71, "Y");
        this.scene.add(axY);

        const axZ = this.createAxis(0x3498db, "Z");
        axZ.rotation.x = Math.PI / 2;
        this.scene.add(axZ);
        
        // 网格
        const grid = new THREE.GridHelper(
            CONFIG.scene.gridSize,
            CONFIG.scene.gridDivisions,
            0xbdc3c7,
            0xdfe4ea
        );
        this.scene.add(grid);
    }

    createAxis(color, label) {
        const g = new THREE.Group();
        const rod = new THREE.Mesh(
            new THREE.CylinderGeometry(0.04, 0.04, 6, 16),
            new THREE.MeshStandardMaterial({color: color, metalness: 0.2, roughness: 0.4})
        );
        rod.position.y = 3;
        rod.castShadow = true;
        
        const cone = new THREE.Mesh(
            new THREE.ConeGeometry(0.15, 0.4, 32),
            new THREE.MeshStandardMaterial({color: color})
        );
        cone.position.y = 6;
        
        // 文字
        const canvas = document.createElement('canvas');
        canvas.width = 128;
        canvas.height = 128;
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = "#" + color.toString(16).padStart(6, '0');
        ctx.font = "bold 100px Arial";
        ctx.textAlign = "center";
        ctx.fillText(label, 64, 100);
        const sprite = new THREE.Sprite(new THREE.SpriteMaterial({map: new THREE.CanvasTexture(canvas)}));
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

