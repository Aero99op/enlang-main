
let terminalStep = 0;

let mouseX = 0;

let mouseY = 0;

let targetX = 0;

let targetY = 0;

let orbitTime = 0;

console.log("⚡ Initializing Prayas Kumar Sahoo 3D Cyber Portfolio Engine in Pure Enlang (.enlgs)...");

function createSunTexture() {
  let c = document.createElement ( "canvas" );
  c.width = 1024;
  c.height = 512;
  let ctx = c.getContext ( "2d" );
  let grad = ctx.createLinearGradient ( 0 , 0 , 0 , 512 );
  grad.addColorStop(0, "#ff2200")
  grad.addColorStop(0.25, "#ff6600")
  grad.addColorStop(0.5, "#ffaa00")
  grad.addColorStop(0.75, "#ffdd33")
  grad.addColorStop(1, "#ff4400")
  ctx.fillStyle = grad;
  ctx.fillRect(0, 0, 1024, 512)
  for (let _i1 = 0; _i1 < 1800; _i1++) {
    let x = Math.random ( ) * 1024;
    let y = Math.random ( ) * 512;
    let r = 2 + Math.random ( ) * 20;
    let alpha = 0.15 + Math.random ( ) * 0.4;
    if (Math.random ( ) > 0.4) {
      ctx.fillStyle = "rgba(255, 255, 220, " + alpha + ")";
    }
    else {
      ctx.fillStyle = "rgba(255, 60, 0, " + alpha + ")";
    }
    ctx.beginPath()
    ctx.arc(x, y, r, 0, Math.PI * 2)
    ctx.fill()
  }
  for (let _i2 = 0; _i2 < 22; _i2++) {
    let sx = Math.random ( ) * 1024;
    let sy = 100 + Math.random ( ) * 312;
    let sr = 6 + Math.random ( ) * 16;
    ctx.fillStyle = "rgba(45, 8, 0, 0.85)";
    ctx.beginPath()
    ctx.arc(sx, sy, sr, 0, Math.PI * 2)
    ctx.fill()
    ctx.strokeStyle = "rgba(255, 120, 0, 0.6)";
    ctx.lineWidth = 4;
    ctx.stroke()
  }
  let tex = new THREE.CanvasTexture ( c );
  tex.wrapS = THREE.RepeatWrapping;
  tex.wrapT = THREE.RepeatWrapping;
  return tex;
}

function createMoonTexture() {
  let c = document.createElement ( "canvas" );
  c.width = 1024;
  c.height = 512;
  let ctx = c.getContext ( "2d" );
  ctx.fillStyle = "#8a96a6";
  ctx.fillRect(0, 0, 1024, 512)
  let maria = [ { x : 350 , y : 200 , r : 130 } , { x : 270 , y : 300 , r : 95 } , { x : 490 , y : 170 , r : 120 } , { x : 660 , y : 220 , r : 145 } , { x : 770 , y : 320 , r : 85 } , { x : 180 , y : 350 , r : 75 } ];
  for (const m of maria) {
    let mg = ctx.createRadialGradient ( m.x , m.y , 8 , m.x , m.y , m.r );
    mg.addColorStop(0, "rgba(40, 50, 65, 0.9)")
    mg.addColorStop(0.65, "rgba(55, 65, 80, 0.7)")
    mg.addColorStop(1, "rgba(138, 150, 166, 0)")
    ctx.fillStyle = mg;
    ctx.beginPath()
    ctx.arc(m.x, m.y, m.r, 0, Math.PI * 2)
    ctx.fill()
  }
  for (let _i3 = 0; _i3 < 2400; _i3++) {
    let x = Math.random ( ) * 1024;
    let y = Math.random ( ) * 512;
    let r = 1 + Math.random ( ) * 6;
    if (Math.random ( ) > 0.5) {
      ctx.fillStyle = "rgba(255, 255, 255, 0.3)";
    }
    else {
      ctx.fillStyle = "rgba(15, 20, 30, 0.35)";
    }
    ctx.beginPath()
    ctx.arc(x, y, r, 0, Math.PI * 2)
    ctx.fill()
  }
  let craters = [ { x : 310 , y : 380 , r : 26 , rays : true } , { x : 430 , y : 140 , r : 20 , rays : false } , { x : 560 , y : 190 , r : 18 , rays : false } , { x : 790 , y : 260 , r : 22 , rays : false } , { x : 210 , y : 180 , r : 17 , rays : false } , { x : 890 , y : 380 , r : 24 , rays : false } ];
  for (const cr of craters) {
    if (cr.rays == true) {
      ctx.strokeStyle = "rgba(255, 255, 255, 0.4)";
      ctx.lineWidth = 1.5;
      for (let _i4 = 0; _i4 < 14; _i4++) {
        let angle = ( Math.random ( ) ) * Math.PI * 2;
        ctx.beginPath()
        ctx.moveTo(cr.x, cr.y)
        ctx.lineTo(cr.x + Math.cos(angle) * 180, cr.y + Math.sin(angle) * 180)
        ctx.stroke()
      }
    }
    ctx.fillStyle = "rgba(12, 16, 24, 0.85)";
    ctx.beginPath()
    ctx.arc(cr.x, cr.y, cr.r, 0, Math.PI * 2)
    ctx.fill()
    ctx.strokeStyle = "rgba(255, 255, 255, 0.8)";
    ctx.lineWidth = 3;
    ctx.stroke()
  }
  let tex = new THREE.CanvasTexture ( c );
  return tex;
}

