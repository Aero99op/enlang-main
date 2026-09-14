console.log("Initializing Subway Surfers Sovereign Game Engine (.enlngs)...");

let score = 0;

let highScore = 0;

let coins = 0;

let runCoins = 0;

let keys = 2;

let baseMultiplier = 3;

let isAudioMuted = false;

let gameState = "start";

const LANES = [ - 2.8 , 0 , 2.8 ];

const CHUNK_LENGTH = 60;

const ACTIVE_CHUNKS = 8;

let laneIndex = 1;

let playerX = 0;

let targetX = 0;

let playerY = 0;

let playerZ = 0;

let playerVY = 0;

let baseSpeed = 26;

let speed = 26;

let maxSpeed = 52;

let gravity = - 54;

let jumpForce = 18;

let isGrounded = true;

let isJumping = false;

let isRolling = false;

let rollTimer = 0;

let invulnerableTimer = 0;

let lastTime = 0;

let scene = null;

let camera = null;

let renderer = null;

let canvas = null;

let jakeGroup = null;

let pelvisPivot = null;

let torsoPivot = null;

let headPivot = null;

let leftHipPivot = null;

let rightHipPivot = null;

let leftShoulderPivot = null;

let rightShoulderPivot = null;

let hoverboardMeshGroup = null;

let jakeRunCycle = 0;

let chunks = [ ];

let trains = [ ];

let obstacles = [ ];

let coinsList = [ ];

let powerupPickups = [ ];

let nextChunkZ = 30;

let ballastMat = null;

let tieMat = null;

let railMat = null;

let bldgMat1 = null;

let palmTrunkMat = null;

let palmFrondMat = null;

let trainBlueSideMat = null;

let trainBlueFrontMat = null;

let trainRedSideMat = null;

let trainRedFrontMat = null;

let trainRoofMat = null;

let chevronMat = null;

let coinMat = null;

let hoverboardActive = false;

let hoverboardTimer = 0;

let hoverboardDuration = 25;

let magnetActive = false;

let multiplierActive = false;

let audioCtx = null;

let rand = Math.random;

let floor = Math.floor;

let sin = Math.sin;

let cos = Math.cos;

let abs = Math.abs;

let min = Math.min;

let max = Math.max;

let hypot = Math.hypot;

let pi = Math.PI;

let RepeatWrapping = THREE.RepeatWrapping;

let DoubleSide = THREE.DoubleSide;

function getElement(elemId) {
  return ( document.getElementById ( elemId ) || document.querySelector ( elemId ) );
}

function getWindowWidth() {
  return window.innerWidth;
}

function getWindowHeight() {
  return window.innerHeight;
}

function getDevicePixelRatio() {
  return ( window.devicePixelRatio || 1 );
}

function getNow() {
  return performance.now ( );
}

function formatScore(num) {
  return String ( floor ( num ) ) .padStart ( 6 , "0" );
}

function saveHighScore(val) {
  localStorage.setItem ( "subway_high_score" , val );
}

function loadHighScore() {
  return ( localStorage.getItem ( "subway_high_score" ) || 0 );
}

function setBarWidth(elemId, pct) {
  getElement ( elemId ).style.width = pct + "%";
}

function createCanvas(w, h) {
  let c = document.createElement ( "canvas" );
  c.width = w;
  c.height = h;
  return c;
}

function get2DContext(c) {
  return c.getContext ( "2d" );
}

function createTexture(c) {
  return new THREE.CanvasTexture ( c );
}

function configureTextureRepeat(tex, rx, ry) {
  tex.wrapS = RepeatWrapping;
  tex.wrapT = RepeatWrapping;
  tex.repeat.set ( rx , ry );
}

function createLinearGrad(ctx, x0, y0, x1, y1) {
  return ctx.createLinearGradient ( x0 , y0 , x1 , y1 );
}

function addGradColor(grad, stop, col) {
  grad.addColorStop ( stop , col );
}

function fillCanvasRect(ctx, x, y, w, h) {
  ctx.fillRect ( x , y , w , h );
}

function drawRect(ctx, col, x, y, w, h) {
  ctx.fillStyle = col;
  ctx.fillRect ( x , y , w , h );
}

function strokeRect(ctx, col, lw, x, y, w, h) {
  ctx.strokeStyle = col;
  ctx.lineWidth = lw;
  ctx.strokeRect ( x , y , w , h );
}

function drawCircle(ctx, col, x, y, r) {
  ctx.fillStyle = col;
  ctx.beginPath ( );
  ctx.arc ( x , y , r , 0 , pi * 2 );
  ctx.fill ( );
}

function drawEllipse(ctx, col, x, y, rx, ry) {
  ctx.fillStyle = col;
  ctx.beginPath ( );
  ctx.ellipse ( x , y , rx , ry , 0 , 0 , pi * 2 );
  ctx.fill ( );
}

function drawLine(ctx, col, lw, x1, y1, x2, y2) {
  ctx.strokeStyle = col;
  ctx.lineWidth = lw;
  ctx.beginPath ( );
  ctx.moveTo ( x1 , y1 );
  ctx.lineTo ( x2 , y2 );
  ctx.stroke ( );
}

function drawText(ctx, txt, col, fontStr, alignStr, x, y) {
  ctx.fillStyle = col;
  ctx.font = fontStr;
  ctx.textAlign = alignStr;
  ctx.fillText ( txt , x , y );
}

function setPosition(obj, x, y, z) {
  obj.position.set ( x , y , z );
}

function setPosX(obj, x) {
  obj.position.x = x;
}

function setPosY(obj, y) {
  obj.position.y = y;
}

function setPosZ(obj, z) {
  obj.position.z = z;
}

function addPosX(obj, delta) {
  obj.position.x += delta;
}

function addPosY(obj, delta) {
  obj.position.y += delta;
}

function addPosZ(obj, delta) {
  obj.position.z += delta;
}

function setRotation(obj, rx, ry, rz) {
  obj.rotation.set ( rx , ry , rz );
}

function setRotX(obj, rx) {
  obj.rotation.x = rx;
}

