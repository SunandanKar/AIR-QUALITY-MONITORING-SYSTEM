#include <WiFi.h>
#include <WebServer.h>

// WiFi Credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

WebServer server(80);

String inputString = "";
bool stringComplete = false;

// Sensor data variables
String tempData = "N/A";
String humData = "N/A";
String mq135Data = "N/A";
String mq4Data = "N/A";
String pm10Data = "--";
String pm25Data = "--";

// Time/Date variables
String currentTime = "";
String currentDate = "";
bool hasTimeData = false;
bool hasDateData = false;

// Data logging variables
const int MAX_LOG_ENTRIES = 10;
String dateLog[MAX_LOG_ENTRIES];
String timeLog[MAX_LOG_ENTRIES];
String aqiLog[MAX_LOG_ENTRIES];
String tempLog[MAX_LOG_ENTRIES];
String methaneLog[MAX_LOG_ENTRIES];
String humLog[MAX_LOG_ENTRIES];
String pm10Log[MAX_LOG_ENTRIES];
String pm25Log[MAX_LOG_ENTRIES];
int currentLogIndex = 0;
unsigned long lastLogTime = 0;

// Function declarations
String getAqiColor(String aqiValue) {
  if (aqiValue == "N/A" || aqiValue == "--") return "#94a3b8";
  
  int value = aqiValue.toInt();
  if (value <= 50) return "#2ecc71"; // Green (Good)
  else if (value <= 100) return "#f39c12"; // Orange (Moderate)
  else return "#e74c3c"; // Red (Unhealthy)
}

String getAqiStatus(String aqiValue) {
  if (aqiValue == "N/A" || aqiValue == "--") return "No Data";
  
  int value = aqiValue.toInt();
  if (value <= 50) return "Good";
  else if (value <= 100) return "Moderate";
  else return "Unhealthy";
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  // Initialize log arrays
  for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
    dateLog[i] = "";
    timeLog[i] = "";
    aqiLog[i] = "";
    tempLog[i] = "";
    humLog[i] = "";
    methaneLog[i] = "";
    pm10Log[i] = "";
    pm25Log[i] = "";
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void parseSensorData(String dataString) {
  Serial.println("Parsing sensor string: " + dataString);

  int tIndex = dataString.indexOf("Temp:");
  int hIndex = dataString.indexOf("Humidity:");
  int m135Index = dataString.indexOf("AQI (MQ135):");
  int m4Index = dataString.indexOf("Methane (MQ4 ppm):");

  if (tIndex == -1 || hIndex == -1 || m135Index == -1 || m4Index == -1) {
    Serial.println("Parsing failed: One or more keywords not found.");
    return;
  }

  tempData = dataString.substring(tIndex + 5, hIndex);
  tempData.trim();
  tempData.replace("°C", "");
  tempData.replace("°", "");
  tempData.replace(",", "");
  tempData.replace("Â", "");
  if (tempData.length() == 0) tempData = "N/A";

  humData = dataString.substring(hIndex + 8, m135Index);
  humData.trim();
  humData.replace("%", "");
  humData.replace(",", "");
  humData.replace(":", "");
  if (humData.length() == 0) humData = "N/A";

  mq135Data = dataString.substring(m135Index + 11, m4Index);
  mq135Data.trim();
  mq135Data.replace(",", "");
  mq135Data.replace(":", "");
  if (mq135Data.length() == 0) mq135Data = "N/A";

  mq4Data = dataString.substring(m4Index + 17);
  mq4Data.trim();
  mq4Data.replace(",", "");
  mq4Data.replace(":", "");
  if (mq4Data.length() == 0) mq4Data = "N/A";

  // Log data every second
  if (millis() - lastLogTime >= 1000) {
    lastLogTime = millis();
    
    // Store date and time separately
    if (hasTimeData && hasDateData) {
      dateLog[currentLogIndex] = currentDate;
      timeLog[currentLogIndex] = currentTime;
    } else {
      // Fallback to uptime
      unsigned long seconds = millis() / 1000;
      int hours = seconds / 3600;
      int minutes = (seconds % 3600) / 60;
      int secs = seconds % 60;
      dateLog[currentLogIndex] = "Uptime";
      timeLog[currentLogIndex] = String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes) + ":" + (secs < 10 ? "0" : "") + String(secs);
    }
    
    aqiLog[currentLogIndex] = mq135Data;
    tempLog[currentLogIndex] = tempData;
    humLog[currentLogIndex] = humData;
    methaneLog[currentLogIndex] = mq4Data;
    pm10Log[currentLogIndex] = pm10Data;
    pm25Log[currentLogIndex] = pm25Data;
    
    currentLogIndex = (currentLogIndex + 1) % MAX_LOG_ENTRIES;
    
    // Reset time/date flags
    hasTimeData = false;
    hasDateData = false;
  }

  Serial.println("Parsed values:");
  Serial.println("Date: " + currentDate + " Time: " + currentTime);
  Serial.println("Temp: " + tempData);
  Serial.println("Humidity: " + humData);
  Serial.println("MQ135: " + mq135Data);
  Serial.println("MQ4: " + mq4Data);
}

