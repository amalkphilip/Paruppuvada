// Initialize Chip-8 VM
const vm = new Chip8VM();

// Canvas & Audio Context setup
const canvas = document.getElementById('displayCanvas');
const ctx = canvas.getContext('2d');
let audioCtx = null;
let oscillator = null;
let gainNode = null;

// UI Elements
const btnPlayPause = document.getElementById('btnPlayPause');
const btnStep = document.getElementById('btnStep');
const btnReset = document.getElementById('btnReset');
const speedSlider = document.getElementById('speedSlider');
const speedVal = document.getElementById('speedVal');
const presetRomSelect = document.getElementById('presetRomSelect');
const romFileInput = document.getElementById('romFileInput');
const themeSelect = document.getElementById('themeSelect');
const cpuStatus = document.getElementById('cpuStatus');
const currentRomName = document.getElementById('currentRomName');

// Registers UI
const valPC = document.getElementById('valPC');
const valI = document.getElementById('valI');
const valSP = document.getElementById('valSP');
const valDT = document.getElementById('valDT');
const valST = document.getElementById('valST');
const valOpcode = document.getElementById('valOpcode');
const vRegsGrid = document.getElementById('vRegsGrid');
const disasmList = document.getElementById('disasmList');
const stackView = document.getElementById('stackView');

// Build V0-VF elements in DOM
for (let i = 0; i < 16; i++) {
  const hex = i.toString(16).toUpperCase();
  const div = document.createElement('div');
  div.className = 'v-reg-box';
  div.innerHTML = `<span class="v-lbl">V${hex}</span><span class="v-val" id="regV${i}">0x00</span>`;
  vRegsGrid.appendChild(div);
}

// Key mapping
const keyMap = {
  '1': 0x1, '2': 0x2, '3': 0x3, '4': 0xC,
  'q': 0x4, 'w': 0x5, 'e': 0x6, 'r': 0xD,
  'a': 0x7, 's': 0x8, 'd': 0x9, 'f': 0xE,
  'z': 0xA, 'x': 0x0, 'c': 0xB, 'v': 0xF
};

// State
let isRunning = false;
let animationFrameId = null;
let cyclesPerFrame = 9;

// Init Audio
function initAudio() {
  if (!audioCtx) {
    const AudioContext = window.AudioContext || window.webkitAudioContext;
    audioCtx = new AudioContext();
    gainNode = audioCtx.createGain();
    gainNode.gain.setValueAtTime(0, audioCtx.currentTime);
    gainNode.connect(audioCtx.destination);

    oscillator = audioCtx.createOscillator();
    oscillator.type = 'square';
    oscillator.frequency.setValueAtTime(440, audioCtx.currentTime); // 440 Hz standard A tone
    oscillator.connect(gainNode);
    oscillator.start();
  }
}

function setBeep(active) {
  if (!gainNode || !audioCtx) return;
  if (active) {
    gainNode.gain.setTargetAtTime(0.15, audioCtx.currentTime, 0.01);
  } else {
    gainNode.gain.setTargetAtTime(0, audioCtx.currentTime, 0.01);
  }
}

// Rendering
function renderDisplay() {
  const style = getComputedStyle(document.body);
  const colorOn = style.getPropertyValue('--pixel-on').trim() || '#39ff14';
  const colorOff = style.getPropertyValue('--pixel-off').trim() || '#051405';

  ctx.fillStyle = colorOff;
  ctx.fillRect(0, 0, 640, 320);

  ctx.fillStyle = colorOn;
  for (let y = 0; y < 32; y++) {
    for (let x = 0; x < 64; x++) {
      if (vm.display[x + y * 64] === 1) {
        ctx.fillRect(x * 10, y * 10, 10, 10);
      }
    }
  }
}

// Update UI Registers
function updateUI() {
  valPC.textContent = '0x' + vm.pc.toString(16).toUpperCase().padStart(3, '0');
  valI.textContent = '0x' + vm.index.toString(16).toUpperCase().padStart(3, '0');
  valSP.textContent = vm.sp;
  valDT.textContent = vm.delayTimer;
  valST.textContent = vm.soundTimer;
  valOpcode.textContent = '0x' + vm.currentOpcode.toString(16).toUpperCase().padStart(4, '0');

  for (let i = 0; i < 16; i++) {
    document.getElementById(`regV${i}`).textContent = '0x' + vm.v[i].toString(16).toUpperCase().padStart(2, '0');
  }

  // Update Stack
  if (vm.sp === 0) {
    stackView.textContent = '(Stack Empty)';
  } else {
    let stackStr = '';
    for (let i = 0; i < vm.sp; i++) {
      stackStr += `[#${i}: 0x${vm.stack[i].toString(16).toUpperCase().padStart(3, '0')}] `;
    }
    stackView.textContent = stackStr;
  }

  // Update Disassembly View
  updateDisassembly();
}

function updateDisassembly() {
  disasmList.innerHTML = '';
  const startAddr = Math.max(0x200, vm.pc - 6);
  const endAddr = Math.min(4094, vm.pc + 10);

  for (let addr = startAddr; addr <= endAddr; addr += 2) {
    const isCurrent = (addr === vm.pc);
    const row = document.createElement('div');
    row.className = 'disasm-row' + (isCurrent ? ' current' : '');

    const addrStr = '0x' + addr.toString(16).toUpperCase().padStart(3, '0');
    const op = (vm.memory[addr] << 8) | vm.memory[addr + 1];
    const opStr = '0x' + op.toString(16).toUpperCase().padStart(4, '0');
    const mnemonic = vm.disassemble(addr);

    row.innerHTML = `
      <span class="disasm-addr">${addrStr}</span>
      <span class="disasm-code">${opStr}</span>
      <span class="disasm-mnemonic">${mnemonic}</span>
    `;
    disasmList.appendChild(row);
  }
}