function setRotY(obj, ry) {
  obj.rotation.y = ry;
}

function setRotZ(obj, rz) {
  obj.rotation.z = rz;
}

function addRotX(obj, delta) {
  obj.rotation.x += delta;
}

function addRotY(obj, delta) {
  obj.rotation.y += delta;
}

function addRotZ(obj, delta) {
  obj.rotation.z += delta;
}

function setScale(obj, sx, sy, sz) {
  obj.scale.set ( sx , sy , sz );
}

function setVisible(obj, isVis) {
  obj.visible = isVis;
}

function setShadows(mesh, cast, recv) {
  mesh.castShadow = cast;
  mesh.receiveShadow = recv;
}

function addToScene(obj) {
  scene.add ( obj );
}

function removeFromScene(obj) {
  scene.remove ( obj );
}

function addToGroup(grp, child) {
  grp.add ( child );
}

function createGroup() {
  return new THREE.Group ( );
}

function createScene() {
  return new THREE.Scene ( );
}

function setSceneBackground(scn, colHex) {
  scn.background = new THREE.Color ( colHex );
}

function setSceneFog(scn, colHex, near, far) {
  scn.fog = new THREE.Fog ( colHex , near , far );
}

function createPerspectiveCamera(fov, aspect, near, far) {
  return new THREE.PerspectiveCamera ( fov , aspect , near , far );
}

function updateCameraProjection(cam, aspect) {
  cam.aspect = aspect;
  cam.updateProjectionMatrix ( );
}

function createWebGLRenderer(c) {
  return new THREE.WebGLRenderer ( { "canvas" : c , "antialias" : true , "powerPreference" : "high-performance" } );
}

function configureRenderer(r, w, h, dpr) {
  r.setSize ( w , h );
  r.setPixelRatio ( dpr );
}

function createHemisphereLight(skyCol, groundCol, intensity) {
  return new THREE.HemisphereLight ( skyCol , groundCol , intensity );
}

function createDirectionalLight(col, intensity) {
  return new THREE.DirectionalLight ( col , intensity );
}

function createColor(hexStr) {
  return new THREE.Color ( hexStr );
}

function createBoxMesh(w, h, d, mat) {
  let geo = new THREE.BoxGeometry ( w , h , d );
  return new THREE.Mesh ( geo , mat );
}

function createCylinderMesh(rt, rb, h, seg, mat) {
  let geo = new THREE.CylinderGeometry ( rt , rb , h , seg );
  return new THREE.Mesh ( geo , mat );
}

function createSphereMesh(r, seg, mat) {
  let geo = new THREE.SphereGeometry ( r , seg , seg );
  return new THREE.Mesh ( geo , mat );
}

function createPlaneMesh(w, h, mat) {
  let geo = new THREE.PlaneGeometry ( w , h );
  return new THREE.Mesh ( geo , mat );
}

function createBasicMat(col) {
  return new THREE.MeshBasicMaterial ( { "color" : col } );
}

function createStandardMat(col, rough, metal) {
  return new THREE.MeshStandardMaterial ( { "color" : col , "roughness" : rough , "metalness" : metal } );
}

function createTexMat(tex, rough) {
  return new THREE.MeshStandardMaterial ( { "map" : tex , "roughness" : rough } );
}

function lookCameraAt(x, y, z) {
  camera.lookAt ( x , y , z );
}

function renderUniverse() {
  renderer.render ( scene , camera );
}

function createBallastTexture() {
  let c = createCanvas ( 256 , 256 );
  let ctx = get2DContext ( c );
  drawRect(ctx , "#1e293b" , 0 , 0 , 256 , 256);
  for (let _i1 = 0; _i1 < 1200; _i1++) {
    let x = rand ( ) * 256;
    let y = rand ( ) * 256;
    let r = 1 + rand ( ) * 2.5;
    let shade = floor ( 50 + rand ( ) * 80 );
    let colStr = "rgb(" + shade + "," + ( shade + 4 ) + "," + ( shade + 8 ) + ")";
    drawCircle(ctx , colStr , x , y , r);
  }
  let tex = createTexture ( c );
  configureTextureRepeat(tex , 1 , 12);
  return tex;
}

function createTieTexture() {
  let c = createCanvas ( 128 , 64 );
  let ctx = get2DContext ( c );
  drawRect(ctx , "#3e2723" , 0 , 0 , 128 , 64);
  for (let _i2 = 0; _i2 < 40; _i2++) {
    drawRect(ctx , "#4e342e" , 0 , 0 , 128 , 1);
  }
  drawRect(ctx , "#78909c" , 18 , 16 , 8 , 32);
  drawRect(ctx , "#78909c" , 102 , 16 , 8 , 32);
  return createTexture ( c );
}

function createRailTexture() {
  let c = createCanvas ( 64 , 128 );
  let ctx = get2DContext ( c );
  let grad = createLinearGrad ( ctx , 0 , 0 , 64 , 0 );
  addGradColor(grad , 0 , "#475569");
  addGradColor(grad , 0.3 , "#cbd5e1");
  addGradColor(grad , 0.5 , "#ffffff");
  addGradColor(grad , 0.7 , "#cbd5e1");
  addGradColor(grad , 1 , "#334155");
  ctx.fillStyle = grad;
  fillCanvasRect(ctx , 0 , 0 , 64 , 128);
  let tex = createTexture ( c );
  configureTextureRepeat(tex , 1 , 24);
  return tex;
}

function createBuildingTexture(baseCol, trimCol) {
  let c = createCanvas ( 256 , 512 );
  let ctx = get2DContext ( c );
  drawRect(ctx , baseCol , 0 , 0 , 256 , 512);
  for (const rowIdx of [ 0 , 1 , 2 , 3 , 4 , 5 ]) {
    let rowY = 30 + rowIdx * 75;
    for (const colIdx of [ 0 , 1 , 2 , 3 ]) {
      let colX = 20 + colIdx * 55;
      drawRect(ctx , trimCol , colX - 3 , rowY - 3 , 38 , 48);
      let winGrad = createLinearGrad ( ctx , colX , rowY , colX , rowY + 42 );
      addGradColor(winGrad , 0 , "#0284c7");
      addGradColor(winGrad , 1 , "#0369a1");
      ctx.fillStyle = winGrad;
      fillCanvasRect(ctx , colX , rowY , 32 , 42);
      drawRect(ctx , "rgba(255, 255, 255, 0.4)" , colX + 2 , rowY + 2 , 28 , 4);
      drawRect(ctx , "#ffffff" , colX + 15 , rowY , 2 , 42);
      drawRect(ctx , "#ffffff" , colX , rowY + 20 , 32 , 2);
    }
  }
  let tex = createTexture ( c );
  configureTextureRepeat(tex , 1 , 2);
  return tex;
}