String generateDataTable() {
  String table = "<div class='data-table-container'>";
  table += "<h3>Historical Data (Last 10 Readings)</h3>";
  table += "<table class='data-table'>";
  table += "<thead><tr>";
  table += "<th>Date</th><th>Time</th><th>AQI</th><th>Status</th><th>Temp (&deg;C)</th><th>Humidity (%)</th><th>Methane (ppm)</th><th>PM10</th><th>PM2.5</th>";
  table += "</tr></thead><tbody>";

  // Display last 10 entries in reverse chronological order (newest first)
  for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
    int index = (currentLogIndex - 1 - i + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
    if (dateLog[index] == "") continue;
    
    String aqiStatus = getAqiStatus(aqiLog[index]);
    String aqiColor = getAqiColor(aqiLog[index]);
    
    table += "<tr>";
    table += "<td class='centered'>" + dateLog[index] + "</td>";
    table += "<td class='centered'>" + timeLog[index] + "</td>";
    table += "<td class='centered' style='color:" + aqiColor + "'>" + aqiLog[index] + "</td>";
    table += "<td class='centered'><span class='status-badge' style='background:" + aqiColor + "'>" + aqiStatus + "</span></td>";
    table += "<td class='centered' style='color:#f1c40f'>" + tempLog[index] + "</td>";
    table += "<td class='centered' style='color:#3498db'>" + humLog[index] + "</td>";
    table += "<td class='centered' style='color:#2ecc71'>" + methaneLog[index] + "</td>";
    table += "<td class='centered' style='color:#9b59b6'>" + pm10Log[index] + "</td>";
    table += "<td class='centered' style='color:#e91e63'>" + pm25Log[index] + "</td>";
    table += "</tr>";
  }

  table += "</tbody></table></div>";
  return table;
}