function init3D() {
  if (typeof THREE == "undefined") {
    setTimeout(function() {
      init3D();
    }, 150);
    return;
  }
  let canvas = (document.getElementById("webgl-canvas") || document.querySelector("webgl-canvas"));
  if (! canvas) {
    setTimeout(function() {
      init3D();
    }, 150);
    return;
  }
  let scene = new THREE.Scene ( );
  let camera = new THREE.PerspectiveCamera ( 60 , window.innerWidth / window.innerHeight , 0.1 , 1000 );
  let renderer = new THREE.WebGLRenderer ( { canvas : canvas , antialias : true , alpha : true } );
  renderer.setSize(window.innerWidth, window.innerHeight)
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))
  let ambientLight = new THREE.AmbientLight ( "#475569" , 0.65 );
  scene.add(ambientLight)
  let sunLight = new THREE.PointLight ( "#ffeedd" , 2.2 , 80 );
  sunLight.position.set(7.0, 3.5, -4.5)
  scene.add(sunLight)
  let particlesCount = 2500;
  let positions = new Float32Array ( particlesCount * 3 );
  let colors = new Float32Array ( particlesCount * 3 );
  for (let _i5 = 0; _i5 < 2500; _i5++) {
    let i = ( Math.floor ( Math.random ( ) * 2500 ) ) * 3;
    positions[i] = (Math.random() - 0.5) * 45
    positions[i + 1] = (Math.random() - 0.5) * 45
    positions[i + 2] = (Math.random() - 0.5) * 45
    let rChoice = Math.random ( );
    if (rChoice < 0.45) {
      colors[i] = 0.0
      colors[i + 1] = 0.95
      colors[i + 2] = 1.0
    }
    else if (rChoice < 0.8) {
      colors[i] = 0.7
      colors[i + 1] = 0.35
      colors[i + 2] = 1.0
    }
    else {
      colors[i] = 1.0
      colors[i + 1] = 0.85
      colors[i + 2] = 0.4
    }
  }
  let particlesGeometry = new THREE.BufferGeometry ( );
  particlesGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3))
  particlesGeometry.setAttribute('color', new THREE.BufferAttribute(colors, 3))
  let particlesMaterial = new THREE.PointsMaterial ( { size : 0.11 , vertexColors : true , transparent : true , opacity : 0.85 } );
  let particlesMesh = new THREE.Points ( particlesGeometry , particlesMaterial );
  scene.add(particlesMesh)
  let sunTex = createSunTexture ( );
  let sunGeo = new THREE.SphereGeometry ( 2.5 , 64 , 64 );
  let sunMat = new THREE.MeshBasicMaterial ( { map : sunTex } );
  let sunMesh = new THREE.Mesh ( sunGeo , sunMat );
  sunMesh.position.set(7.0, 3.5, -5.5)
  scene.add(sunMesh)
  let sunCoronaGeo1 = new THREE.SphereGeometry ( 2.78 , 32 , 32 );
  let sunCoronaMat1 = new THREE.MeshBasicMaterial ( { color : "#ff8800" , transparent : true , opacity : 0.45 , blending : THREE.AdditiveBlending } );
  let sunCorona1 = new THREE.Mesh ( sunCoronaGeo1 , sunCoronaMat1 );
  sunMesh.add(sunCorona1)
  let sunCoronaGeo2 = new THREE.SphereGeometry ( 3.3 , 32 , 32 );
  let sunCoronaMat2 = new THREE.MeshBasicMaterial ( { color : "#ffbb00" , transparent : true , opacity : 0.22 , blending : THREE.AdditiveBlending } );
  let sunCorona2 = new THREE.Mesh ( sunCoronaGeo2 , sunCoronaMat2 );
  sunMesh.add(sunCorona2)
  let moonTex = createMoonTexture ( );
  let moonGeo = new THREE.SphereGeometry ( 1.75 , 64 , 64 );
  let moonMat = new THREE.MeshStandardMaterial ( { map : moonTex , roughness : 0.85 , metalness : 0.1 } );
  let moonMesh = new THREE.Mesh ( moonGeo , moonMat );
  moonMesh.position.set(-6.8, -1.8, -5.0)
  scene.add(moonMesh)
  let moonHaloGeo = new THREE.SphereGeometry ( 1.9 , 32 , 32 );
  let moonHaloMat = new THREE.MeshBasicMaterial ( { color : "#38bdf8" , transparent : true , opacity : 0.2 , blending : THREE.AdditiveBlending } );
  let moonHalo = new THREE.Mesh ( moonHaloGeo , moonHaloMat );
  moonMesh.add(moonHalo)
  let ringGeo = new THREE.TorusGeometry ( 2.6 , 0.04 , 16 , 100 );
  let ringMat = new THREE.MeshBasicMaterial ( { color : "#00f2fe" , transparent : true , opacity : 0.35 } );
  let ringMesh = new THREE.Mesh ( ringGeo , ringMat );
  ringMesh.rotation.x = Math.PI / 2.5;
  ringMesh.rotation.y = Math.PI / 6;
  moonMesh.add(ringMesh)
  let geom1 = new THREE.IcosahedronGeometry ( 1.4 , 1 );
  let mat1 = new THREE.MeshBasicMaterial ( { color : "#00f2fe" , wireframe : true , transparent : true , opacity : 0.25 } );
  let poly1 = new THREE.Mesh ( geom1 , mat1 );
  poly1.position.set(2.0, -3.8, -7.0)
  scene.add(poly1)
  let geom2 = new THREE.TorusKnotGeometry ( 1.2 , 0.3 , 64 , 8 );
  let mat2 = new THREE.MeshBasicMaterial ( { color : "#a855f7" , wireframe : true , transparent : true , opacity : 0.2 } );
  let poly2 = new THREE.Mesh ( geom2 , mat2 );
  poly2.position.set(-2.5, 3.8, -8.0)
  scene.add(poly2)
  camera.position.z = 7;
  function render3DScene() {
    requestAnimationFrame(render3DScene)
    orbitTime = orbitTime + 0.008;
    targetX = targetX + ( mouseX - targetX ) * 0.05;
    targetY = targetY + ( mouseY - targetY ) * 0.05;
    particlesMesh.rotation.y = particlesMesh.rotation.y + 0.0008;
    particlesMesh.rotation.x = particlesMesh.rotation.x + 0.0004;
    sunMesh.rotation.y = sunMesh.rotation.y + 0.005;
    sunMesh.rotation.x = sunMesh.rotation.x + 0.002;
    sunCorona1.rotation.y = sunCorona1.rotation.y - 0.008;
    sunCorona2.rotation.z = sunCorona2.rotation.z + 0.004;
    sunTex.offset.x = ( sunTex.offset.x + 0.001 ) % 1;
    sunMesh.position.x = 7.0 + targetX * 1.6;
    sunMesh.position.y = 3.5 - targetY * 1.6 + Math.sin ( orbitTime ) * 0.25;
    sunLight.position.copy(sunMesh.position)
    moonMesh.rotation.y = moonMesh.rotation.y + 0.004;
    ringMesh.rotation.z = ringMesh.rotation.z + 0.01;
    moonMesh.position.x = - 6.8 + targetX * 1.4;
    moonMesh.position.y = - 1.8 - targetY * 1.4 + Math.cos ( orbitTime ) * 0.2;
    poly1.rotation.x = poly1.rotation.x + 0.008;
    poly1.rotation.y = poly1.rotation.y + 0.012;
    poly2.rotation.x = poly2.rotation.x - 0.006;
    poly2.rotation.y = poly2.rotation.y + 0.009;
    camera.position.x = targetX * 0.7;
    camera.position.y = - targetY * 0.7;
    camera.lookAt(scene.position)
    renderer.render(scene, camera)
  }
  render3DScene();
}