function createPalmTrunkTexture() {
  let c = createCanvas ( 64 , 128 );
  let ctx = get2DContext ( c );
  drawRect(ctx , "#78350f" , 0 , 0 , 64 , 128);
  for (const trunkStep of [ 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10 ]) {
    drawRect(ctx , "#451a03" , 0 , trunkStep * 12 , 64 , 3);
  }
  return createTexture ( c );
}

function createPalmFrondTexture() {
  let c = createCanvas ( 128 , 256 );
  let ctx = get2DContext ( c );
  drawEllipse(ctx , "#15803d" , 64 , 128 , 50 , 120);
  drawRect(ctx , "#4ade80" , 62 , 0 , 4 , 256);
  return createTexture ( c );
}

function createTrainSideTexture(isRed) {
  let c = createCanvas ( 512 , 256 );
  let ctx = get2DContext ( c );
  drawRect(ctx , isRed ? "#dc2626" : "#2563eb" , 0 , 0 , 512 , 256);
  drawRect(ctx , "#facc15" , 0 , 200 , 512 , 24);
  drawRect(ctx , "#ffffff" , 0 , 224 , 512 , 8);
  for (const winStep of [ 0 , 1 , 2 , 3 , 4 ]) {
    let x = 40 + winStep * 90;
    drawRect(ctx , "#0f172a" , x - 2 , 48 , 64 , 94);
    let wGrad = createLinearGrad ( ctx , x , 50 , x , 140 );
    addGradColor(wGrad , 0 , "#38bdf8");
    addGradColor(wGrad , 1 , "#0284c7");
    ctx.fillStyle = wGrad;
    fillCanvasRect(ctx , x , 50 , 60 , 90);
  }
  return createTexture ( c );
}

function createTrainFrontTexture(isRed) {
  let c = createCanvas ( 256 , 256 );
  let ctx = get2DContext ( c );
  drawRect(ctx , isRed ? "#b91c1c" : "#1d4ed8" , 0 , 0 , 256 , 256);
  drawRect(ctx , "#0f172a" , 28 , 38 , 200 , 94);
  let winGrad = createLinearGrad ( ctx , 30 , 40 , 30 , 130 );
  addGradColor(winGrad , 0 , "#38bdf8");
  addGradColor(winGrad , 1 , "#0284c7");
  ctx.fillStyle = winGrad;
  fillCanvasRect(ctx , 30 , 40 , 196 , 90);
  for (const stripeStep of [ 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 ]) {
    let stripeCol = ( stripeStep % 2 == 0 ) ? "#facc15" : "#1e293b";
    ctx.fillStyle = stripeCol;
    let stripeX = stripeStep * 32;
    drawLine(ctx , stripeCol , 16 , stripeX + 12 , 200 , stripeX - 4 , 256);
  }
  return createTexture ( c );
}

function createChevronTexture() {
  let c = createCanvas ( 256 , 64 );
  let ctx = get2DContext ( c );
  drawRect(ctx , "#facc15" , 0 , 0 , 256 , 64);
  for (const chevronStep of [ 0 , 1 , 2 , 3 , 4 , 5 , 6 ]) {
    let x = - 64 + chevronStep * 48;
    drawLine(ctx , "#0f172a" , 24 , x + 12 , 0 , x + 42 , 64);
  }
  return createTexture ( c );
}

function initAudio() {
  if (audioCtx == null) {
    let AudioClass = window [ "AudioContext" ] || window [ "webkitAudioContext" ];
    if (AudioClass != null) {
      audioCtx = new AudioClass ( );
    }
  }
  if (audioCtx != null && audioCtx [ "state" ] == "suspended") {
    audioCtx.resume ( );
  }
}

function playSynthTone(waveType, startF, endF, vol, dur) {
  if (isAudioMuted == true || audioCtx == null) {
    return;
  }
  let now = audioCtx [ "currentTime" ];
  let osc = audioCtx.createOscillator ( );
  let gain = audioCtx.createGain ( );
  osc.type = waveType;
  osc.frequency.setValueAtTime ( startF , now );
  osc.frequency.exponentialRampToValueAtTime ( endF , now + dur );
  gain.gain.setValueAtTime ( vol , now );
  gain.gain.linearRampToValueAtTime ( 0.01 , now + dur );
  osc.connect ( gain );
  gain.connect ( audioCtx [ "destination" ] );
  osc.start ( now );
  osc.stop ( now + dur );
}

function playSoundJump() {
  playSynthTone("triangle" , 180 , 540 , 0.3 , 0.18);
}

function playSoundSlide() {
  playSynthTone("sawtooth" , 320 , 120 , 0.2 , 0.22);
}

function playSoundCoin() {
  if (isAudioMuted == true || audioCtx == null) {
    return;
  }
  let now = audioCtx [ "currentTime" ];
  let osc1 = audioCtx.createOscillator ( );
  let osc2 = audioCtx.createOscillator ( );
  let gain = audioCtx.createGain ( );
  osc1.type = "sine";
  osc2.type = "sine";
  osc1.frequency.setValueAtTime ( 988 , now );
  osc2.frequency.setValueAtTime ( 1318 , now + 0.05 );
  gain.gain.setValueAtTime ( 0.25 , now );
  gain.gain.linearRampToValueAtTime ( 0.01 , now + 0.18 );
  osc1.connect ( gain );
  osc2.connect ( gain );
  gain.connect ( audioCtx [ "destination" ] );
  osc1.start ( now );
  osc1.stop ( now + 0.1 );
  osc2.start ( now + 0.05 );
  osc2.stop ( now + 0.18 );
}