// Main Frame Loop
function frame() {
  if (!isRunning) return;

  for (let i = 0; i < cyclesPerFrame; i++) {
    vm.emulateCycle();
  }

  vm.updateTimers();
  setBeep(vm.soundTimer > 0);

  if (vm.drawFlag) {
    renderDisplay();
    vm.drawFlag = false;
  }

  updateUI();
  animationFrameId = requestAnimationFrame(frame);
}

function start() {
  if (!vm.isLoaded) return;
  initAudio();
  isRunning = true;
  btnPlayPause.textContent = '⏸ Pause';
  btnPlayPause.classList.remove('btn-primary');
  btnPlayPause.classList.add('btn-secondary');
  cpuStatus.textContent = 'RUNNING';
  cpuStatus.classList.remove('paused');
  animationFrameId = requestAnimationFrame(frame);
}

function pause() {
  isRunning = false;
  btnPlayPause.textContent = '▶ Run';
  btnPlayPause.classList.remove('btn-secondary');
  btnPlayPause.classList.add('btn-primary');
  cpuStatus.textContent = 'PAUSED';
  cpuStatus.classList.add('paused');
  setBeep(false);
  if (animationFrameId) {
    cancelAnimationFrame(animationFrameId);
    animationFrameId = null;
  }
  updateUI();
}

function step() {
  pause();
  initAudio();
  vm.emulateCycle();
  vm.updateTimers();
  renderDisplay();
  updateUI();
}

function reset() {
  pause();
  vm.reset();
  renderDisplay();
  updateUI();
}

// Load base64 ROM helper
function loadBase64Rom(name, b64) {
  const binaryString = atob(b64);
  const len = binaryString.length;
  const bytes = new Uint8Array(len);
  for (let i = 0; i < len; i++) {
    bytes[i] = binaryString.charCodeAt(i);
  }
  vm.loadRom(bytes.buffer);
  currentRomName.textContent = name + ' (' + len + ' bytes)';
  renderDisplay();
  updateUI();
  start();
}

// Event Listeners
btnPlayPause.addEventListener('click', () => {
  if (isRunning) pause();
  else start();
});

btnStep.addEventListener('click', step);
btnReset.addEventListener('click', () => {
  const val = presetRomSelect.value;
  if (val && PRESET_ROMS[val]) {
    loadBase64Rom(val, PRESET_ROMS[val]);
  } else {
    reset();
  }
});

speedSlider.addEventListener('input', (e) => {
  cyclesPerFrame = parseInt(e.target.value, 10);
  speedVal.textContent = `${cyclesPerFrame} cycles/frame`;
});

presetRomSelect.addEventListener('change', (e) => {
  const val = e.target.value;
  if (val && PRESET_ROMS[val]) {
    loadBase64Rom(val, PRESET_ROMS[val]);
  }
});

romFileInput.addEventListener('change', (e) => {
  const file = e.target.files[0];
  if (file) {
    const reader = new FileReader();
    reader.onload = function(evt) {
      vm.loadRom(evt.target.result);
      currentRomName.textContent = file.name + ' (' + file.size + ' bytes)';
      renderDisplay();
      updateUI();
      start();
    };
    reader.readAsArrayBuffer(file);
  }
});

themeSelect.addEventListener('change', (e) => {
  document.body.className = e.target.value;
  renderDisplay();
});

// Keypad DOM handlers
const keyButtons = document.querySelectorAll('.key-btn');
keyButtons.forEach(btn => {
  const keyVal = parseInt(btn.getAttribute('data-key'), 16);
  btn.addEventListener('mousedown', () => {
    vm.key[keyVal] = 1;
    btn.classList.add('active');
  });
  btn.addEventListener('mouseup', () => {
    vm.key[keyVal] = 0;
    btn.classList.remove('active');
  });
  btn.addEventListener('mouseleave', () => {
    vm.key[keyVal] = 0;
    btn.classList.remove('active');
  });
});

// Physical Keyboard Event Handlers
window.addEventListener('keydown', (e) => {
  const key = e.key.toLowerCase();
  if (key in keyMap) {
    const chipKey = keyMap[key];
    vm.key[chipKey] = 1;
    const btn = document.querySelector(`.key-btn[data-key="0x${chipKey.toString(16).toUpperCase()}"]`);
    if (btn) btn.classList.add('active');
  }
});

window.addEventListener('keyup', (e) => {
  const key = e.key.toLowerCase();
  if (key in keyMap) {
    const chipKey = keyMap[key];
    vm.key[chipKey] = 0;
    const btn = document.querySelector(`.key-btn[data-key="0x${chipKey.toString(16).toUpperCase()}"]`);
    if (btn) btn.classList.remove('active');
  }
});

// Auto load Pong on start
if (typeof PRESET_ROMS !== 'undefined' && PRESET_ROMS.Pong) {
  presetRomSelect.value = 'Pong';
  loadBase64Rom('Pong', PRESET_ROMS.Pong);
} else {
  renderDisplay();
  updateUI();
}