window.addEventListener('mousemove', function(event) {
  mouseX = ( event.clientX / window.innerWidth - 0.5 ) * 2;
  mouseY = ( event.clientY / window.innerHeight - 0.5 ) * 2;
});

window.addEventListener('resize', function(event) {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix()
  renderer.setSize(window.innerWidth, window.innerHeight)
});

init3D();

function showToast(message) {
  (document.getElementById("toast-alert") || document.querySelector("toast-alert")).textContent = message;
  (document.getElementById("toast-alert") || document.querySelector("toast-alert")).style.display = "block"
  setTimeout(function() {
    (document.getElementById("toast-alert") || document.querySelector("toast-alert")).style.display = "none"
  }, 3000);
}

(document.getElementById("btn-copy-phone") || document.querySelector("btn-copy-phone")).addEventListener('click', function(event) {
  navigator.clipboard.writeText("8260350182");
  showToast("📞 Phone number copied: 8260350182");
});

(document.getElementById("btn-copy-email") || document.querySelector("btn-copy-email")).addEventListener('click', function(event) {
  navigator.clipboard.writeText("prayaskumarsahoo45@gmail.com");
  showToast("✉️ Email copied: prayaskumarsahoo45@gmail.com");
});

const codeSnippets = [ "// [1] Booting Spring Boot 3.2.0 Microservice..." , "@RestController @RequestMapping(\"/api/v1/lms\")npublic class CourseEnrollmentController {n    @Autowired private EnrollmentService service;n    @PostMapping(\"/enroll\")n    public ResponseEntity<ApiResponse> enrollStudent(@Valid @RequestBody StudentDTO dto) {n        return ResponseEntity.ok(service.processEnrollment(dto));n    }n}" , "// [2] Connecting Oracle & MySQL Production Clusters...n[DB POOL] HikariCP initialized (10 active connections, latency 1.2ms)n[AUTH] Spring Security JWT Filter Chain Active [200 OK]" , "// [3] Running Payroll Calculation Engine...n[PAYROLL ENGINE] Tax brackets calculated. Stored procedure executed successfully.nStatus: ALL SYSTEMS OPERATIONAL (Java 21 Virtual Threads Enabled)" ];