function playSoundHoverboard() {
  playSynthTone("sine" , 300 , 900 , 0.3 , 0.25);
}

function playSoundCrash() {
  playSynthTone("sawtooth" , 140 , 40 , 0.5 , 0.35);
}

function buildJakeCharacter() {
  jakeGroup = createGroup ( );
  let skinMat = createStandardMat ( "#ffdbac" , 0.55 , 0 );
  let capRedMat = createStandardMat ( "#d32f2f" , 0.45 , 0 );
  let capWhiteMat = createStandardMat ( "#f8fafc" , 0.5 , 0 );
  let hoodieMat = createStandardMat ( "#f1f1ea" , 0.7 , 0 );
  let vestMat = createStandardMat ( "#1d4ed8" , 0.6 , 0 );
  let jeansMat = createStandardMat ( "#2563eb" , 0.65 , 0 );
  let shoeWhiteMat = createStandardMat ( "#ffffff" , 0.35 , 0 );
  let shoeGreenMat = createStandardMat ( "#22c55e" , 0.4 , 0 );
  let metalMat = createStandardMat ( "#d1d5db" , 0.2 , 0.9 );
  pelvisPivot = createGroup ( );
  setPosY(pelvisPivot , 0.82);
  addToGroup(jakeGroup , pelvisPivot);
  let pelvisMesh = createBoxMesh ( 0.38 , 0.22 , 0.26 , jeansMat );
  addToGroup(pelvisPivot , pelvisMesh);
  torsoPivot = createGroup ( );
  setPosY(torsoPivot , 0.16);
  addToGroup(pelvisPivot , torsoPivot);
  let hoodieMesh = createBoxMesh ( 0.44 , 0.46 , 0.28 , hoodieMat );
  setPosY(hoodieMesh , 0.22);
  addToGroup(torsoPivot , hoodieMesh);
  let vestMesh = createBoxMesh ( 0.46 , 0.42 , 0.30 , vestMat );
  setPosY(vestMesh , 0.22);
  addToGroup(torsoPivot , vestMesh);
  let eCanvas = createCanvas ( 128 , 64 );
  let ectx = get2DContext ( eCanvas );
  drawRect(ectx , "#0f172a" , 0 , 0 , 128 , 64);
  drawText(ectx , "SUB" , "#facc15" , "900 24stripeX sans-serif" , "center" , 64 , 28);
  drawText(ectx , "SURF" , "#00f0ff" , "900 24stripeX sans-serif" , "center" , 64 , 54);
  let eTex = createTexture ( eCanvas );
  let eMesh = createPlaneMesh ( 0.24 , 0.16 , createBasicMat ( null ) );
  eMesh [ "material" ].map = eTex;
  setPosition(eMesh , 0 , 0.24 , - 0.155);
  setRotY(eMesh , pi);
  addToGroup(torsoPivot , eMesh);
  headPivot = createGroup ( );
  setPosY(headPivot , 0.52);
  addToGroup(torsoPivot , headPivot);
  let headMesh = createBoxMesh ( 0.28 , 0.30 , 0.28 , skinMat );
  setPosY(headMesh , 0.12);
  addToGroup(headPivot , headMesh);
  let glassMesh = createBoxMesh ( 0.26 , 0.08 , 0.06 , createStandardMat ( "#090d16" , 0.1 , 0.8 ) );
  setPosition(glassMesh , 0 , 0.14 , 0.14);
  addToGroup(headPivot , glassMesh);
  let capCrown = createSphereMesh ( 0.18 , 16 , capRedMat );
  setPosition(capCrown , 0 , 0.20 , 0);
  addToGroup(headPivot , capCrown);
  let capBrim = createBoxMesh ( 0.24 , 0.025 , 0.18 , capWhiteMat );
  setPosition(capBrim , 0 , 0.22 , - 0.16);
  setRotX(capBrim , - 0.18);
  addToGroup(headPivot , capBrim);
  leftShoulderPivot = createGroup ( );
  setPosition(leftShoulderPivot , - 0.28 , 0.38 , 0);
  addToGroup(torsoPivot , leftShoulderPivot);
  let lArm = createBoxMesh ( 0.11 , 0.36 , 0.11 , hoodieMat );
  setPosY(lArm , - 0.16);
  addToGroup(leftShoulderPivot , lArm);
  let lHand = createBoxMesh ( 0.09 , 0.10 , 0.09 , skinMat );
  setPosY(lHand , - 0.36);
  addToGroup(leftShoulderPivot , lHand);
  rightShoulderPivot = createGroup ( );
  setPosition(rightShoulderPivot , 0.28 , 0.38 , 0);
  addToGroup(torsoPivot , rightShoulderPivot);
  let rArm = createBoxMesh ( 0.11 , 0.36 , 0.11 , hoodieMat );
  setPosY(rArm , - 0.16);
  addToGroup(rightShoulderPivot , rArm);
  let rHand = createBoxMesh ( 0.09 , 0.10 , 0.09 , skinMat );
  setPosY(rHand , - 0.36);
  addToGroup(rightShoulderPivot , rHand);
  let canBody = createCylinderMesh ( 0.045 , 0.045 , 0.14 , 12 , metalMat );
  setPosition(canBody , 0.02 , - 0.38 , 0.08);
  addToGroup(rightShoulderPivot , canBody);
  leftHipPivot = createGroup ( );
  setPosition(leftHipPivot , - 0.13 , - 0.08 , 0);
  addToGroup(pelvisPivot , leftHipPivot);
  let lLeg = createBoxMesh ( 0.14 , 0.44 , 0.15 , jeansMat );
  setPosY(lLeg , - 0.20);
  addToGroup(leftHipPivot , lLeg);
  let lShoe = createBoxMesh ( 0.15 , 0.06 , 0.28 , shoeWhiteMat );
  setPosition(lShoe , 0 , - 0.48 , 0.03);
  addToGroup(leftHipPivot , lShoe);
  let lToe = createBoxMesh ( 0.14 , 0.08 , 0.14 , shoeGreenMat );
  setPosition(lToe , 0 , - 0.43 , 0.09);
  addToGroup(leftHipPivot , lToe);
  rightHipPivot = createGroup ( );
  setPosition(rightHipPivot , 0.13 , - 0.08 , 0);
  addToGroup(pelvisPivot , rightHipPivot);
  let rLeg = createBoxMesh ( 0.14 , 0.44 , 0.15 , jeansMat );
  setPosY(rLeg , - 0.20);
  addToGroup(rightHipPivot , rLeg);
  let rShoe = createBoxMesh ( 0.15 , 0.06 , 0.28 , shoeWhiteMat );
  setPosition(rShoe , 0 , - 0.48 , 0.03);
  addToGroup(rightHipPivot , rShoe);
  let rToe = createBoxMesh ( 0.14 , 0.08 , 0.14 , shoeGreenMat );
  setPosition(rToe , 0 , - 0.43 , 0.09);
  addToGroup(rightHipPivot , rToe);
  hoverboardMeshGroup = createGroup ( );
  setPosition(hoverboardMeshGroup , 0 , 0.08 , 0);
  setVisible(hoverboardMeshGroup , false);
  let deckMesh = createBoxMesh ( 0.65 , 0.05 , 1.4 , createStandardMat ( "#ef4444" , 0.3 , 0.4 ) );
  addToGroup(hoverboardMeshGroup , deckMesh);
  let neonMesh = createBoxMesh ( 0.48 , 0.06 , 1.3 , createStandardMat ( "#00f0ff" , 0.2 , 0 ) );
  addToGroup(hoverboardMeshGroup , neonMesh);
  addToGroup(jakeGroup , hoverboardMeshGroup);
  addToScene(jakeGroup);
}

