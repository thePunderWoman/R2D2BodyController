// web_page.h — wireless firmware update page + OTA plumbing.
//
// Single self-hosted page (no external CSS/JS/font dependencies — the
// browser is connected to this device's own WiFi AP, which has no internet
// route). Pattern ported from ~/repos/Amidala's web/update.html and
// src/wifi_ap.cpp OTA handlers, trimmed down since Body Controller only
// needs this one page (Amidala's ~18-page setup uses a build-time embed
// script that isn't worth the overhead here).

#ifndef web_page_h
#define web_page_h

#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ESPmDNS.h>

WebServer webServer(80);

static const char UPDATE_PAGE[] PROGMEM = R"HTMLPAGE(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Firmware Update &mdash; Body Controller</title>
<style>
:root{color-scheme:light dark}
body{font-family:ui-monospace,Menlo,Consolas,monospace;max-width:32rem;margin:2rem auto;padding:0 1rem;line-height:1.5}
h1{font-size:1.1rem;letter-spacing:.05em;text-transform:uppercase}
.row{display:flex;justify-content:space-between;padding:.4rem 0;border-bottom:1px solid #8884}
.warn-box{border:1px solid #c59000;background:#c5900022;padding:.9rem 1.1rem;font-size:.85rem;line-height:1.6;margin:1rem 0;border-radius:4px}
.warn-box strong{display:block;margin-bottom:.3rem}
#fw-file{width:100%;margin:.9rem 0}
#flash-btn{background:#c0392b22;color:#c0392b;border:1px solid #c0392b;padding:.5rem 1.4rem;font-size:.8rem;letter-spacing:.1em;text-transform:uppercase;border-radius:4px;cursor:pointer}
#flash-btn:disabled{opacity:.35;cursor:default}
progress{width:100%;height:20px;margin-top:1rem}
.prog-pct{font-size:.8rem;text-align:right;margin-top:.3rem;opacity:.7}
.success-box{border:1px solid #2d7a2d;background:#2d7a2d22;padding:1.2rem;text-align:center;margin-top:1rem;border-radius:6px}
</style>
</head>
<body>
<h1>Body Controller &mdash; Firmware Update</h1>

<div id="form-area">
  <div class="row"><span>Version</span><span id="cur-ver">&mdash;</span></div>
  <div class="row"><span>MCU</span><span id="cur-mcu">&mdash;</span></div>
  <div class="row"><span>Build Date</span><span id="cur-date">&mdash;</span></div>

  <div class="warn-box">
    <strong>&#9888; WARNING</strong>
    This will overwrite the firmware currently running on the droid.
    Do not power off the board while the update is in progress &mdash; this can
    permanently brick it. Only upload a <code>.bin</code> built for this board.
  </div>

  <input type="file" id="fw-file" accept=".bin">
  <br>
  <button id="flash-btn" disabled>Flash Firmware</button>
</div>

<div id="prog-area" hidden>
  <div id="prog-label">Uploading firmware&hellip;</div>
  <progress id="prog" value="0" max="100"></progress>
  <div class="prog-pct" id="prog-pct">0%</div>
</div>

<div id="success-area" hidden>
  <div class="success-box">
    &#10003; Firmware update complete.<br>
    <span id="new-ver"></span>
  </div>
</div>

<script>
fetch('/api/info').then(function(r){return r.json();}).then(function(d){
  document.getElementById('cur-ver').textContent  = 'v' + d.version;
  document.getElementById('cur-mcu').textContent  = d.mcu;
  document.getElementById('cur-date').textContent = d.date;
});

var fileInput   = document.getElementById('fw-file');
var flashBtn    = document.getElementById('flash-btn');
var formArea    = document.getElementById('form-area');
var progArea    = document.getElementById('prog-area');
var successArea = document.getElementById('success-area');
var prog        = document.getElementById('prog');
var progLabel   = document.getElementById('prog-label');
var progPct     = document.getElementById('prog-pct');

fileInput.onchange = function() { flashBtn.disabled = !fileInput.files.length; };

flashBtn.onclick = function() {
  if (!fileInput.files.length) return;
  var ok = window.confirm(
    'WARNING\n\nThis will overwrite the firmware currently running on the droid.\n\n' +
    'Do NOT power off the board during the update.\n\nPress OK to begin flashing.'
  );
  if (!ok) return;
  startUpload();
};

function setPhase(label, pct) {
  progLabel.textContent = label;
  prog.value = pct;
  progPct.textContent = pct + '%';
}

function startUpload() {
  formArea.hidden = true;
  progArea.hidden = false;
  setPhase('Uploading firmware…', 0);

  var xhr = new XMLHttpRequest();
  xhr.upload.onprogress = function(e) {
    if (e.lengthComputable) setPhase('Uploading firmware…', Math.round(e.loaded / e.total * 85));
  };
  xhr.onload = function() {
    if (xhr.status !== 200 || xhr.responseText.trim() !== 'OK') {
      setPhase('Update failed: ' + xhr.responseText.trim(), prog.value);
      return;
    }
    onFlashing();
  };
  xhr.onerror = function() {
    setPhase('Upload failed — check connection and try again.', prog.value);
  };

  var fd = new FormData();
  fd.append('firmware', fileInput.files[0]);
  xhr.open('POST', '/update');
  xhr.send(fd);
}

function onFlashing() {
  setPhase('Writing to flash…', 90);
  setTimeout(function() {
    setPhase('Restarting…', 95);
    pollForRestart();
  }, 3000);
}

function pollForRestart() {
  var attempts = 0;
  var timer = setInterval(function() {
    if (++attempts > 30) {
      clearInterval(timer);
      setPhase('Timed out — check the board manually.', 95);
      return;
    }
    fetch('/api/info', { cache: 'no-store' })
      .then(function(r) { return r.json(); })
      .then(function(d) {
        clearInterval(timer);
        setPhase('Complete', 100);
        setTimeout(function() {
          progArea.hidden = true;
          successArea.hidden = false;
          document.getElementById('new-ver').textContent = 'Now running v' + d.version + ' · built ' + d.date;
        }, 400);
      })
      .catch(function() { /* still restarting */ });
  }, 2000);
}
</script>
</body>
</html>
)HTMLPAGE";

static void handleRoot()
{
  webServer.send_P(200, "text/html", UPDATE_PAGE);
}

static void handleApiInfo()
{
  String json = "{";
  json += "\"version\":\"" FIRMWARE_VERSION "\",";
  json += "\"mcu\":\""     MCU_VARIANT     "\",";
  json += "\"date\":\""    __DATE__        "\",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap());
  json += "}";
  webServer.send(200, "application/json", json);
}

// Uses the standard Arduino ESP32 Update class, which writes to the inactive
// OTA partition (ota_0 / ota_1 — see platformio.ini's min_spiffs.csv) and
// switches the boot slot on completion.
static void handleUpdateUpload()
{
  HTTPUpload& upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    DEBUG_PRINT_LN(F("OTA: upload started"));
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      DEBUG_PRINT_LN(F("OTA: begin failed"));
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      DEBUG_PRINT_LN(F("OTA: write error"));
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      DEBUG_PRINT_LN(F("OTA: flash complete, restarting"));
    } else {
      DEBUG_PRINT_LN(F("OTA: end failed"));
    }
  }
}

static void handleUpdatePost()
{
  if (Update.hasError()) {
    webServer.send(500, "text/plain", "UPDATE FAILED: see serial monitor");
  } else {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/plain", "OK");
    delay(200);
    ESP.restart();
  }
}

// Self-hosted AP (no external router dependency) + mDNS + the three routes
// above. Call once from setup().
static void startOTAWebServer()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(OTA_AP_SSID, OTA_AP_PASSWORD);
  DEBUG_PRINT_LN(F("[WiFi] AP started: " OTA_AP_SSID));

  if (MDNS.begin("bodycontroller")) {
    MDNS.addService("http", "tcp", 80);
    DEBUG_PRINT_LN(F("[WiFi] mDNS: http://bodycontroller.local/"));
  }

  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/api/info", HTTP_GET, handleApiInfo);
  webServer.on("/update", HTTP_POST, handleUpdatePost, handleUpdateUpload);
  webServer.begin();
}

#endif