function advanceTerminal() {
  terminalStep = ( terminalStep + 1 ) % 4;
  (document.getElementById("terminal-output") || document.querySelector("terminal-output")).textContent = codeSnippets [ terminalStep ];
  showToast("💻 Loaded Backend Module [" + ( terminalStep + 1 ) + "/4]");
}

(document.getElementById("btn-term-next") || document.querySelector("btn-term-next")).addEventListener('click', function(event) {
  advanceTerminal();
});

(document.getElementById("contact-form") || document.querySelector("contact-form")).addEventListener('submit', function(event) {
  if (typeof event !== 'undefined' && event.preventDefault) event.preventDefault();
  let senderName = (document.getElementById("contact-name") || document.querySelector("contact-name")).value;
  let senderEmail = (document.getElementById("contact-email") || document.querySelector("contact-email")).value;
  let senderMsg = (document.getElementById("contact-message") || document.querySelector("contact-message")).value;
  if (senderName == "") {
    alert("Please enter your name");
    return;
  }
  if (senderEmail == "") {
    alert("Please enter your email");
    return;
  }
  showToast("🚀 Message sent! Thanks " + senderName + ", Prayas will reply soon.");
  (document.getElementById("contact-status") || document.querySelector("contact-status")).textContent = "✅ Inquiry submitted successfully. Thank you!";
  (document.getElementById("contact-name") || document.querySelector("contact-name")).value = "";
  (document.getElementById("contact-email") || document.querySelector("contact-email")).value = "";
  (document.getElementById("contact-message") || document.querySelector("contact-message")).value = "";
});

setInterval(function() {
  (document.getElementById("live-clock") || document.querySelector("live-clock")).textContent = "Engine Active • Asia/Kolkata • " + new Date ( ) .toLocaleTimeString ( );
}, 1000);

console.log("⚡ 3D Sun & Moon Celestial Scene is 100% Active in Pure Enlang Script (.enlgs)!");
    