function updateJakeAnimation(dt) {
  let bankLean = ( targetX - playerX );
  setRotZ(jakeGroup , bankLean * 0.14);
  if (isRolling == true) {
    setRotX(torsoPivot , 0.8);
    setRotX(headPivot , - 0.4);
    setRotX(leftHipPivot , - 1.2);
    setRotX(rightHipPivot , - 1.2);
    setPosY(pelvisPivot , 0.45);
    return;
  }
  else {
    setPosY(pelvisPivot , 0.82);
    setRotX(torsoPivot , 0);
    setRotX(headPivot , 0);
  }
  if (hoverboardActive == true) {
    setRotX(leftHipPivot , 0.15);
    setRotX(rightHipPivot , - 0.15);
    setRotX(leftShoulderPivot , - 0.2);
    setRotX(rightShoulderPivot , 0.2);
    setRotZ(hoverboardMeshGroup , sin ( getNow ( ) * 0.01 ) * 0.08);
    return;
  }
  if (isGrounded == false || isJumping == true) {
    setRotX(leftHipPivot , - 0.7);
    setRotX(rightHipPivot , 0.3);
    setRotX(leftShoulderPivot , 1.1);
    setRotX(rightShoulderPivot , - 0.6);
    return;
  }
  jakeRunCycle += dt * ( speed * 0.72 );
  let sVal = sin ( jakeRunCycle );
  let cVal = cos ( jakeRunCycle );
  setRotX(leftHipPivot , sVal * 0.85);
  setRotX(rightHipPivot , - sVal * 0.85);
  setRotX(leftShoulderPivot , - sVal * 0.75);
  setRotX(rightShoulderPivot , sVal * 0.75);
  setPosY(pelvisPivot , 0.82 + abs ( cVal ) * 0.08);
  setRotY(headPivot , sVal * 0.08);
}

function spawnTrackChunk(chunkZ) {
  let chunkGroup = createGroup ( );
  setPosZ(chunkGroup , chunkZ);
  let ballastMesh = createBoxMesh ( 14 , 0.4 , CHUNK_LENGTH , ballastMat );
  setPosition(ballastMesh , 0 , - 0.2 , 0);
  addToGroup(chunkGroup , ballastMesh);
  for (const laneIdx of [ 0 , 1 , 2 ]) {
    let laneX = LANES [ laneIdx ];
    let railL = createBoxMesh ( 0.08 , 0.14 , CHUNK_LENGTH , railMat );
    setPosition(railL , laneX - 0.72 , 0.07 , 0);
    let railR = createBoxMesh ( 0.08 , 0.14 , CHUNK_LENGTH , railMat );
    setPosition(railR , laneX + 0.72 , 0.07 , 0);
    addToGroup(chunkGroup , railL);
    addToGroup(chunkGroup , railR);
    for (const tieStep of [ 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10 , 11 , 12 , 13 , 14 , 15 , 16 , 17 , 18 , 19 , 20 , 21 , 22 , 23 ]) {
      let tieMesh = createBoxMesh ( 1.9 , 0.08 , 0.44 , tieMat );
      setPosition(tieMesh , laneX , 0.04 , - CHUNK_LENGTH / 2 + 1.2 + tieStep * 2.4);
      addToGroup(chunkGroup , tieMesh);
    }
  }
  for (const sideIdx of [ 0 , 1 ]) {
    let side = ( sideIdx == 0 ) ? - 1 : 1;
    for (const bldgStep of [ 0 , 1 ]) {
      let bldgZ = - CHUNK_LENGTH / 2 + 15 + bldgStep * 30;
      let bMesh = createBoxMesh ( 10 , 26 , 28 , bldgMat1 );
      setPosition(bMesh , side * 13 , 13 , bldgZ);
      addToGroup(chunkGroup , bMesh);
      let palmMesh = createCylinderMesh ( 0.22 , 0.28 , 8.5 , 8 , palmTrunkMat );
      setPosition(palmMesh , side * 7.5 , 4.2 , bldgZ + 6);
      addToGroup(chunkGroup , palmMesh);
    }
  }
  addToScene(chunkGroup);
  chunks.push({ "group" : chunkGroup , "z" : chunkZ });
  if (chunkZ < - 20) {
    populateEntities(chunkZ);
  }
}

