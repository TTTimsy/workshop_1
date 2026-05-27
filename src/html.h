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
    position: relative;
  }
  h1 { font-size: 1.8rem; margin-bottom: 6px; letter-spacing: 3px; }
  #status {
    font-size: 1rem;
    padding: 6px 20px;
    border-radius: 14px;
    margin-bottom: 24px;
    background: #ff4444;
    transition: background 0.3s;
  }
  #status.connected { background: #44bb44; }

  /* --- Disconnect overlay --- */
  #disconnect-overlay {
    display: none;
    position: fixed;
    inset: 0;
    background: rgba(0,0,0,0.85);
    z-index: 9999;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 16px;
  }
  #disconnect-overlay.visible { display: flex; }
  #disconnect-overlay .icon {
    font-size: 4rem;
    line-height: 1;
  }
  #disconnect-overlay .msg {
    font-size: 1.6rem;
    font-weight: 700;
    color: #ff6b6b;
  }
  #disconnect-overlay .sub {
    font-size: 0.95rem;
    color: #aaa;
    text-align: center;
    max-width: 300px;
  }
  #disconnect-overlay .retry-btn {
    margin-top: 8px;
    padding: 12px 36px;
    font-size: 1rem;
    font-weight: 600;
    border: none;
    border-radius: 30px;
    background: #d4a017;
    color: #1a1a2e;
    cursor: pointer;
    touch-action: manipulation;
  }
  #disconnect-overlay .retry-btn:active {
    background: #f0c040;
  }

  .motors-container {
    display: flex;
    flex-direction: row;
    justify-content: center;
    align-items: center;
    gap: 60px;
    width: 100%;
    max-width: 600px;
  }
  .motor-group {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
  }
  .motor-label {
    font-size: 1.1rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 2px;
    color: #888;
  }
  .speed-label {
    font-size: 1rem;
    color: #aaa;
    font-variant-numeric: tabular-nums;
    min-width: 3.5em;
    text-align: center;
  }

  /* --- Custom slider track --- */
  .slider-track {
    position: relative;
    width: 100px;
    height: min(60vh, 420px);
    min-height: 300px;
    background: linear-gradient(to top, #6a2d2d 50%, #2d6a2d 50%);
    border-radius: 50px;
    cursor: pointer;
    touch-action: none;
    box-shadow: inset 0 2px 12px rgba(0,0,0,0.5);
  }
  .slider-track::before {
    content: '';
    position: absolute;
    top: 50%;
    left: 12px;
    right: 12px;
    height: 2px;
    background: rgba(255,255,255,0.3);
    transform: translateY(-1px);
    pointer-events: none;
  }

  /* --- Thumb --- */
  .slider-thumb {
    position: absolute;
    left: 8px;
    right: 8px;
    height: 32px;
    top: 50%;
    background: #ddd;
    border-radius: 16px;
    box-shadow: 0 3px 10px rgba(0,0,0,0.5);
    transform: translateY(-50%);
    transition: top 0.25s cubic-bezier(0.34, 1.56, 0.64, 1);
    pointer-events: none;
  }
  .slider-track.dragging .slider-thumb {
    transition: none;
  }

  /* --- Start button --- */
  .start-btn {
    margin-top: 28px;
    padding: 16px 48px;
    font-size: 1.2rem;
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

  @media (max-width: 500px) {
    .slider-track { width: 76px; min-height: 260px; }
    .motors-container { gap: 36px; }
    h1 { font-size: 1.4rem; }
    .motor-label { font-size: 0.95rem; }
    .speed-label { font-size: 0.85rem; }
    .start-btn { padding: 14px 36px; font-size: 1rem; }
  }
  @media (max-width: 360px) {
    .slider-track { width: 64px; min-height: 220px; }
    .motors-container { gap: 24px; }
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

<!-- Disconnect overlay -->
<div id="disconnect-overlay">
  <div class="icon">⚠️</div>
  <div class="msg">Connection Lost</div>
  <div class="sub">Signal weak or boat is out of range.<br>Reconnecting automatically…</div>
  <button class="retry-btn" id="retryBtn">↻ Retry Now</button>
</div>

<script>
(function() {
  const statusEl = document.getElementById('status');
  const startBtn = document.getElementById('startBtn');
  const overlayEl = document.getElementById('disconnect-overlay');
  const retryBtn = document.getElementById('retryBtn');
  let ws = null;
  let reconnectTimer = null;
  let sendThrottle = {};
  let motorsEnabled = false;

  // --- Reconnection state ---
  let reconnectAttempt = 0;
  let reconnectBaseDelay = 1000;    // start at 1s
  let reconnectMaxDelay = 30000;    // cap at 30s
  let wasEverConnected = false;     // set true after first successful onopen

  // --- App-level keepalive ---
  let pingInterval = null;
  let pongTimeout = null;
  let lastPongTime = 0;
  const PING_INTERVAL = 3000;       // send {"ping":1} every 3s
  const PONG_TIMEOUT = 9000;        // disconnect if no pong for 9s

  // --- Connection timeout ---
  let connectTimeout = null;
  const CONNECT_TIMEOUT = 5000;     // fail if not open within 5s

  // --- Disconnect overlay ---
  function showOverlay() {
    overlayEl.classList.add('visible');
  }
  function hideOverlay() {
    overlayEl.classList.remove('visible');
  }

  // --- Slider tracks (enable/disable based on motorsEnabled) ---
  const sliderTracks = document.querySelectorAll('.slider-track');

  function setSlidersEnabled(enabled) {
    sliderTracks.forEach(function(track) {
      track.style.pointerEvents = enabled ? 'auto' : 'none';
      track.style.opacity = enabled ? '1' : '0.4';
    });
  }
  setSlidersEnabled(false);  // disabled until START

  // --- App-level ping/pong ---
  function startKeepalive() {
    stopKeepalive();
    lastPongTime = Date.now();
    pingInterval = setInterval(function() {
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ ping: 1 }));
        // If we haven't seen a pong in PONG_TIMEOUT, treat as dead
        if (Date.now() - lastPongTime > PONG_TIMEOUT) {
          console.warn('[Keepalive] No pong received, forcing disconnect');
          ws.close();
        }
      }
    }, PING_INTERVAL);
  }

  function stopKeepalive() {
    if (pingInterval) { clearInterval(pingInterval); pingInterval = null; }
    if (pongTimeout)  { clearTimeout(pongTimeout);   pongTimeout = null; }
  }

  function handlePong() {
    lastPongTime = Date.now();
  }

  // --- Exponential backoff ---
  function getReconnectDelay() {
    var delay = reconnectBaseDelay * Math.pow(2, reconnectAttempt);
    if (delay > reconnectMaxDelay) delay = reconnectMaxDelay;
    // Add small random jitter (±20%) to avoid thundering herd
    delay = delay * (0.8 + Math.random() * 0.4);
    return Math.round(delay);
  }

  function connect() {
    if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) return;

    stopKeepalive();

    ws = new WebSocket('ws://' + window.location.hostname + ':81');

    // Connection timeout
    if (connectTimeout) clearTimeout(connectTimeout);
    connectTimeout = setTimeout(function() {
      if (ws && ws.readyState === WebSocket.CONNECTING) {
        console.warn('[WS] Connection timed out');
        ws.close();
      }
    }, CONNECT_TIMEOUT);

    ws.onopen = function() {
      if (connectTimeout) { clearTimeout(connectTimeout); connectTimeout = null; }
      wasEverConnected = true;
      reconnectAttempt = 0;         // reset backoff on success
      hideOverlay();
      statusEl.textContent = 'Connected';
      statusEl.classList.add('connected');
      startKeepalive();
    };

    ws.onmessage = function(e) {
      try {
        var data = JSON.parse(e.data);
        if (data.hasOwnProperty('pong')) {
          handlePong();
          return;
        }
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
      stopKeepalive();
      statusEl.textContent = 'Disconnected — reconnecting...';
      statusEl.classList.remove('connected');

      // Only reset motors state if we were connected (not on first load)
      if (wasEverConnected) {
        motorsEnabled = false;
        setSlidersEnabled(false);
        startBtn.textContent = '⏻ START MOTORS';
        startBtn.classList.remove('active');
        startBtn.disabled = false;
        showOverlay();
      }

      scheduleReconnect();
    };

    ws.onerror = function() {
      // onclose fires after onerror, so just close to trigger reconnect
      ws.close();
    };
  }

  function scheduleReconnect() {
    if (reconnectTimer) clearTimeout(reconnectTimer);
    // On first load before ever connected, reconnect quickly
    var delay = wasEverConnected ? getReconnectDelay() : 1000;
    reconnectAttempt++;
    reconnectTimer = setTimeout(connect, delay);
  }

  // --- Manual retry ---
  retryBtn.addEventListener('click', function() {
    if (reconnectTimer) clearTimeout(reconnectTimer);
    reconnectAttempt = 0;   // reset so retry is instant
    connect();
  });

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
    var now = Date.now();
    if (!sendThrottle[motor] || now - sendThrottle[motor].last > 50) {
      sendThrottle[motor] = { last: now };
      sendCmd(motor, speed);
    }
  }

  // --- Slider logic ---
  // For mouse: only one cursor, so we track the active track globally
  var activeTrack = null;

  document.querySelectorAll('.slider-track').forEach(function(track) {
    var motor   = track.getAttribute('data-motor');
    var thumb   = track.querySelector('.slider-thumb');
    var speedEl = document.getElementById('speed-' + motor);

    function posToSpeed(clientY) {
      var rect = track.getBoundingClientRect();
      var frac = 1 - (clientY - rect.top) / rect.height;
      var clamped = Math.max(0, Math.min(1, frac));
      return Math.round(clamped * 510 - 255);           // -255 .. +255
    }

    function updateThumb(speed) {
      var frac = 1 - (speed + 255) / 510;               // +255→top, -255→bottom
      var topPx = frac * (track.clientHeight - thumb.clientHeight);
      thumb.style.top = topPx + 'px';
      speedEl.textContent = speed;
    }

    // Initialise thumb to centre (speed 0)
    updateThumb(0);

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
