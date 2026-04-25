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
        body {
            margin: 0; font-family: 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #1f4037, #99f2c8);
            display: flex; justify-content: center; align-items: center; min-height: 100vh;
        }
        .card { background: white; padding: 30px; width: 90%; max-width: 380px; border-radius: 20px; box-shadow: 0 10px 40px rgba(0,0,0,0.3); text-align: center; }
        h2 { margin-top: 0; color: #1f4037; }
        input { width: 100%; padding: 12px; margin: 10px 0; border-radius: 10px; border: 1px solid #ccc; font-size: 1em; }
        button { width: 100%; padding: 12px; border-radius: 10px; border: none; background: linear-gradient(90deg, #00c6ff, #0072ff); color: white; font-weight: bold; font-size: 1em; cursor: pointer; transition: 0.3s; }
        button:hover { opacity: 0.85; }
        .back { display: block; margin-top: 15px; font-size: 0.85em; color: #555; text-decoration: none; }
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
        body {
            margin: 0; font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #0f2027, #2c5364);
            color: white; text-align: center; overflow-x: hidden;
        }
        .container { width: 95%; max-width: 900px; margin: auto; padding-top: 30px; padding-bottom: 40px; }
        .header-card { background: rgba(255,255,255,0.1); padding: 15px; border-radius: 15px; margin-bottom: 20px; }
        
        /* Cubes Grid */
        .cubes-grid {
            display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px;
        }
        .cube {
            background: rgba(0,0,0,0.3); border-radius: 15px; padding: 20px;
            cursor: pointer; transition: transform 0.2s, background 0.2s;
            border: 2px solid transparent; box-shadow: 0 4px 15px rgba(0,0,0,0.2);
        }
        .cube:hover { transform: translateY(-5px); background: rgba(0,198,255,0.15); }
        .cube h3 { margin: 0 0 10px 0; color: #00c6ff; }
        .cube-summary { font-size: 14px; color: #ccc; text-align: left; }
        .warning { color: #ff6b6b; font-weight: bold; }

        /* Expanded Modal */
        .modal-overlay {
            display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%;
            background: rgba(0,0,0,0.8); z-index: 1000; overflow-y: auto;
        }
        .modal-content {
            background: #1e3c4a; width: 95%; max-width: 800px; margin: 50px auto;
            padding: 20px; border-radius: 15px; position: relative; text-align: left;
        }
        .close-btn {
            position: absolute; top: 15px; right: 20px; color: white;
            font-size: 24px; cursor: pointer; font-weight: bold;
        }
        
        .sensor-settings { display: grid; grid-template-columns: 1fr; gap: 10px; margin-top: 15px; }
        .setting-row {
            display: flex; justify-content: space-between; align-items: center;
            background: rgba(0,0,0,0.2); padding: 10px; border-radius: 8px;
        }
        .setting-row input {
            width: 80px; padding: 5px; border-radius: 5px; border: none; text-align: right;
        }
        .chart-wrap { position: relative; height: 300px; margin-top: 20px; background: rgba(0,0,0,0.2); border-radius: 10px; padding: 10px;}
        
        button.action-btn {
            padding: 8px 15px; border-radius: 8px; border: none;
            background: #00c6ff; color: white; font-weight: bold; cursor: pointer; margin-top: 15px;
        }
    </style>
</head>
<body>
<div class="container">
    <div class="header-card">
        <h2>Smart Plant Network</h2>
        <div style="font-size: 14px;">Gateway IP: %STA_IP% | Status: %WIFI_STATUS%</div>
        <a href="/setup" style="color:#00ffcc; font-size:12px; display:block; margin-top:10px;">Configure Wi-Fi</a>
    </div>

    <div class="cubes-grid" id="cubes-container"></div>
</div>

<div class="modal-overlay" id="device-modal">
    <div class="modal-content">
        <span class="close-btn" onclick="closeModal()">&times;</span>
        <h2 id="modal-title">Device Details</h2>
        <p id="modal-ip" style="color:#00c6ff; margin-top:-10px; font-size:14px;"></p>

        <div class="chart-wrap">
            <canvas id="deviceChart"></canvas>
        </div>

        <h3 style="margin-top: 25px;">Device Settings</h3>
        <div id="device-delay-container"></div>

        <h3 style="margin-top: 20px;">Sensor Thresholds</h3>
        <div class="sensor-settings" id="sensor-settings"></div>
    </div>
</div>

<script>
var systemData = { devices: [] };
var PALETTE = ["#00ffcc","#00c6ff","#ff6b6b","#ffd166","#a29bfe"];
var myChart = null;
var activeDeviceId = null;

function renderCubes() {
    var container = document.getElementById("cubes-container");
    container.innerHTML = "";

    if(!systemData.devices || systemData.devices.length === 0) {
        container.innerHTML = "<p>Waiting for clients to connect...</p>";
        return;
    }

    systemData.devices.forEach(device => {
        var div = document.createElement("div");
        div.className = "cube";
        div.onclick = () => openModal(device.id);
        
        var html = "<h3>Device " + device.id + "</h3>";
        html += "<div class='cube-summary'>";
        
        device.sensors.forEach(sensor => {
            var latestVal = sensor.data[sensor.data.length - 1];
            var valStr = (latestVal !== null && latestVal !== undefined) ? latestVal.toFixed(1) : "--";
            var warningClass = (sensor.max && latestVal > sensor.max) ? "warning" : "";
            
            html += "<div>" + sensor.name + ": <span class='" + warningClass + "'>" + valStr + "</span>";
            if(sensor.max) html += " <span style='font-size:10px; opacity:0.6;'>(Max: " + sensor.max + ")</span>";
            html += "</div>";
        });
        
        html += "</div>";
        div.innerHTML = html;
        container.appendChild(div);
    });
}

function openModal(devId) {
    activeDeviceId = devId;
    var device = systemData.devices.find(d => d.id === devId);
    if(!device) return;

    document.getElementById("modal-title").textContent = "Device " + device.id;
    document.getElementById("modal-ip").textContent = "IP: " + (device.ip || "Unknown");
    
    // Inject Device Delay Input
    var currentDelay = device.delay || 5000;
    var delayContainer = document.getElementById("device-delay-container");
    delayContainer.innerHTML = `
        <div class="setting-row">
            <div><strong>Update Rate (ms)</strong></div>
            <div>
                <input type="number" id="delay_${devId}" value="${currentDelay}" step="500" min="1000">
                <button class="action-btn" style="margin-top:0; padding:5px 10px;" onclick="saveDelay('${devId}')">Set</button>
            </div>
        </div>
    `;

    // Inject Sensor Thresholds
    var settingsDiv = document.getElementById("sensor-settings");
    settingsDiv.innerHTML = "";
    
    device.sensors.forEach(sensor => {
        var currentMax = sensor.max || "";
        var latestVal = sensor.data[sensor.data.length - 1] || 0;
        var isExceeded = currentMax && (latestVal > currentMax);
        
        var row = document.createElement("div");
        row.className = "setting-row";
        row.innerHTML = `
            <div>
                <strong>${sensor.name}</strong> 
                <span style="margin-left:10px; color:${isExceeded ? '#ff6b6b' : '#00ffcc'}">${latestVal.toFixed(1)}</span>
            </div>
            <div>
                <label>Max: </label>
                <input type="number" id="max_${devId}_${sensor.name}" value="${currentMax}" step="0.1">
                <button class="action-btn" style="margin-top:0; padding:5px 10px;" onclick="saveThreshold('${devId}', '${sensor.name}')">Save</button>
            </div>
        `;
        settingsDiv.appendChild(row);
    });

    renderChart(device);
    document.getElementById("device-modal").style.display = "block";
}

function closeModal() {
    document.getElementById("device-modal").style.display = "none";
    activeDeviceId = null;
}

function saveDelay(devId) {
    var inputVal = document.getElementById(`delay_${devId}`).value;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", `/set_delay?dev=${devId}&delay=${inputVal}`, true);
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4 && xhr.status === 200) {
            alert("Update rate saved for Device " + devId + "!");
            poll(); 
        }
    };
    xhr.send();
}

function saveThreshold(devId, sensorName) {
    var inputVal = document.getElementById(`max_${devId}_${sensorName}`).value;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", `/set_max?dev=${devId}&sensor=${sensorName}&val=${inputVal}`, true);
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4 && xhr.status === 200) {
            alert("Threshold saved!");
            poll(); 
        }
    };
    xhr.send();
}

function renderChart(device) {
    var ctx = document.getElementById("deviceChart").getContext("2d");
    if (myChart) { myChart.destroy(); }

    var datasets = device.sensors.map((sensor, idx) => {
        return {
            label: sensor.name,
            data: sensor.data,
            borderColor: PALETTE[idx % PALETTE.length],
            backgroundColor: PALETTE[idx % PALETTE.length] + "33",
            fill: false,
            tension: 0.3,
            spanGaps: true
        };
    });

    myChart = new Chart(ctx, {
        type: "line",
        data: { labels: device.labels, datasets: datasets },
        options: {
            responsive: true, maintainAspectRatio: false,
            plugins: { legend: { labels: { color: "#fff" } } },
            scales: {
                x: { ticks: { color: "rgba(255,255,255,0.6)" }, grid: { color: "rgba(255,255,255,0.08)" } },
                y: { ticks: { color: "rgba(255,255,255,0.6)" }, grid: { color: "rgba(255,255,255,0.08)" } }
            }
        }
    });
}

function poll() {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/history", true); 
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4 && xhr.status === 200) {
            try {
                systemData = JSON.parse(xhr.responseText);
                renderCubes();
                if(activeDeviceId) {
                    var device = systemData.devices.find(d => d.id === activeDeviceId);
                    if(device) renderChart(device);
                }
            } catch(e) { console.error("Data parse error."); }
        }
    };
    xhr.send();
}

setInterval(poll, 2000);
poll();
</script>
</body>
</html>
)rawliteral";

#endif // HTML_PAGES_H