function populateEntities(chunkZ) {
  if (rand ( ) < 0.65) {
    let lane = floor ( rand ( ) * 3 );
    let isRed = ( rand ( ) < 0.45 );
    let trainGroup = createGroup ( );
    let trainX = LANES [ lane ];
    let trainZ = chunkZ + ( rand ( ) * 20 - 10 );
    let bodyMat = isRed ? trainRedSideMat : trainBlueSideMat;
    let bodyMesh = createBoxMesh ( 2.3 , 3.4 , 18 , bodyMat );
    setPosY(bodyMesh , 1.7);
    addToGroup(trainGroup , bodyMesh);
    let frontMat = isRed ? trainRedFrontMat : trainBlueFrontMat;
    let frontMesh = createPlaneMesh ( 2.3 , 3.4 , frontMat );
    setPosition(frontMesh , 0 , 1.7 , 9.01);
    addToGroup(trainGroup , frontMesh);
    let roofMesh = createBoxMesh ( 2.32 , 0.2 , 18.02 , trainRoofMat );
    setPosY(roofMesh , 3.5);
    addToGroup(trainGroup , roofMesh);
    setPosition(trainGroup , trainX , 0 , trainZ);
    addToScene(trainGroup);
    trains.push({ "group" : trainGroup , "lane" : lane , "isMoving" : isRed , "speed" : isRed ? 14 : 0 , "x" : trainX , "y" : 0 , "z" : trainZ , "width" : 2.3 , "height" : 3.6 , "length" : 18 });
    for (const coinStep of [ 0 , 1 , 2 , 3 ]) {
      spawnCoin(trainX , 4.2 , trainZ - 6 + coinStep * 4);
    }
  }
  for (const obsLane of [ 0 , 1 , 2 ]) {
    if (rand ( ) < 0.4) {
      let obsX = LANES [ obsLane ];
      let obsZ = chunkZ + ( rand ( ) * 30 - 15 );
      let hurdleGroup = createGroup ( );
      let hBoard = createBoxMesh ( 2.2 , 0.45 , 0.12 , chevronMat );
      setPosY(hBoard , 0.75);
      addToGroup(hurdleGroup , hBoard);
      setPosition(hurdleGroup , obsX , 0 , obsZ);
      addToScene(hurdleGroup);
      obstacles.push({ "group" : hurdleGroup , "type" : "low_barrier" , "x" : obsX , "y" : 0 , "z" : obsZ , "width" : 2.2 , "height" : 1.0 , "length" : 0.4 });
    }
    else if (rand ( ) < 0.5) {
      let coinX = LANES [ obsLane ];
      for (const coinStep of [ 0 , 1 , 2 , 3 ]) {
        spawnCoin(coinX , 0.95 , chunkZ - 8 + coinStep * 4);
      }
      if (rand ( ) < 0.15) {
        spawnPowerup(coinX , 1.2 , chunkZ , "hoverboard");
      }
    }
  }
}

function spawnCoin(x, y, z) {
  let coinMesh = createCylinderMesh ( 0.38 , 0.38 , 0.08 , 16 , coinMat );
  setRotX(coinMesh , pi * 0.5);
  setPosition(coinMesh , x , y , z);
  addToScene(coinMesh);
  coinsList.push({ "mesh" : coinMesh , "x" : x , "y" : y , "z" : z , "collected" : false });
}

function spawnPowerup(x, y, z, pType) {
  let pGroup = createGroup ( );
  setPosition(pGroup , x , y , z);
  let pMesh = createBoxMesh ( 0.65 , 0.65 , 0.65 , createStandardMat ( "#ef4444" , 0.2 , 0 ) );
  addToGroup(pGroup , pMesh);
  addToScene(pGroup);
  powerupPickups.push({ "group" : pGroup , "type" : pType , "x" : x , "y" : y , "z" : z , "collected" : false });
}

function updateWorld(dt, curPlayerZ) {
  for (const chunk of chunks) {
    if (chunk [ "z" ] > curPlayerZ + CHUNK_LENGTH * 1.5) {
      setPosZ(chunk [ "group" ] , nextChunkZ);
      chunk.z = nextChunkZ;
      nextChunkZ -= CHUNK_LENGTH;
      populateEntities(chunk [ "z" ]);
    }
  }
  for (const t of trains) {
    if (t [ "isMoving" ] == true) {
      t.z += t [ "speed" ] * dt;
      setPosZ(t [ "group" ] , t [ "z" ]);
    }
  }
  for (const c of coinsList) {
    if (c [ "collected" ] == false) {
      addRotZ(c [ "mesh" ] , dt * 3.5);
    }
  }
  for (const p of powerupPickups) {
    if (p [ "collected" ] == false) {
      addRotY(p [ "group" ] , dt * 3.0);
    }
  }
}

