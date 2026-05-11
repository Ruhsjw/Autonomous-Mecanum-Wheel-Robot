const char body[] PROGMEM = R"===(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Car Buttons</title>
<style>
  body { font-family: system-ui, -apple-system, Segoe UI, Roboto, Arial; margin: 24px; }
  .row { display:flex; gap:24px; align-items:flex-start; flex-wrap: wrap; }
  #controls {
    display: grid;
    grid-template-columns: repeat(3, 100px);
    grid-auto-rows: 60px;
    gap: 12px;
  }
  .btn {
    display:flex;
    align-items:center;
    justify-content:center;
    border: 1px solid #ccc;
    border-radius: 8px;
    background:#f5f5f5;
    cursor:pointer;
    user-select:none;
    font-weight:500;
    box-shadow: 0 2px 4px rgba(0,0,0,.08);
    touch-action: manipulation;
  }
  .btn:active {
    transform: translateY(1px);
    box-shadow: 0 1px 3px rgba(0,0,0,.15);
    background:#e9e9e9;
  }

  /* New toggle buttons look identical but do NOT get joystick handling */
  .togglebtn {
    display:flex;
    align-items:center;
    justify-content:center;
    border: 1px solid #ccc;
    border-radius: 8px;
    background:#f5f5f5;
    cursor:pointer;
    user-select:none;
    font-weight:500;
    box-shadow: 0 2px 4px rgba(0,0,0,.08);
    touch-action: manipulation;
  }
  .togglebtn:active {
    transform: translateY(1px);
    box-shadow: 0 1px 3px rgba(0,0,0,.15);
    background:#e9e9e9;
  }

  #modeToggle {
    margin-bottom: 16px;
    padding: 8px 16px;
    border-radius: 999px;
    border: 1px solid #888;
    background:#fff;
    cursor:pointer;
    font-weight:600;
  }

  #readout { min-width: 260px; }
  #outputlabel { font-weight: 600; margin-top: 8px; display:block; }
  code { background:#f6f6f6; padding:2px 6px; border-radius:4px; }
</style>
</head>
<body>

<h2>Car Control</h2>
<div class="row">
  <div>
    <button id="modeToggle" type="button"></button>

    <div id="controls">

      <!-- === ORIGINAL BUTTONS PRESERVED EXACTLY === -->

      <!-- Row 1 -->
      <div></div>
      <div class="btn" data-dx="0"   data-dy="100" data-w="0">Forward</div>
      <div></div>

      <!-- Row 2 -->
      <div class="btn" data-dx="-100" data-dy="100" data-w="0">Front Left</div>
      <div class="btn" data-dx="0"    data-dy="0"   data-w="0">Stop</div>
      <div class="btn" data-dx="100"  data-dy="100" data-w="0">Front Right</div>

      <!-- Row 3 -->
      <div class="btn" data-dx="-100" data-dy="0"   data-w="0">Left</div>
      <div></div>
      <div class="btn" data-dx="100"  data-dy="0"   data-w="0">Right</div>

      <!-- Row 4 -->
      <div class="btn" data-dx="-100" data-dy="-100" data-w="0">Back Left</div>
      <div class="btn" data-dx="0"    data-dy="-100" data-w="0">Backward</div>
      <div class="btn" data-dx="100"  data-dy="-100" data-w="0">Back Right</div>

      <!-- Row 5 -->
      <div class="btn" data-dx="0" data-dy="0" data-w="100">Spin Left</div>
      <div></div>
      <div class="btn" data-dx="0" data-dy="0" data-w="-100">Spin Right</div>

      <!-- === NEW BUTTONS (Row 6) === -->
      <div></div>
      <div class="togglebtn" id="wallToggleBtn">Wall Follow</div>
      <div class="togglebtn" id="servoToggleBtn">Servo Enable</div>

      <!-- === NEW BUTTONS (Row 7) === -->
      <div class="togglebtn" id="toggleBtn1">LOW R</div>
      <div class="togglebtn" id="toggleBtn2">HIGH R</div>
      <div class="togglebtn" id="toggleBtn3">NEXUS R</div>

      <!-- === NEW BUTTONS (Row 8) === -->
      <div class="togglebtn" id="toggleBtn4">LOW B</div>
      <div class="togglebtn" id="toggleBtn5">HIGH B</div>
      <div class="togglebtn" id="toggleBtn6">NEXUS B</div>
      
      <!-- === NEW BUTTONS (Row 9) === -->
      <div class="togglebtn" id="toggleBtn7">Combo R</div>
      <div class="togglebtn" id="toggleBtn8">attack R</div>
      <div class="togglebtn" id="toggleBtn9">attack B</div>

      <!-- === NEW BUTTONS (Row 10) === -->
      <div class="togglebtn" id="toggleBtn10">auto nexus R</div>
      <div class="togglebtn" id="toggleBtn11">auto high R</div>
      <div class="togglebtn" id="toggleBtn12">auto low R</div>


    </div>
  </div>

  <div id="readout">
    <div>Sending: <code>joy?data=&lt;mode&gt;,&lt;dx&gt;,&lt;dy&gt;,&lt;w&gt;</code></div>
    <p>Mode: <span id="modeText"></span></p>
    <p>Last command: <span id="cmdText">data=0,0,0,0</span></p>
    <span id="outputlabel"></span>

    <!-- System params coming from "params" endpoint -->
    <h3>System Parameters</h3>
    <p>A = <span id="paramA">0</span></p>
    <p>B = <span id="paramB">0</span></p>
    <p>C = <span id="paramC">0</span></p>
    <p>D = <span id="paramD">0</span></p>

    <!-- Send two uint16 values to the server -->
    <h3>Send Uint16 Values</h3>
    <p>
      <label>Value 1:
        <input id="txVal1" type="number" min="0" max="65535" value="0">
      </label>
    </p>
    <p>
      <label>Value 2:
        <input id="txVal2" type="number" min="0" max="65535" value="0">
      </label>
    </p>
    <button id="sendUint16Btn" class="togglebtn" type="button">Send Values</button>

    <!-- NEW: E-STOP BUTTON -->
    <h3>Emergency</h3>
    <button id="estopBtn" class="togglebtn" type="button"
            style="margin-top:8px; background:#ffdddd; border-color:#ff0000;">
      E-STOP
    </button>
  </div>
</div>

<script>
(() => {
  let mode = 0;

  const modeToggle = document.getElementById('modeToggle');
  const modeText   = document.getElementById('modeText');
  const cmdText    = document.getElementById('cmdText');

  function updateModeUI() {
    if (mode === 0) {
      modeToggle.textContent = 'Switch to GLOBAL head';
      modeText.textContent = 'Relative head (mode=0)';
    } else {
      modeToggle.textContent = 'Switch to RELATIVE head';
      modeText.textContent = 'Global head (mode=1)';
    }
  }

  modeToggle.addEventListener('click', () => {
    mode = (mode === 0) ? 1 : 0;
    updateModeUI();
  });

  function sendData(dx, dy, w) {
    const dataStr = mode + ',' + dx + ',' + dy + ',' + w;
    cmdText.textContent = 'data=' + dataStr;

    const xhttp = new XMLHttpRequest();
    const url = 'joy?data=' + dataStr;
    xhttp.open('GET', url, true);
    xhttp.send();
  }

  function attachButtonEvents(btn) {
    const dx = parseInt(btn.getAttribute('data-dx'), 10);
    const dy = parseInt(btn.getAttribute('data-dy'), 10);
    const w  = parseInt(btn.getAttribute('data-w'), 10);

    btn.addEventListener('pointerdown', (e) => {
      e.preventDefault();
      sendData(dx, dy, w);
    }, { passive: false });

    btn.addEventListener('pointerup', (e) => {
      e.preventDefault();
      sendData(0, 0, 0);
    }, { passive: false });

    btn.addEventListener('pointercancel', (e) => {
      e.preventDefault();
      sendData(0, 0, 0);
    }, { passive: false });
  }

  document.querySelectorAll('.btn').forEach(attachButtonEvents);

  updateModeUI();
})();

setInterval(updateLabel, 50);
updateLabel();
function updateLabel() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("outputlabel").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "measuredSpeed", true);
  xhttp.send();
}
</script>

