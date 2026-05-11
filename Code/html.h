const char body[] PROGMEM = R"===(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Car Joystick</title>
<style>
  body { font-family: system-ui, -apple-system, Segoe UI, Roboto, Arial; margin: 24px; }
  .row { display:flex; gap:24px; align-items:flex-start; flex-wrap: wrap; }
  #joystick {
    position: relative;
    width: 220px; height: 220px;
    border-radius: 50%;
    border: 2px solid #ccc;
    box-shadow: inset 0 0 12px rgba(0,0,0,.08);
    touch-action: none;  /* allow drag without scrolling */
    user-select: none;
  }
  #knob {
    position: absolute;
    width: 70px; height: 70px;
    border-radius: 50%;
    background: #eee;
    border: 2px solid #aaa;
    box-shadow: 0 4px 10px rgba(0,0,0,.15);
    transform: translate(-50%, -50%);
    left: 50%; top: 50%;
  }
  #readout { min-width: 240px; }
  #outputlabel { font-weight: 600; }
  code { background:#f6f6f6; padding:2px 6px; border-radius:4px; }
</style>
</head>
<body>

<h2>Virtual Joystick</h2>
<div class="row">
  <div id="joystick" aria-label="virtual joystick" role="application">
    <div id="knob"></div>
  </div>

  <div id="readout">
    <div>Sending to <code>joy?x=&lt;val&gt;&amp;y=&lt;val&gt;</code> (~30 Hz)</div>
    <p>Vector: <span id="vec">x: 0, y: 0</span></p>
    /* <p>Magnitude: <span id="mag">0</span></p> */
    <p><span id="outputlabel"> </span></p>
  </div>
</div>

<script>
(() => {
  const joy = document.getElementById('joystick');
  const knob = document.getElementById('knob');
  const vecSpan = document.getElementById('vec');
  // const magSpan = document.getElementById('mag');

  const box = 220;                   // joystick diameter (px)
  const radius = box / 2;            // 110
  const travel = radius - 10 - 35;   // max knob center radius (px): border/space & knob radius
  const normMax = 100;               // map to [-100,100]

  let active = false;
  let center = { x: 0, y: 0 };

  // rate limit XHRs to ~30Hz
  const minInterval = 33;
  let lastSent = 0;
  let pending = null;

  function sendXY(nx, ny) {
    const now = performance.now();
    const doSend = () => {
      const xhttp = new XMLHttpRequest();
      const url = `joy?xy=${nx},${ny}`;
      xhttp.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
          outLabel.textContent = this.responseText;
        }
      };
      xhttp.open("GET", url, true);
      xhttp.send();
      lastSent = performance.now();
      pending = null;
    };

    if (now - lastSent >= minInterval) {
      doSend();
    } else {
      // collapse multiple moves into one latest send
      pending = { nx, ny };
      const delay = Math.max(0, minInterval - (now - lastSent));
      setTimeout(() => {
        if (pending) {
          const { nx, ny } = pending;
          pending = null;
          sendXY(nx, ny);
        }
      }, delay);
    }
  }

  function setKnob(px, py) {
    knob.style.left = `${px}px`;
    knob.style.top  = `${py}px`;
  }

  function updateFromPointer(clientX, clientY) {
    const rect = joy.getBoundingClientRect();
    const cx = rect.left + rect.width / 2;
    const cy = rect.top + rect.height / 2;
    const dx = clientX - cx;
    const dy = clientY - cy;

    // limit within circle
    const dist = Math.hypot(dx, dy);
    const clamped = dist > travel ? travel / dist : 1;
    const ndx = dx * clamped;
    const ndy = dy * clamped;

    // place knob (convert to local coords)
    setKnob(radius + ndx, radius + ndy);

    // normalize to [-100, 100]; invert y so up is positive
    const nx = Math.round((ndx / travel) * normMax);
    const ny = Math.round((-ndy / travel) * normMax);
    // const mag = Math.min(normMax, Math.round((dist / travel) * normMax));

    vecSpan.textContent = `x: ${nx}, y: ${ny}`;
    // magSpan.textContent = `${mag}`;

    sendXY(nx, ny);
  }

  function centerKnob(sendZero = true) {
    setKnob(radius, radius);
    vecSpan.textContent = `x: 0, y: 0`;
    // magSpan.textContent = `0`;
    if (sendZero) sendXY(0, 0);
  }

  // Pointer events
  joy.addEventListener('pointerdown', (e) => {
    active = true;
    joy.setPointerCapture(e.pointerId);
    updateFromPointer(e.clientX, e.clientY);
    e.preventDefault();
  }, { passive: false });
  joy.addEventListener('pointermove', (e) => {
    if (!active) return;
    updateFromPointer(e.clientX, e.clientY);
    e.preventDefault();
  }, { passive: false });
  const end = (e) => {
    if (!active) return;
    active = false;
    try { joy.releasePointerCapture(e.pointerId); } catch {}
    centerKnob(true);
    e.preventDefault();
  };
  joy.addEventListener('pointerup', end, { passive: false });
  joy.addEventListener('pointercancel', end, { passive: false });
  joy.addEventListener('pointerleave', (e) => { if (active) end(e); }, { passive: false });
  // initialize
  centerKnob(false);
})();

        setInterval(updateLabel, 50);
        updateLabel();
        function updateLabel()
        {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function()
            {
                if (this.readyState == 4 && this.status == 200)
                {
                    document.getElementById("outputlabel").innerHTML = this.responseText;
                }
            }
            xhttp.open("GET", "measuredSpeed", true);
            xhttp.send();
        }
</script>
</body>
</html>
)===";