function checkCollisions() {
  if (invulnerableTimer > 0) {
    return;
  }
  let pMinX = playerX - 0.45;
  let pMaxX = playerX + 0.45;
  let pMinY = playerY;
  let pMaxY = playerY + ( isRolling ? 0.75 : 1.7 );
  let pMinZ = playerZ - 0.45;
  let pMaxZ = playerZ + 0.45;
  for (const t of trains) {
    let tMinX = t [ "x" ] - t [ "width" ] / 2;
    let tMaxX = t [ "x" ] + t [ "width" ] / 2;
    let tMinY = t [ "y" ];
    let tMaxY = t [ "y" ] + t [ "height" ];
    let tMinZ = t [ "z" ] - t [ "length" ] / 2;
    let tMaxZ = t [ "z" ] + t [ "length" ] / 2;
    if (pMaxX > tMinX && pMinX < tMaxX && pMaxZ > tMinZ && pMinZ < tMaxZ) {
      if (playerY >= tMaxY - 0.5 && playerVY <= 0) {
        playerY = tMaxY;
        playerVY = 0;
        isGrounded = true;
        isJumping = false;
        return;
      }
      handleHit();
      return;
    }
  }
  for (const obs of obstacles) {
    let oMinX = obs [ "x" ] - obs [ "width" ] / 2;
    let oMaxX = obs [ "x" ] + obs [ "width" ] / 2;
    let oMinZ = obs [ "z" ] - obs [ "length" ] / 2;
    let oMaxZ = obs [ "z" ] + obs [ "length" ] / 2;
    if (pMaxX > oMinX && pMinX < oMaxX && pMaxZ > oMinZ && pMinZ < oMaxZ) {
      if (obs [ "type" ] == "low_barrier" && playerY > 0.8) {
        return;
      }
      handleHit();
      return;
    }
  }
  for (const c of coinsList) {
    if (c [ "collected" ] == false) {
      let dist = hypot ( playerX - c [ "x" ] , playerZ - c [ "z" ] );
      if (magnetActive == true && dist < 18) {
        c.x += ( playerX - c [ "x" ] ) * 0.16;
        c.y += ( playerY + 0.8 - c [ "y" ] ) * 0.16;
        c.z += ( playerZ - c [ "z" ] ) * 0.16;
        setPosition(c [ "mesh" ] , c [ "x" ] , c [ "y" ] , c [ "z" ]);
      }
      if (dist < 1.3) {
        c.collected = true;
        removeFromScene(c [ "mesh" ]);
        runCoins += 1;
        playSoundCoin();
      }
    }
  }
  for (const p of powerupPickups) {
    if (p [ "collected" ] == false) {
      let pDist = hypot ( playerX - p [ "x" ] , playerZ - p [ "z" ] );
      if (pDist < 1.4) {
        p.collected = true;
        removeFromScene(p [ "group" ]);
        activateHoverboard();
      }
    }
  }
}

function handleHit() {
  if (hoverboardActive == true) {
    hoverboardActive = false;
    setVisible(hoverboardMeshGroup , false);
    (document.getElementById("hoverboard-widget") || document.querySelector("hoverboard-widget")).style.display = 'none';
    invulnerableTimer = 1.8;
    playSoundCrash();
    return;
  }
  playSoundCrash();
  gameState = "gameover";
  if (score > highScore) {
    highScore = floor ( score );
    saveHighScore(highScore);
  }
  (document.getElementById("final-score") || document.querySelector("final-score")).textContent = formatScore ( score );
  (document.getElementById("final-highscore") || document.querySelector("final-highscore")).textContent = formatScore ( highScore );
  (document.getElementById("final-coins") || document.querySelector("final-coins")).textContent = String ( coins + runCoins );
  (document.getElementById("gameover-overlay") || document.querySelector("gameover-overlay")).classList.remove('hidden');
}

function activateHoverboard() {
  if (gameState != "playing") {
    return;
  }
  if (hoverboardActive == false) {
    hoverboardActive = true;
    hoverboardTimer = hoverboardDuration;
    setVisible(hoverboardMeshGroup , true);
    (document.getElementById("hoverboard-widget") || document.querySelector("hoverboard-widget")).style.display = 'block';
    playSoundHoverboard();
  }
}

function updateHUD() {
  (document.getElementById("score-text") || document.querySelector("score-text")).textContent = formatScore ( score );
  (document.getElementById("multiplier-text") || document.querySelector("multiplier-text")).textContent = "x" + ( multiplierActive ? baseMultiplier * 2 : baseMultiplier );
  (document.getElementById("coin-text") || document.querySelector("coin-text")).textContent = String ( coins + runCoins );
  (document.getElementById("key-text") || document.querySelector("key-text")).textContent = String ( keys );
  if (hoverboardActive == true) {
    let pct = ( hoverboardTimer / hoverboardDuration ) * 100;
    setBarWidth("hoverboard-bar-fill" , pct);
  }
}

function moveLeft() {
  if (gameState != "playing") {
    return;
  }
  if (laneIndex > 0) {
    laneIndex -= 1;
    targetX = LANES [ laneIndex ];
    playSoundSlide();
  }
}

function moveRight() {
  if (gameState != "playing") {
    return;
  }
  if (laneIndex < 2) {
    laneIndex += 1;
    targetX = LANES [ laneIndex ];
    playSoundSlide();
  }
}

function jump() {
  if (gameState != "playing") {
    return;
  }
  if (isGrounded == true) {
    playerVY = jumpForce;
    isGrounded = false;
    isJumping = true;
    playSoundJump();
  }
}

function roll() {
  if (gameState != "playing") {
    return;
  }
  isRolling = true;
  rollTimer = 0.75;
  if (isGrounded == false) {
    playerVY = - 28;
  }
  playSoundSlide();
}

function startGame() {
  initAudio();
  gameState = "playing";
  (document.getElementById("start-overlay") || document.querySelector("start-overlay")).classList.add('hidden');
}

function togglePause() {
  if (gameState == "playing") {
    gameState = "paused";
    (document.getElementById("pause-overlay") || document.querySelector("pause-overlay")).classList.remove('hidden');
  }
  else if (gameState == "paused") {
    gameState = "playing";
    (document.getElementById("pause-overlay") || document.querySelector("pause-overlay")).classList.add('hidden');
  }
}

function restartGame() {
  gameState = "playing";
  score = 0;
  runCoins = 0;
  speed = baseSpeed;
  laneIndex = 1;
  targetX = LANES [ 1 ];
  playerX = 0;
  playerY = 0;
  playerZ = 0;
  playerVY = 0;
  isRolling = false;
  rollTimer = 0;
  invulnerableTimer = 0;
  hoverboardActive = false;
  setVisible(hoverboardMeshGroup , false);
  (document.getElementById("pause-overlay") || document.querySelector("pause-overlay")).classList.add('hidden');
  (document.getElementById("gameover-overlay") || document.querySelector("gameover-overlay")).classList.add('hidden');
}

