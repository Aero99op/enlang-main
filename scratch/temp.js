
let terminalStep = 0;

console.log("⚡ Initializing Prayas Kumar Sahoo 3D Cyber Portfolio Engine...");

;function init3D() {

;if (typeof THREE === "undefined") {

setTimeout(init3D, 200);

return ;

}

const canvas = document.getElementById ( "webgl-canvas" );

;if (!canvas) return;

const scene = new THREE.Scene ( );

const camera = new THREE.PerspectiveCamera ( 60 , window.innerWidth / window.innerHeight , 0.1 , 1000 );

const renderer = new THREE.WebGLRenderer ( { canvas : canvas , antialias : true , alpha : true } );

renderer.setSize(window.innerWidth, window.innerHeight);

renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));

const particlesCount = 1200;

const positions = new Float32Array ( particlesCount * 3 );

const colors = new Float32Array ( particlesCount * 3 );

;for (let i = 0; i < particlesCount * 3; i += 3) {

positions[i] = (Math.random() - 0.5) * 30;

positions[i + 1] = (Math.random() - 0.5) * 30;

positions[i + 2] = (Math.random() - 0.5) * 30;

colors[i] = 0.0;

colors[i + 1] = 0.95;

colors[i + 2] = 1.0;

}

const particlesGeometry = new THREE.BufferGeometry ( );

particlesGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));

particlesGeometry.setAttribute('color', new THREE.BufferAttribute(colors, 3));

const particlesMaterial = new THREE.PointsMaterial ( { size : 0.06 , vertexColors : true , transparent : true , opacity : 0.75 } );

const particlesMesh = new THREE.Points ( particlesGeometry , particlesMaterial );

scene.add(particlesMesh);

const geom1 = new THREE.IcosahedronGeometry ( 2.2 , 1 );

const mat1 = new THREE.MeshBasicMaterial ( { color : "#00f2fe" , wireframe : true , transparent : true , opacity : 0.35 } );

const poly1 = new THREE.Mesh ( geom1 , mat1 );

poly1.position.set(4, 1, -5);

scene.add(poly1);

const geom2 = new THREE.TorusKnotGeometry ( 1.6 , 0.4 , 64 , 8 );

const mat2 = new THREE.MeshBasicMaterial ( { color : "#8a2be2" , wireframe : true , transparent : true , opacity : 0.25 } );

const poly2 = new THREE.Mesh ( geom2 , mat2 );

poly2.position.set(-4.5, -2, -6);

scene.add(poly2);

camera.position.z = 7;

let mouseX = 0;

let mouseY = 0;

let targetX = 0;

let targetY = 0;

window.addEventListener('mousemove', function(event) {

mouseX = ( event.clientX / window.innerWidth - 0.5 ) * 2;

mouseY = ( event.clientY / window.innerHeight - 0.5 ) * 2;

});

window.addEventListener('resize', function() {

camera.aspect = window.innerWidth / window.innerHeight;

camera.updateProjectionMatrix();

renderer.setSize(window.innerWidth, window.innerHeight);

});

;function render3DScene() {

requestAnimationFrame(render3DScene);

targetX += ( mouseX - targetX ) * 0.05;

targetY += ( mouseY - targetY ) * 0.05;

particlesMesh.rotation.y += 0.001;

particlesMesh.rotation.x += 0.0005;

poly1.rotation.x += 0.008;

poly1.rotation.y += 0.012;

poly1.position.x = 4 + targetX * 1.5;

poly1.position.y = 1 - targetY * 1.5;

poly2.rotation.x -= 0.006;

poly2.rotation.y += 0.009;

poly2.position.x = - 4.5 + targetX * 1.2;

poly2.position.y = - 2 - targetY * 1.2;

camera.position.x = targetX * 0.8;

camera.position.y = - targetY * 0.8;

camera.lookAt(scene.position);

renderer.render(scene, camera);

}

render3DScene();

}

init3D();

function showToast(message) {
  (document.getElementById("toast-alert") || document.querySelector("toast-alert")).textContent = message;
  (document.getElementById("toast-alert") || document.querySelector("toast-alert")).style.display = "block"
  setTimeout(function() {
    (document.getElementById("toast-alert") || document.querySelector("toast-alert")).style.display = "none"
  }, 3000);
}

(document.getElementById("btn-copy-phone") || document.querySelector("btn-copy-phone")).addEventListener('click', function(event) {
  copyToClipboard("8260350182");
  showToast("📞 Phone number copied: 8260350182");
});

(document.getElementById("btn-copy-email") || document.querySelector("btn-copy-email")).addEventListener('click', function(event) {
  copyToClipboard("prayaskumarsahoo45@gmail.com");
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

console.log("⚡ 3D Scene, Particle Grid, and Interactive Controls are 100% Active!");
    