<!-- Toggle button GET calls -->
<script>
document.getElementById("wallToggleBtn").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "walltoggle", true);
  xhttp.send();
});

document.getElementById("servoToggleBtn").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "servotoggle", true);
  xhttp.send();
});
</script>

<script>
document.getElementById("toggleBtn1").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle1", true);
  xhttp.send();
});

document.getElementById("toggleBtn2").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle2", true);
  xhttp.send();
});

document.getElementById("toggleBtn3").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle3", true);
  xhttp.send();
});


document.getElementById("toggleBtn4").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle4", true);
  xhttp.send();
});
document.getElementById("toggleBtn5").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle5", true);
  xhttp.send();
});
document.getElementById("toggleBtn6").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle6", true);
  xhttp.send();
});

document.getElementById("toggleBtn7").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle7", true);
  xhttp.send();
});
document.getElementById("toggleBtn8").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle8", true);
  xhttp.send();
});
document.getElementById("toggleBtn9").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle9", true);
  xhttp.send();
});


document.getElementById("toggleBtn10").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle10", true);
  xhttp.send();
});


document.getElementById("toggleBtn11").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle11", true);
  xhttp.send();
});


document.getElementById("toggleBtn12").addEventListener("pointerdown", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "toggle12", true);
  xhttp.send();
});
</script>


<!-- Read paramA..D every 100ms -->
<script>
function updateParams() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      // Directly print whatever the server sent
      document.getElementById("paramA").textContent = this.responseText;
      // Optionally clear the others
      document.getElementById("paramB").textContent = "";
      document.getElementById("paramC").textContent = "";
      document.getElementById("paramD").textContent = "";
    }
  };
  xhttp.open("GET", "params", true);
  xhttp.send();
}

setInterval(updateParams, 100);
updateParams();

</script>

<!-- Send two uint16 values to the server via GET /setvals?data=v1,v2 -->
<script>
function sendUint16Values() {
  const input1 = document.getElementById("txVal1");
  const input2 = document.getElementById("txVal2");

  let v1 = parseInt(input1.value, 10);
  let v2 = parseInt(input2.value, 10);

  if (isNaN(v1)) v1 = 0;
  if (isNaN(v2)) v2 = 0;

  // clamp to uint16 range
  if (v1 < 0) v1 = 0; else if (v1 > 65535) v1 = 65535;
  if (v2 < 0) v2 = 0; else if (v2 > 65535) v2 = 65535;

  input1.value = v1;
  input2.value = v2;

  const payload = v1 + "," + v2;

  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "setvals?data=" + payload, true);
  xhttp.send();
}

document.getElementById("sendUint16Btn").addEventListener("click", function(e) {
  e.preventDefault();
  sendUint16Values();
});
</script>

<!-- NEW: E-STOP request script -->
<script>
document.getElementById("estopBtn").addEventListener("click", function(e) {
  e.preventDefault();
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "estop", true);
  xhttp.send();
});
</script>

</body>
</html>
)===";
