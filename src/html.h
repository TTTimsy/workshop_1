#ifndef HTML_H
#define HTML_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>InnoX Boat RC</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body {
    font-family: -apple-system, 'Segoe UI', sans-serif;
    background: #1a1a2e;
    color: #eee;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    user-select: none;
    -webkit-user-select: none;
    touch-action: none;
  }
  h1 { font-size: 1.5rem; margin-bottom: 4px; letter-spacing: 2px; }
  #status {
    font-size: 0.85rem;
    padding: 4px 14px;
    border-radius: 12px;
    margin-bottom: 20px;
    background: #ff4444;
    transition: background 0.3s;
  }
  #status.connected { background: #44bb44; }

  .motors-container {
    display: flex;
    flex-direction: row;
    justify-content: center;
    align-items: center;
    gap: 40px;
    width: 100%;
    max-width: 400px;
  }
  .motor-group {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 6px;
  }
  .motor-label {
    font-size: 0.9rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 1px;
    color: #888;
  }
  .speed-label {
    font-size: 0.8rem;
    color: #aaa;
    font-variant-numeric: tabular-nums;
    min-width: 3em;
    text-align: center;
  }

  /* --- Custom slider track --- */
  .slider-track {
    position: relative;
    width: 64px;
    height: 280px;
    background: linear-gradient(to top, #6a2d2d 50%, #2d6a2d 50%);
    border-radius: 32px;
    cursor: pointer;
    touch-action: none;
    box-shadow: inset 0 2px 8px rgba(0,0,0,0.5);
  }
  .slider-track::before {
    content: '';
    position: absolute;
    top: 50%;
    left: 8px;
    right: 8px;
    height: 2px;
    background: rgba(255,255,255,0.3);
    transform: translateY(-1px);
    pointer-events: none;
  }

  /* --- Thumb --- */
  .slider-thumb {
    position: absolute;
    left: 6px;
    right: 6px;
    height: 20px;
    background: #ddd;
    border-radius: 12px;
    box-shadow: 0 2px 6px rgba(0,0,0,0.4);
    transition: top 0.25s cubic-bezier(0.34, 1.56, 0.64, 1);
    pointer-events: none;
  }
  .slider-track.dragging .slider-thumb {
    transition: none;
  }

  /* --- Start button --- */
  .start-btn {
    margin-top: 24px;
    padding: 14px 40px;
    font-size: 1.1rem;
    font-weight: 700;
    letter-spacing: 2px;
    border: none;
    border-radius: 40px;
    cursor: pointer;
    transition: background 0.3s, opacity 0.3s;
    touch-action: manipulation;
    background: #d4a017;
    color: #1a1a2e;
  }
  .start-btn:disabled {
    opacity: 0.3;
    cursor: not-allowed;
  }
  .start-btn:not(:disabled):hover {
    background: #f0c040;
  }
  .start-btn.active {
    background: #44bb44;
    color: #fff;
  }

  @media (max-width: 400px) {
    .slider-track { height: 220px; width: 54px; }
    .motors-container { gap: 24px; }
    h1 { font-size: 1.2rem; }
  }
</style>
</head>
<body>

<h1>⛵ INNOX BOAT</h1>
<div id="status">Disconnected</div>

<div class="motors-container">

<div class="motor-group">
  <div class="motor-label">Motor A</div>
  <div class="slider-track" data-motor="a">
    <div class="slider-thumb"></div>
  </div>
  <div class="speed-label" id="speed-a">0</div>
</div>

<div class="motor-group">
  <div class="motor-label">Motor B</div>
  <div class="slider-track" data-motor="b">
    <div class="slider-thumb"></div>
  </div>
  <div class="speed-label" id="speed-b">0</div>
</div>

</div>

<button id="startBtn" class="start-btn" disabled>⏻ START MOTORS</button>

<script>
(function() {
  const statusEl = document.getElementById('status');
  const startBtn = document.getElementById('startBtn');
  let ws = null;
  let reconnectTimer = null;
  let sendThrottle = {};
  let motorsEnabled = false;

  // --- Slider tracks (enable/disable based on motorsEnabled) ---
  const sliderTracks = document.querySelectorAll('.slider-track');

  function setSlidersEnabled(enabled) {
    sliderTracks.forEach(function(track) {
      track.style.pointerEvents = enabled ? 'auto' : 'none';
      track.style.opacity = enabled ? '1' : '0.4';
    });
  }
  setSlidersEnabled(false);  // disabled until START

  function connect() {
    if (ws && ws.readyState === WebSocket.OPEN) return;
    ws = new WebSocket('ws://' + window.location.hostname + ':81');
    ws.onopen = function() {
      statusEl.textContent = 'Connected';
      statusEl.classList.add('connected');
    };
    ws.onmessage = function(e) {
      try {
        var data = JSON.parse(e.data);
        if (data.hasOwnProperty('motorsEnabled')) {
          motorsEnabled = data.motorsEnabled;
          setSlidersEnabled(motorsEnabled);
          if (motorsEnabled) {
            startBtn.textContent = '✓ MOTORS ACTIVE';
            startBtn.classList.add('active');
            startBtn.disabled = true;
          }
        }
      } catch(_) {}
    };
    ws.onclose = function() {
      statusEl.textContent = 'Disconnected — reconnecting...';
      statusEl.classList.remove('connected');
      scheduleReconnect();
    };
    ws.onerror = function() { ws.close(); };
  }

  function scheduleReconnect() {
    if (reconnectTimer) clearTimeout(reconnectTimer);
    reconnectTimer = setTimeout(connect, 2000);
  }

  // --- START button ---
  startBtn.disabled = false;
  startBtn.addEventListener('click', function() {
    if (ws && ws.readyState === WebSocket.OPEN && !motorsEnabled) {
      ws.send(JSON.stringify({ cmd: 'start' }));
    }
  });

  function sendCmd(motor, speed) {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ motor: motor, speed: speed }));
    }
  }

  // Throttle — max one send per motor per 50ms
  function throttledSend(motor, speed) {
    const now = Date.now();
    if (!sendThrottle[motor] || now - sendThrottle[motor].last > 50) {
      sendThrottle[motor] = { last: now };
      sendCmd(motor, speed);
    }
  }

  // --- Slider logic ---
  // For mouse: only one cursor, so we track the active track globally
  var activeTrack = null;

  document.querySelectorAll('.slider-track').forEach(function(track) {
    const motor   = track.getAttribute('data-motor');
    const thumb   = track.querySelector('.slider-thumb');
    const speedEl = document.getElementById('speed-' + motor);

    function posToSpeed(clientY) {
      const rect = track.getBoundingClientRect();
      const frac = 1 - (clientY - rect.top) / rect.height;
      const clamped = Math.max(0, Math.min(1, frac));
      return Math.round(clamped * 510 - 255);           // -255 .. +255
    }

    function updateThumb(speed) {
      const frac = 1 - (speed + 255) / 510;             // +255→top, -255→bottom
      const topPx = frac * (track.clientHeight - thumb.clientHeight);
      thumb.style.top = topPx + 'px';
      speedEl.textContent = speed;
    }

    // --- Mouse handlers (single cursor — use activeTrack) ---
    function onMouseDown(e) {
      e.preventDefault();
      activeTrack = track;
      track.classList.add('dragging');
      var speed = posToSpeed(e.clientY);
      updateThumb(speed);
      sendCmd(motor, speed);
    }

    function onMouseMove(e) {
      if (activeTrack !== track) return;
      e.preventDefault();
      var speed = posToSpeed(e.clientY);
      updateThumb(speed);
      throttledSend(motor, speed);
    }

    function onMouseUp(e) {
      if (activeTrack !== track) return;
      activeTrack = null;
      track.classList.remove('dragging');
      updateThumb(0);
      sendCmd(motor, 0);
    }

    // --- Touch handlers (per-track — each finger has its own touch) ---
    var touchId = null;

    function onTouchStart(e) {
      // Only grab one finger per track
      if (touchId !== null) return;
      e.preventDefault();
      var t = e.changedTouches[0];
      touchId = t.identifier;
      track.classList.add('dragging');
      var speed = posToSpeed(t.clientY);
      updateThumb(speed);
      sendCmd(motor, speed);
    }

    function onTouchMove(e) {
      if (touchId === null) return;
      // Find our finger
      for (var i = 0; i < e.changedTouches.length; i++) {
        if (e.changedTouches[i].identifier === touchId) {
          e.preventDefault();
          var speed = posToSpeed(e.changedTouches[i].clientY);
          updateThumb(speed);
          throttledSend(motor, speed);
          return;
        }
      }
    }

    function onTouchEnd(e) {
      if (touchId === null) return;
      for (var i = 0; i < e.changedTouches.length; i++) {
        if (e.changedTouches[i].identifier === touchId) {
          e.preventDefault();
          touchId = null;
          track.classList.remove('dragging');
          updateThumb(0);
          sendCmd(motor, 0);
          return;
        }
      }
    }

    // Mouse
    track.addEventListener('mousedown', onMouseDown);
    document.addEventListener('mousemove', onMouseMove);
    document.addEventListener('mouseup',   onMouseUp);

    // Touch
    track.addEventListener('touchstart', onTouchStart, { passive: false });
    track.addEventListener('touchmove',  onTouchMove,  { passive: false });
    track.addEventListener('touchend',   onTouchEnd,   { passive: false });
    track.addEventListener('touchcancel', onTouchEnd,  { passive: false });
  });

  connect();
})();
</script>
</body>
</html>
)rawliteral";

#endif // HTML_H
