#ifndef HTML_PAGES_H
#define HTML_PAGES_H

#include <pgmspace.h>

const char setup_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Wi-Fi Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { box-sizing: border-box; }
        body { margin:0; font-family:'Segoe UI',sans-serif; background:linear-gradient(135deg,#1f4037,#99f2c8); display:flex; justify-content:center; align-items:center; min-height:100vh; }
        .card { background:white; padding:30px; width:90%; max-width:380px; border-radius:20px; box-shadow:0 10px 40px rgba(0,0,0,0.3); text-align:center; }
        h2 { margin-top:0; color:#1f4037; }
        input { width:100%; padding:12px; margin:10px 0; border-radius:10px; border:1px solid #ccc; }
        button { width:100%; padding:12px; border-radius:10px; border:none; background:linear-gradient(90deg,#00c6ff,#0072ff); color:white; font-weight:bold; cursor:pointer; }
    </style>
</head>
<body>
<div class="card">
    <h2>Change Wi-Fi</h2>
    <form action="/save" method="get">
        <input type="text" name="ssid" placeholder="Wi-Fi Name (SSID)" required>
        <input type="password" name="pass" placeholder="Password">
        <button type="submit">Save & Reconnect</button>
    </form>
    <a class="back" href="/">Back to Dashboard</a>
</div>
</body>
</html>
)rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Plant Dashboard</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
    <style>
        * { box-sizing: border-box; }
        body { margin:0; font-family:Arial,sans-serif; background:linear-gradient(135deg,#0f2027,#2c5364); color:white; text-align:center; overflow-x:hidden; }
        .container { width:95%; max-width:900px; margin:auto; padding-top:30px; padding-bottom:40px; }
        .header-card { background:rgba(255,255,255,0.1); padding:15px; border-radius:15px; margin-bottom:20px; }
        
        @keyframes flashRed { 0%{box-shadow:0 0 10px #ff4757;} 50%{box-shadow:0 0 30px #ff4757;} 100%{box-shadow:0 0 10px #ff4757;} }
        .cube.alarm-active { animation:flashRed 1.5s infinite; border-color:#ff4757; }
        
        .global-alarm-banner { display:none; background:#ff4757; color:white; padding:12px; margin-bottom:20px; border-radius:10px; font-weight:bold; font-size:1.2em; }
        
        .cubes-grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(200px,1fr)); gap:20px; }
        .cube { background:rgba(0,0,0,0.3); border-radius:15px; padding:20px; cursor:pointer; transition:0.2s; border:2px solid transparent; }
        .cube:hover { transform:translateY(-5px); background:rgba(0,198,255,0.15); }
        .cube h3 { margin:0 0 10px 0; color:#00c6ff; }
        .warning { color:#ff6b6b; font-weight:bold; }
        
        .modal-overlay { display:none; position:fixed; top:0; left:0; width:100%; height:100%; background:rgba(0,0,0,0.85); z-index:1000; overflow-y:auto; }
        .modal-content { background:#1e3c4a; width:95%; max-width:820px; margin:40px auto; padding:25px; border-radius:15px; position:relative; }
        .close-btn { position:absolute; top:15px; right:25px; font-size:32px; cursor:pointer; color:#fff; }
        
        .sensor-settings { display:grid; gap:12px; margin-top:15px; }
        .setting-row { display:flex; justify-content:space-between; align-items:center; background:rgba(0,0,0,0.25); padding:12px; border-radius:8px; }
        .setting-row input { width:85px; padding:6px; border-radius:5px; border:none; text-align:right; font-size:1em; }
        .chart-wrap { position:relative; height:340px; margin:25px 0; background:rgba(0,0,0,0.2); border-radius:10px; padding:10px; }
        .action-btn { padding:8px 16px; border-radius:8px; border:none; background:#00c6ff; color:white; font-weight:bold; cursor:pointer; }
    </style>
</head>
<body>
<div class="container">
    <div class="header-card">
        <h2>Smart Plant Network</h2>
        <div style="font-size:14px;">Gateway IP: %STA_IP% | Status: %WIFI_STATUS%</div>
        <a href="/setup" style="color:#00ffcc;font-size:12px;display:block;margin-top:8px;">Configure Wi-Fi</a>
    </div>

    <div id="global-alarm-banner" class="global-alarm-banner"> ALARM TRIGGERED ON ONE OR MORE DEVICES! </div>
    <div class="cubes-grid" id="cubes-container"></div>
</div>

<div class="modal-overlay" id="device-modal">
    <div class="modal-content">
        <span class="close-btn" onclick="closeModal()">×</span>
        <h2 id="modal-title">Device Details</h2>
        <p id="modal-ip" style="color:#00c6ff;margin-top:-8px;"></p>

        <div id="alarm-controls" style="margin:15px 0 20px 0;"></div>
        
        <div class="chart-wrap"><canvas id="deviceChart"></canvas></div>

        <h3>Device Settings</h3>
        <div id="device-delay-container"></div>

        <h3 style="margin-top:20px;">Sensor Thresholds</h3>
        <div class="sensor-settings" id="sensor-settings"></div>
    </div>
</div>

<script>
var systemData = { devices: [] };
var PALETTE = ["#00ffcc","#00c6ff","#ff6b6b","#ffd166","#a29bfe"];
var myChart = null;
var activeDeviceId = null;
var modalOpen = false;
var modalUiBuiltFor = null;

function latestFromSeries(data) {
    if (!data || !data.length) return null;
    var v = data[data.length - 1];
    return (v === null || v === undefined) ? null : v;
}

function renderCubes() {
    const container = document.getElementById("cubes-container");
    container.innerHTML = "";
    let anyAlarm = false;

    if (!systemData.devices || systemData.devices.length === 0) {
        container.innerHTML = "<p style='color:#aaa;'>Waiting for clients...</p>";
        document.getElementById("global-alarm-banner").style.display = "none";
        return;
    }

    systemData.devices.forEach(dev => {
        if (dev.alarm) anyAlarm = true;
        const cube = document.createElement("div");
        cube.className = `cube ${dev.alarm ? "alarm-active" : ""}`;
        cube.onclick = () => openModal(dev.id);

        let html = `<h3>Device ${dev.id}</h3><div class="cube-summary">`;
        dev.sensors.forEach(s => {
            const latest = s.data ? s.data[s.data.length - 1] : null;
            const valStr = (latest !== null) ? latest.toFixed(1) : "--";
            const warn = (s.max && latest !== null && latest > s.max) ? "warning" : "";
            html += `<div>${s.name}: <span class="${warn}">${valStr}</span>`;
            if (s.max) html += ` <span style="font-size:10px;opacity:0.6;">(Max:${s.max})</span>`;
            html += `</div>`;
        });
        html += `</div>`;
        cube.innerHTML = html;
        container.appendChild(cube);
    });

    document.getElementById("global-alarm-banner").style.display = anyAlarm ? "block" : "none";
}

function openModal(devId) {
    activeDeviceId = devId;
    modalOpen = true;
    updateModalContent();
    document.getElementById("device-modal").style.display = "block";
}

function buildDelayControls(device) {
    const delayDiv = document.getElementById("device-delay-container");
    delayDiv.innerHTML = "";
    const wrap = document.createElement("div");
    wrap.className = "setting-row";
    const left = document.createElement("div");
    const strong = document.createElement("strong");
    strong.textContent = "Update Rate (ms)";
    const cur = document.createElement("div");
    cur.id = "current_delay";
    cur.style.cssText = "font-size:12px;opacity:0.7;margin-top:4px;";
    left.appendChild(strong);
    left.appendChild(cur);
    const right = document.createElement("div");
    const input = document.createElement("input");
    input.type = "number";
    input.id = "delay_input";
    input.step = "500";
    input.min = "1000";
    input.placeholder = "New Rate";
    input.style.width = "90px";
    const btn = document.createElement("button");
    btn.className = "action-btn";
    btn.textContent = "Set";
    btn.onclick = function () { saveDelay(activeDeviceId); };
    right.appendChild(input);
    right.appendChild(btn);
    wrap.appendChild(left);
    wrap.appendChild(right);
    delayDiv.appendChild(wrap);
    cur.textContent = "(Current: " + (device.delay || 5000) + "ms)";
}

function buildSensorRows(device) {
    const settingsDiv = document.getElementById("sensor-settings");
    settingsDiv.innerHTML = "";
    device.sensors.forEach(function (s, idx) {
        const row = document.createElement("div");
        row.className = "setting-row";
        const left = document.createElement("div");
        const nameEl = document.createElement("strong");
        nameEl.textContent = s.name;
        const valSpan = document.createElement("span");
        valSpan.id = "val_" + idx;
        valSpan.style.marginLeft = "6px";
        const curMax = document.createElement("div");
        curMax.id = "cur_max_" + idx;
        curMax.style.cssText = "font-size:12px;opacity:0.7;margin-top:4px;";
        left.appendChild(nameEl);
        left.appendChild(document.createTextNode(" "));
        left.appendChild(valSpan);
        left.appendChild(curMax);
        const right = document.createElement("div");
        const input = document.createElement("input");
        input.type = "number";
        input.id = "max_" + idx;
        input.step = "0.1";
        input.placeholder = "New Max";
        input.style.width = "90px";
        const btn = document.createElement("button");
        btn.className = "action-btn";
        btn.textContent = "Save";
        btn.onclick = function () { saveThreshold(activeDeviceId, idx); };
        right.appendChild(input);
        right.appendChild(btn);
        row.appendChild(left);
        row.appendChild(right);
        settingsDiv.appendChild(row);
    });
}

function updateModalContent() {
    const device = systemData.devices.find(d => d.id === activeDeviceId);
    if (!device) {
        if (modalOpen) closeModal();
        return;
    }

    if (modalUiBuiltFor !== activeDeviceId) {
        modalUiBuiltFor = activeDeviceId;
        if (myChart) { myChart.destroy(); myChart = null; }
        buildDelayControls(device);
        buildSensorRows(device);
    } else {
        const settingsDiv = document.getElementById("sensor-settings");
        if (settingsDiv.children.length !== device.sensors.length) {
            if (myChart) { myChart.destroy(); myChart = null; }
            buildSensorRows(device);
        }
    }

    document.getElementById("modal-title").textContent = "Device " + device.id;
    document.getElementById("modal-ip").textContent = "IP: " + (device.ip || "Unknown");

    const alarmDiv = document.getElementById("alarm-controls");
    alarmDiv.innerHTML = "";
    if (device.alarm) {
        const btn = document.createElement("button");
        btn.className = "action-btn";
        btn.style.cssText = "background:#ff4757;width:100%;padding:14px;font-size:17px;";
        btn.textContent = "Turn Off Alarm";
        btn.onclick = function () { clearAlarm(activeDeviceId); };
        alarmDiv.appendChild(btn);
    }

    const curDelayEl = document.getElementById("current_delay");
    if (curDelayEl) {
        curDelayEl.textContent = "(Current: " + (device.delay || 5000) + "ms)";
    }

    device.sensors.forEach(function (s, idx) {
        const latest = latestFromSeries(s.data);
        const valEl = document.getElementById("val_" + idx);
        if (valEl) {
            valEl.textContent = (latest !== null) ? latest.toFixed(1) : "--";
            valEl.style.color = (s.max && latest !== null && latest > s.max) ? "#ff6b6b" : "#00ffcc";
        }
        const curMaxEl = document.getElementById("cur_max_" + idx);
        if (curMaxEl) {
            curMaxEl.textContent = s.max ? ("(Current Max: " + s.max + ")") : "(No Max Set)";
        }
    });

    updateChart(device);
}

function updateChart(device) {
    if (myChart && myChart.data.datasets.length !== device.sensors.length) {
        myChart.destroy();
        myChart = null;
    }
    if (!myChart) {
        const ctx = document.getElementById("deviceChart").getContext("2d");
        const datasets = device.sensors.map((sensor, i) => ({
            label: sensor.name,
            data: sensor.data || [],
            borderColor: PALETTE[i % PALETTE.length],
            backgroundColor: PALETTE[i % PALETTE.length] + "33",
            fill: false,
            tension: 0.3,
            spanGaps: true
        }));

        myChart = new Chart(ctx, {
            type: "line",
            data: { labels: device.labels || [], datasets: datasets },
            options: {
                responsive: true, maintainAspectRatio: false,
                plugins: { legend: { labels: { color: "#fff" } } },
                scales: {
                    x: { ticks: { color: "rgba(255,255,255,0.7)" }, grid: { color: "rgba(255,255,255,0.08)" } },
                    y: { ticks: { color: "rgba(255,255,255,0.7)" }, grid: { color: "rgba(255,255,255,0.08)" } }
                }
            }
        });
    } else {
        myChart.data.labels = device.labels || [];
        device.sensors.forEach((sensor, i) => {
            if (myChart.data.datasets[i]) myChart.data.datasets[i].data = sensor.data || [];
        });
        myChart.update("none");
    }
}

function closeModal() {
    modalOpen = false;
    modalUiBuiltFor = null;
    document.getElementById("device-modal").style.display = "none";
    if (myChart) { myChart.destroy(); myChart = null; }
}

function clearAlarm(devId) {
    fetch("/clear_alarm?dev=" + encodeURIComponent(devId)).then(function () { setTimeout(poll, 300); });
}

function saveDelay(devId) {
    const inputEl = document.getElementById("delay_input");
    const val = inputEl.value;
    if (val) {
        fetch("/set_delay?dev=" + encodeURIComponent(devId) + "&delay=" + encodeURIComponent(val)).then(function () {
            alert("Update rate saved!");
            inputEl.value = "";
            poll();
        });
    }
}

function saveThreshold(devId, sensorIndex) {
    const dev = systemData.devices.find(function (d) { return d.id === devId; });
    if (!dev || !dev.sensors[sensorIndex]) return;
    const sensor = dev.sensors[sensorIndex];
    const inputEl = document.getElementById("max_" + sensorIndex);
    const val = inputEl.value;
    fetch("/set_max?dev=" + encodeURIComponent(devId) +
        "&sensor=" + encodeURIComponent(sensor.name) +
        "&val=" + encodeURIComponent(val)).then(function () {
        alert("Threshold saved!");
        inputEl.value = "";
        poll();
    });
}

function poll() {
    fetch("/history")
        .then(r => r.json())
        .then(data => {
            systemData = data;
            renderCubes();
            if (modalOpen && activeDeviceId) updateModalContent();
        })
        .catch(() => {});
}

setInterval(poll, 2000);
poll();
</script>
</body>
</html>
)rawliteral";

#endif