function initEngine() {
  canvas = getElement ( "webgl-canvas" );
  if (canvas == null) {
    return;
  }
  scene = createScene ( );
  setSceneBackground(scene , "#38bdf8");
  setSceneFog(scene , "#7dd3fc" , 90 , 240);
  let aspect = getWindowWidth ( ) / getWindowHeight ( );
  camera = createPerspectiveCamera ( 54 , aspect , 0.1 , 400 );
  setPosition(camera , 0 , 2.1 , 3.8);
  renderer = createWebGLRenderer ( canvas );
  configureRenderer(renderer , getWindowWidth ( ) , getWindowHeight ( ) , min ( getDevicePixelRatio ( ) , 2 ));
  let hemiLight = createHemisphereLight ( "#ffffff" , "#38bdf8" , 0.75 );
  addToScene(hemiLight);
  let dirLight = createDirectionalLight ( "#fffaed" , 0.95 );
  setPosition(dirLight , 30 , 60 , 40);
  addToScene(dirLight);
  ballastMat = createTexMat ( createBallastTexture ( ) , 0.9 );
  tieMat = createTexMat ( createTieTexture ( ) , 0.75 );
  railMat = createTexMat ( createRailTexture ( ) , 0.25 );
  railMat.metalness = 0.9;
  bldgMat1 = createTexMat ( createBuildingTexture ( "#e06d29" , "#ffca28" ) , 0.65 );
  palmTrunkMat = createTexMat ( createPalmTrunkTexture ( ) , 0.85 );
  palmFrondMat = createTexMat ( createPalmFrondTexture ( ) , 0.6 );
  palmFrondMat.side = DoubleSide;
  trainBlueSideMat = createTexMat ( createTrainSideTexture ( false ) , 0.35 );
  trainBlueFrontMat = createTexMat ( createTrainFrontTexture ( false ) , 0.35 );
  trainRedSideMat = createTexMat ( createTrainSideTexture ( true ) , 0.35 );
  trainRedFrontMat = createTexMat ( createTrainFrontTexture ( true ) , 0.35 );
  trainRoofMat = createStandardMat ( "#94a3b8" , 0.35 , 0.6 );
  chevronMat = createTexMat ( createChevronTexture ( ) , 0.5 );
  coinMat = createStandardMat ( "#facc15" , 0.15 , 0.85 );
  coinMat.emissive = createColor ( "#b45309" );
  buildJakeCharacter();
  for (let _i3 = 0; _i3 < ACTIVE_CHUNKS; _i3++) {
    spawnTrackChunk(nextChunkZ);
    nextChunkZ -= CHUNK_LENGTH;
  }
  lastTime = getNow ( );
  requestAnimationFrame ( gameLoop );
  console.log("Subway Surfers Sovereign Engine Running at 60 FPS!");
}

function gameLoop(currentTime) {
  requestAnimationFrame ( gameLoop );
  let dt = max ( min ( ( currentTime - lastTime ) / 1000 , 0.1 ) , 0 );
  lastTime = currentTime;
  if (gameState == "playing") {
    speed = min ( speed + dt * 0.15 , maxSpeed );
    score += speed * dt * baseMultiplier;
    if (invulnerableTimer > 0) {
      invulnerableTimer -= dt;
    }
    if (isRolling == true) {
      rollTimer -= dt;
      if (rollTimer <= 0) {
        isRolling = false;
      }
    }
    if (hoverboardActive == true) {
      hoverboardTimer -= dt;
      if (hoverboardTimer <= 0) {
        hoverboardActive = false;
        setVisible(hoverboardMeshGroup , false);
        (document.getElementById("hoverboard-widget") || document.querySelector("hoverboard-widget")).style.display = 'none';
      }
    }
    let dx = targetX - playerX;
    playerX += dx * 16 * dt;
    playerVY += gravity * dt;
    playerY += playerVY * dt;
    if (playerY <= 0) {
      playerY = 0;
      playerVY = 0;
      isGrounded = true;
      isJumping = false;
    }
    playerZ -= speed * dt;
    updateWorld(dt , playerZ);
    checkCollisions();
    updateHUD();
  }
  setPosition(jakeGroup , playerX , playerY , playerZ);
  updateJakeAnimation(dt);
  setPosX(camera , playerX * 0.65);
  setPosY(camera , playerY + 2.15);
  setPosZ(camera , playerZ + 4.1);
  lookCameraAt(playerX * 0.65 , playerY + 1.35 , playerZ - 10);
  renderUniverse();
}

window.addEventListener('resize', function(event) {
  updateCameraProjection(camera , getWindowWidth ( ) / getWindowHeight ( ));
  configureRenderer(renderer , getWindowWidth ( ) , getWindowHeight ( ) , min ( getDevicePixelRatio ( ) , 2 ));
});

window.addEventListener('keydown', function(event) {
  initAudio();
  if (( gameState == "start" || gameState == "gameover" ) && ( event [ "code" ] == "Space" || event [ "code" ] == "Enter" )) {
    if (gameState == "start") {
      startGame();
    }
    else {
      restartGame();
    }
    return;
  }
  if (event [ "code" ] == "ArrowLeft" || event [ "key" ] == "a" || event [ "key" ] == "A") {
    moveLeft();
  }
  if (event [ "code" ] == "ArrowRight" || event [ "key" ] == "d" || event [ "key" ] == "D") {
    moveRight();
  }
  if (event [ "code" ] == "ArrowUp" || event [ "key" ] == "w" || event [ "key" ] == "W") {
    jump();
  }
  if (event [ "code" ] == "ArrowDown" || event [ "key" ] == "s" || event [ "key" ] == "S") {
    roll();
  }
  if (event [ "code" ] == "Space") {
    activateHoverboard();
  }
  if (event [ "code" ] == "KeyP" || event [ "code" ] == "Escape") {
    togglePause();
  }
});

(function() {
  const targetEl = (document.getElementById("btn-play") || document.querySelector("btn-play"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      startGame();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-pause") || document.querySelector("btn-pause"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      togglePause();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-resume") || document.querySelector("btn-resume"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      togglePause();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-restart") || document.querySelector("btn-restart"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      restartGame();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-restart-pause") || document.querySelector("btn-restart-pause"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      restartGame();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-audio") || document.querySelector("btn-audio"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      isAudioMuted = ! isAudioMuted;
      (document.getElementById("audio-icon") || document.querySelector("audio-icon")).textContent = ( isAudioMuted ? "🔇" : "🔊" );
    });
  }
})();

initEngine();