void handleRoot() {
  String aqiColor = getAqiColor(mq135Data);
  String aqiStatus = getAqiStatus(mq135Data);
  
  // The HTML page remains exactly the same as your original code
  String htmlPage = 
    "<!DOCTYPE html><html><head><title>AQMS Dashboard</title>"
    "<meta http-equiv='refresh' content='5'>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<link href='https://fonts.googleapis.com/css2?family=Orbitron:wght@500;700&family=Rajdhani:wght@500;600&family=Montserrat:wght@400;600&display=swap' rel='stylesheet'>"
    "<style>"
      ":root {"
        "--primary: #00f7ff;"
        "--primary-dark: #00c4cc;"
        "--dark: #0f172a;"
        "--darker: #020617;"
        "--light: #e2e8f0;"
        "--lighter: #f8fafc;"
        "--card-bg: rgba(2, 6, 23, 0.7);"
        "--card-border: rgba(0, 247, 255, 0.3);"
        "--good: #2ecc71;"
        "--moderate: #f39c12;"
        "--unhealthy: #e74c3c;"
      "}"
      "* {"
        "box-sizing: border-box;"
        "margin: 0;"
        "padding: 0;"
      "}"
      "body {"
        "font-family: 'Rajdhani', sans-serif;"
        "background: var(--darker);"
        "background-image: radial-gradient(circle at 50% 50%, rgba(0, 247, 255, 0.05) 0%, transparent 70%);"
        "color: var(--light);"
        "min-height: 100vh;"
        "padding: 20px;"
        "display: flex;"
        "flex-direction: column;"
        "align-items: center;"
        "position: relative;"
        "overflow-x: hidden;"
      "}"
      "body::before {"
        "content: '';"
        "position: absolute;"
        "top: 0;"
        "left: 0;"
        "right: 0;"
        "bottom: 0;"
        "background-image: url('data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\" viewBox=\"0 0 100 100\"><circle cx=\"50\" cy=\"50\" r=\"1\" fill=\"rgba(0, 247, 255, 0.1)\"/></svg>');"
        "background-size: 20px 20px;"
        "opacity: 0.3;"
        "z-index: -1;"
      "}"
      ".header {"
        "text-align: center;"
        "margin-bottom: 30px;"
        "width: 100%;"
        "position: relative;"
      "}"
      ".header h1 {"
        "font-family: 'Orbitron', sans-serif;"
        "font-size: 2.8rem;"
        "margin: 0;"
        "background: linear-gradient(to right, var(--primary), #00a8ff);"
        "-webkit-background-clip: text;"
        "-webkit-text-fill-color: transparent;"
        "letter-spacing: 1px;"
        "position: relative;"
        "padding-bottom: 15px;"
        "text-shadow: 0 0 10px rgba(0, 247, 255, 0.3);"
      "}"
      ".header h1::after {"
        "content: '';"
        "position: absolute;"
        "bottom: 0;"
        "left: 50%;"
        "transform: translateX(-50%);"
        "width: 200px;"
        "height: 3px;"
        "background: linear-gradient(to right, transparent, var(--primary), transparent);"
      "}"
      ".header p {"
        "font-size: 1.2rem;"
        "color: #94a3b8;"
        "margin-top: 15px;"
        "letter-spacing: 0.5px;"
      "}"
      ".metrics-grid {"
        "display: grid;"
        "grid-template-columns: repeat(3, 1fr);"
        "grid-template-rows: repeat(2, 1fr);"
        "gap: 20px;"
        "width: 100%;"
        "max-width: 1200px;"
        "margin-bottom: 30px;"
      "}"
      ".metric-card {"
        "background: var(--card-bg);"
        "border-radius: 15px;"
        "padding: 25px;"
        "border: 1px solid var(--card-border);"
        "transition: all 0.3s ease;"
        "box-shadow: 0 0 15px rgba(0, 247, 255, 0.1);"
        "display: flex;"
        "flex-direction: column;"
        "align-items: center;"
        "justify-content: center;"
        "text-align: center;"
        "min-height: 200px;"
        "position: relative;"
        "overflow: hidden;"
      "}"
      ".metric-card::before {"
        "content: '';"
        "position: absolute;"
        "top: -50%;"
        "left: -50%;"
        "width: 200%;"
        "height: 200%;"
        "background: linear-gradient(transparent, rgba(0, 247, 255, 0.1), transparent);"
        "transform: rotate(45deg);"
        "transition: all 0.6s ease;"
        "opacity: 0;"
      "}"
      ".metric-card:hover {"
        "transform: translateY(-5px);"
        "box-shadow: 0 0 25px rgba(0, 247, 255, 0.2);"
      "}"
      ".metric-card:hover::before {"
        "opacity: 1;"
        "animation: shine 1.5s;"
      "}"
      ".metric-title {"
        "font-size: 1.3rem;"
        "margin-bottom: 15px;"
        "color: var(--light);"
        "font-weight: 600;"
        "letter-spacing: 0.5px;"
      "}"
      ".metric-value {"
        "font-size: 2.8rem;"
        "font-weight: 600;"
        "margin-bottom: 10px;"
        "font-family: 'Orbitron', sans-serif;"
        "letter-spacing: 1px;"
      "}"
      ".unit {"
        "font-size: 1.2rem;"
        "opacity: 0.8;"
        "margin-left: 5px;"
      "}"
      ".metric-status {"
        "color: #94a3b8;"
        "font-size: 0.9rem;"
        "font-family: 'Montserrat', sans-serif;"
      "}"
      ".data-table-container {"
        "width: 100%;"
        "max-width: 1200px;"
        "margin-bottom: 30px;"
        "background: var(--card-bg);"
        "border-radius: 15px;"
        "padding: 20px;"
        "border: 1px solid var(--card-border);"
        "box-shadow: 0 0 15px rgba(0, 247, 255, 0.1);"
      "}"
      ".data-table-container h3 {"
        "color: var(--primary);"
        "margin-bottom: 20px;"
        "font-family: 'Orbitron', sans-serif;"
        "letter-spacing: 1px;"
        "font-size: 1.5rem;"
        "text-align: center;"
      "}"
      ".data-table {"
        "width: 100%;"
        "border-collapse: collapse;"
        "background: rgba(15, 23, 42, 0.5);"
        "border-radius: 10px;"
        "overflow: hidden;"
        "border: 1px solid rgba(0, 247, 255, 0.2);"
      "}"
      ".data-table th {"
        "background: rgba(0, 247, 255, 0.1);"
        "padding: 15px;"
        "text-align: center;"
        "font-weight: 600;"
        "color: var(--primary);"
        "font-family: 'Orbitron', sans-serif;"
        "letter-spacing: 1px;"
        "font-size: 0.9rem;"
        "text-transform: uppercase;"
      "}"
      ".data-table td {"
        "padding: 12px 15px;"
        "border-bottom: 1px solid rgba(0, 247, 255, 0.1);"
        "text-align: center;"
        "font-family: 'Montserrat', sans-serif;"
        "font-size: 0.95rem;"
      "}"
      ".centered {"
        "text-align: center !important;"
      "}"
      ".data-table tr:last-child td {"
        "border-bottom: none;"
      "}"
      ".data-table tr:hover {"
        "background: rgba(0, 247, 255, 0.05);"
      "}"
      ".status-badge {"
        "display: inline-block;"
        "padding: 4px 10px;"
        "border-radius: 20px;"
        "font-size: 0.8rem;"
        "font-weight: 600;"
        "color: white;"
        "text-transform: uppercase;"
        "letter-spacing: 0.5px;"
        "box-shadow: 0 2px 5px rgba(0,0,0,0.2);"
      "}"
      ".refresh-btn {"
        "background: transparent;"
        "color: var(--primary);"
        "border: 2px solid var(--primary);"
        "padding: 12px 30px;"
        "font-size: 1rem;"
        "border-radius: 30px;"
        "cursor: pointer;"
        "transition: all 0.3s ease;"
        "margin: 20px 0 40px;"
        "font-weight: 600;"
        "letter-spacing: 0.5px;"
        "font-family: 'Orbitron', sans-serif;"
        "position: relative;"
        "overflow: hidden;"
        "z-index: 1;"
      "}"
      ".refresh-btn::before {"
        "content: '';"
        "position: absolute;"
        "top: 0;"
        "left: 0;"
        "width: 0;"
        "height: 100%;"
        "background: linear-gradient(90deg, transparent, rgba(0, 247, 255, 0.2), transparent);"
        "transition: all 0.6s ease;"
        "z-index: -1;"
      "}"
      ".refresh-btn:hover {"
        "background: rgba(0, 247, 255, 0.1);"
        "box-shadow: 0 0 15px rgba(0, 247, 255, 0.3);"
      "}"
      ".refresh-btn:hover::before {"
        "width: 100%;"
      "}"
      ".credits {"
        "margin-top: 40px;"
        "text-align: center;"
        "opacity: 0.8;"
        "font-size: 0.9rem;"
        "width: 100%;"
        "max-width: 800px;"
        "background: var(--card-bg);"
        "border-radius: 15px;"
        "padding: 25px;"
        "border: 1px solid var(--card-border);"
      "}"
      ".credits-title {"
        "font-size: 1.1rem;"
        "margin-bottom: 15px;"
        "color: var(--primary);"
        "font-family: 'Orbitron', sans-serif;"
        "letter-spacing: 1px;"
      "}"
      ".team {"
        "display: flex;"
        "justify-content: center;"
        "gap: 30px;"
        "flex-wrap: wrap;"
        "margin-top: 20px;"
      "}"
      ".team-member {"
        "background: rgba(15, 23, 42, 0.5);"
        "padding: 15px 25px;"
        "border-radius: 10px;"
        "border: 1px solid rgba(0, 247, 255, 0.3);"
        "transition: all 0.3s ease;"
        "min-width: 180px;"
      "}"
      ".team-member:hover {"
        "transform: translateY(-3px);"
        "box-shadow: 0 5px 15px rgba(0, 247, 255, 0.1);"
      "}"
      ".team-member strong {"
        "display: block;"
        "font-size: 1.1rem;"
        "margin-bottom: 5px;"
        "color: var(--light);"
        "font-weight: 600;"
      "}"
      ".role {"
        "color: #94a3b8;"
        "font-size: 0.8rem;"
        "font-family: 'Montserrat', sans-serif;"
      "}"
      ".glow-tag {"
        "font-size: 1.2rem;"
        "margin-left: 10px;"
        "padding: 2px 8px;"
        "border-radius: 5px;"
        "color: #00f7ff;"
        "background: rgba(0, 247, 255, 0.08);"
        "box-shadow: 0 0 8px rgba(0, 247, 255, 0.5), 0 0 16px rgba(0, 247, 255, 0.3);"
        "animation: glowPulse 2s infinite ease-in-out;"
        "font-family: 'Orbitron', sans-serif;"
        "letter-spacing: 1px;"
      "}"
      "@keyframes glowPulse {"
        "0% { box-shadow: 0 0 6px rgba(0, 247, 255, 0.4), 0 0 12px rgba(0, 247, 255, 0.2); }"
        "50% { box-shadow: 0 0 12px rgba(0, 247, 255, 0.8), 0 0 24px rgba(0, 247, 255, 0.6); }"
        "100% { box-shadow: 0 0 6px rgba(0, 247, 255, 0.4), 0 0 12px rgba(0, 247, 255, 0.2); }"
      "}"
      "@keyframes shine {"
        "0% { transform: rotate(45deg) translate(-30%, -30%); }"
        "100% { transform: rotate(45deg) translate(30%, 30%); }"
      "}"
      ".pulse {"
        "animation: pulse 2s infinite;"
      "}"
      "@keyframes pulse {"
        "0% { transform: scale(1); opacity: 1; }"
        "50% { transform: scale(1.05); opacity: 0.8; }"
        "100% { transform: scale(1); opacity: 1; }"
      "}"
      ".current-time {"
        "position: fixed;"
        "bottom: 20px;"
        "right: 20px;"
        "background: rgba(0, 247, 255, 0.1);"
        "padding: 8px 15px;"
        "border-radius: 20px;"
        "font-family: 'Orbitron', sans-serif;"
        "font-size: 0.9rem;"
        "color: var(--primary);"
        "border: 1px solid var(--card-border);"
      "}"
      "@media (max-width: 768px) {"
        ".metrics-grid {"
          "grid-template-columns: 1fr;"
          "grid-template-rows: auto;"
        "}"
        ".header h1 {"
          "font-size: 2rem;"
        "}"
        ".metric-value {"
          "font-size: 2.2rem;"
        "}"
        ".data-table-container {"
          "overflow-x: auto;"
        "}"
        ".data-table {"
          "min-width: 700px;"
        "}"
      "}"
    "</style>"
    "</head>"
    "<body>"
      "<div class='header'>"
        "<h1>AIR QUALITY MONITORING SYSTEM <span class='glow-tag'>AQMS</span></h1>"
        "<p>Real-time environmental data visualization dashboard</p>"
      "</div>"
      
      "<div class='metrics-grid'>"
        // Row 1
        "<div class='metric-card pulse'>"
          "<div class='metric-title'>AIR QUALITY INDEX</div>"
          "<div class='metric-value' style='color:" + aqiColor + "'>" + mq135Data + "</div>"
          "<div class='metric-status'>Status: <span style='color:" + aqiColor + "'>" + aqiStatus + "</span></div>"
        "</div>"
        
        "<div class='metric-card'>"
          "<div class='metric-title'>TEMPERATURE</div>"
          "<div class='metric-value' style='color:#f1c40f'>" + tempData + "<span class='unit'>&deg;C</span></div>"
          "<div class='metric-status'>Ambient temperature</div>"
        "</div>"

        "<div class='metric-card'>"
          "<div class='metric-title'>HUMIDITY</div>"
          "<div class='metric-value' style='color:#3498db'>" + humData + "<span class='unit'>%</span></div>"
          "<div class='metric-status'>Relative humidity level</div>"
        "</div>"
        
        // Row 2
        "<div class='metric-card'>"
          "<div class='metric-title'>METHANE CONCENTRATION</div>"
          "<div class='metric-value' style='color:#2ecc71'>" + mq4Data + "<span class='unit'>ppm</span></div>"
          "<div class='metric-status'>Methane gas level</div>"
        "</div>"
        
        "<div class='metric-card'>"
          "<div class='metric-title'>PM10 PARTICULATES</div>"
          "<div class='metric-value' style='color:#9b59b6'>" + pm10Data + "<span class='unit'>&micro;g/m³</span></div>"
          "<div class='metric-status'>Coarse particles</div>"
        "</div>"
        
        "<div class='metric-card'>"
          "<div class='metric-title'>PM2.5 PARTICULATES</div>"
          "<div class='metric-value' style='color:#e91e63'>" + pm25Data + "<span class='unit'>&micro;g/m³</span></div>"
          "<div class='metric-status'>Fine particles</div>"
        "</div>"
      "</div>"
      
      + generateDataTable() +
      
      "<button class='refresh-btn' onclick='location.reload()'>"
        "<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round' style='margin-right: 8px;'><path d='M21.5 2v6h-6M21.5 22v-6h-6M2 11.5a10 10 0 0 1 18-5M2 12.5a10 10 0 0 0 18 5'></path></svg>"
        "REFRESH DASHBOARD"
      "</button>"
      
      "<div class='credits'>"
        "<div class='credits-title'>DEVELOPMENT TEAM</div>"
        "<p>This IoT environmental monitoring system was developed by:</p>"
        "<div class='team'>"
          "<div class='team-member'>"
            "<strong>Sunandan Kar</strong>"
            "<div class='role'>Lead Developer</div>"
          "</div>"
          "<div class='team-member'>"
            "<strong>Sweta Poddar</strong>"
            "<div class='role'>Database &</div>"
            "<div class='role'>Network Management</div>"
          "</div>"
          "<div class='team-member'>"
            "<strong>Swarnavo Pramanik</strong>"
            "<div class='role'>Software Architect</div>"
            "<div class='role'>Data Analyst</div>"
          "</div>"
        "</div>"
      "</div>"
      
      "<div class='current-time' id='currentTime'></div>"
      
      "<script>"
        "function updateTime() {"
          "const now = new Date();"
          "const timeString = now.toLocaleTimeString();"
          "document.getElementById('currentTime').textContent = timeString;"
        "}"
        "setInterval(updateTime, 1000);"
        "updateTime();"
        
        "// Add animation to metric cards on hover"
        "const cards = document.querySelectorAll('.metric-card');"
        "cards.forEach(card => {"
          "card.addEventListener('mouseenter', () => {"
            "card.classList.add('pulse');"
          "});"
          "card.addEventListener('mouseleave', () => {"
            "card.classList.remove('pulse');"
          "});"
        "});"
      "</script>"
    "</body></html>";

  server.send(200, "text/html", htmlPage);
}


void loop() {
  while (Serial2.available()) {
    char inChar = (char)Serial2.read();
    inputString += inChar;
    if (inChar == '\n') {
      inputString.trim();
      Serial.println("Received string: " + inputString);
      
      // Check what type of data we received
      if (inputString.startsWith("Time: ")) {
        currentTime = inputString.substring(6);
        hasTimeData = true;
      } 
      else if (inputString.startsWith("Date: ")) {
        currentDate = inputString.substring(6);
        hasDateData = true;
      } 
      else if (inputString.startsWith("Temp: ")) {
        // Only parse sensor data when we have complete time/date info
        parseSensorData(inputString);
      }
      
      inputString = "";
    }
  }

  server.handleClient();
}