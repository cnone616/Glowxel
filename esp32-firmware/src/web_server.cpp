#include "web_server.h"
#include "websocket_handler.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "config_manager.h"
#include "ota_manager.h"

// 静态成员初始化
AsyncWebServer WebServer::server(80);
uint8_t* WebServer::imageBuffer = nullptr;
size_t WebServer::imageSize = 0;

void WebServer::init() {
  Serial.println("4. 启动HTTP和WebSocket服务器...");
  setupServer();
}

void WebServer::setupServer() {
  // 设置 WebSocket
  server.addHandler(&WebSocketHandler::ws);
  
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  
  setupRoutes();
  setupConfigRoutes();
  setupAPIRoutes();
  
  server.begin();
  Serial.println("HTTP服务器已启动在端口80");
  Serial.println("等待客户端连接...");
}

void WebServer::setupRoutes() {
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到测试请求");
    request->send(200, "text/plain", "ESP32 is working!");
  });
  
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("404: %s %s\n", request->methodToString(), request->url().c_str());
    if (WiFiManager::isConfigMode()) {
      request->redirect("/");
    } else {
      request->send(404, "text/plain", "Not Found");
    }
  });
}

void WebServer::setupConfigRoutes() {
  // WiFi扫描接口
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("开始扫描WiFi网络...");

    int n = WiFi.scanNetworks();
    Serial.printf("扫描完成，找到 %d 个网络\n", n);

    StaticJsonDocument<2048> doc;
    JsonArray networks = doc.createNestedArray("networks");

    for (int i = 0; i < n; i++) {
      JsonObject network = networks.createNestedObject();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted";
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);

    WiFi.scanDelete();
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到根路径请求");
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>RenLight 配置</title>";
    html += "<style>body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif;padding:20px;max-width:400px;margin:0 auto;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh}";
    html += ".card{background:white;padding:24px;border-radius:16px;box-shadow:0 8px 32px rgba(0,0,0,0.2)}";
    html += "h2{color:#333;margin-top:0;text-align:center;font-size:24px}";
    html += ".subtitle{color:#666;text-align:center;font-size:14px;margin-bottom:20px}";
    html += "label{display:block;color:#555;font-size:14px;margin-bottom:8px;font-weight:500}";
    html += "select,input{width:100%;padding:12px;margin-bottom:16px;border:2px solid #e0e0e0;border-radius:8px;box-sizing:border-box;font-size:15px}";
    html += "select:focus,input:focus{outline:none;border-color:#667eea}";
    html += "select{background:white;cursor:pointer}";
    html += "button{width:100%;padding:14px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;border:none;border-radius:8px;cursor:pointer;font-size:16px;font-weight:600}";
    html += "button:active{transform:scale(0.98)}";
    html += "button:disabled{opacity:0.6;cursor:not-allowed}";
    html += ".status{padding:12px;border-radius:8px;margin-bottom:16px;text-align:center;font-size:14px}";
    html += ".success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}";
    html += ".error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}";
    html += ".info{background:#d1ecf1;color:#0c5460;border:1px solid #bee5eb}";
    html += ".manual-input{margin-top:12px;text-align:center}";
    html += ".manual-link{color:#667eea;text-decoration:none;font-size:13px;cursor:pointer}";
    html += ".manual-link:hover{text-decoration:underline}";
    html += "</style></head><body>";
    html += "<div class='card'><h2>RenLight 配置</h2>";
    html += "<div class='subtitle'>连接到您的 WiFi 网络</div>";
    html += "<div id='status'></div>";
    html += "<div id='scan-mode'>";
    html += "<label>选择 WiFi 网络：</label>";
    html += "<select id='ssid-select'><option value=''>正在扫描...</option></select>";
    html += "<label>WiFi 密码：</label>";
    html += "<input type='password' id='password' placeholder='请输入 WiFi 密码'>";
    html += "<button onclick='saveWiFi()' id='connect-btn'>连接</button>";
    html += "<div class='manual-input'><a class='manual-link' onclick='toggleManualMode()'>手动输入 WiFi 名称</a></div>";
    html += "</div>";
    html += "<div id='manual-mode' style='display:none'>";
    html += "<label>WiFi 名称 (SSID)：</label>";
    html += "<input type='text' id='ssid-manual' placeholder='请输入 WiFi 名称'>";
    html += "<label>WiFi 密码：</label>";
    html += "<input type='password' id='password-manual' placeholder='请输入 WiFi 密码'>";
    html += "<button onclick='saveWiFiManual()'>连接</button>";
    html += "<div class='manual-input'><a class='manual-link' onclick='toggleManualMode()'>返回扫描列表</a></div>";
    html += "</div></div>";
    html += "<script>";
    html += "let manualMode=false;";
    html += "function toggleManualMode(){manualMode=!manualMode;document.getElementById('scan-mode').style.display=manualMode?'none':'block';document.getElementById('manual-mode').style.display=manualMode?'block':'none';}";
    html += "async function scanWiFi(){";
    html += "try{showStatus('正在扫描 WiFi 网络...','info');";
    html += "const res=await fetch('/scan');";
    html += "const data=await res.json();";
    html += "const sel=document.getElementById('ssid-select');";
    html += "sel.innerHTML='';";
    html += "if(data.networks.length===0){sel.innerHTML='<option value=\"\">未找到网络</option>';showStatus('未找到 WiFi 网络，请使用手动输入','error');return;}";
    html += "data.networks.forEach(n=>{const opt=document.createElement('option');opt.value=n.ssid;opt.text=n.ssid+' ('+(n.encryption==='open'?'开放':n.rssi+'dBm')+')';sel.add(opt);});";
    html += "showStatus('扫描完成，找到 '+data.networks.length+' 个网络','success');";
    html += "document.getElementById('connect-btn').disabled=false;";
    html += "}catch(e){showStatus('扫描失败，请使用手动输入','error');}}";
    html += "async function saveWiFi(){";
    html += "const ssid=document.getElementById('ssid-select').value;";
    html += "const pwd=document.getElementById('password').value;";
    html += "if(!ssid||!pwd){showStatus('请选择 WiFi 并输入密码','error');return;}";
    html += "showStatus('正在连接 '+ssid+'...','info');";
    html += "const formData=new FormData();formData.append('ssid',ssid);formData.append('password',pwd);";
    html += "try{const res=await fetch('/save',{method:'POST',body:formData});";
    html += "const data=await res.json();showStatus('配置已保存！设备重启中...','success');";
    html += "setTimeout(()=>{showStatus('设备已重启，请关闭此页面并连接到您的 WiFi','success');},3000);";
    html += "}catch(e){showStatus('连接失败，请检查密码是否正确','error');}}";
    html += "async function saveWiFiManual(){";
    html += "const ssid=document.getElementById('ssid-manual').value;";
    html += "const pwd=document.getElementById('password-manual').value;";
    html += "if(!ssid||!pwd){showStatus('请填写完整的 WiFi 信息','error');return;}";
    html += "showStatus('正在连接 '+ssid+'...','info');";
    html += "const formData=new FormData();formData.append('ssid',ssid);formData.append('password',pwd);";
    html += "try{const res=await fetch('/save',{method:'POST',body:formData});";
    html += "const data=await res.json();showStatus('配置已保存！设备重启中...','success');";
    html += "setTimeout(()=>{showStatus('设备已重启，请关闭此页面并连接到您的 WiFi','success');},3000);";
    html += "}catch(e){showStatus('连接失败，请检查密码是否正确','error');}}";
    html += "function showStatus(msg,type){document.getElementById('status').innerHTML='<div class=\"'+type+'\">'+msg+'</div>';}";
    html += "document.getElementById('connect-btn').disabled=true;";
    html += "scanWiFi();";
    html += "</script>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });
  
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String password = request->getParam("password", true)->value();
      
      Serial.println("保存WiFi配置: " + ssid);
      
      ConfigManager::preferences.begin("wifi", false);
      ConfigManager::preferences.putString("ssid", ssid);
      ConfigManager::preferences.putString("password", password);
      ConfigManager::preferences.end();
      
      request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"配置已保存，正在重启...\"}");
      
      Serial.println("3秒后重启...");
      delay(3000);
      ESP.restart();
    } else {
      request->send(400, "application/json", "{\"error\":\"参数错误\"}");
    }
  });
  
  // 清除WiFi配置
  server.on("/clear-wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("清除WiFi配置");
    ConfigManager::preferences.begin("wifi", false);
    ConfigManager::preferences.clear();
    ConfigManager::preferences.end();
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"WiFi配置已清除，3秒后重启\"}");
    Serial.println("3秒后重启...");
    delay(3000);
    ESP.restart();
  });
}

void WebServer::setupAPIRoutes() {
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("收到状态查询请求");
    StaticJsonDocument<300> doc;
    doc["status"] = "ok";
    doc["ip"] = WiFiManager::getDeviceIP();
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["device_type"] = DEVICE_TYPE;
    doc["width"] = DisplayManager::PANEL_RES_X;
    doc["height"] = DisplayManager::PANEL_RES_Y;
    doc["brightness"] = DisplayManager::currentBrightness;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/upload", HTTP_POST, 
    [](AsyncWebServerRequest *request){
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if (index == 0) {
        Serial.println("开始接收图片...");
        if (imageBuffer) free(imageBuffer);
        imageBuffer = nullptr;
        imageSize = 0;
      }
      
      uint8_t* newBuffer = (uint8_t*)realloc(imageBuffer, imageSize + len);
      if (newBuffer) {
        imageBuffer = newBuffer;
        memcpy(imageBuffer + imageSize, data, len);
        imageSize += len;
      }
      
      if (final) {
        Serial.printf("接收完成，大小: %d bytes\n", imageSize);
        if (imageBuffer != nullptr && imageSize > 0) {
          int width = request->hasParam("width") ? request->getParam("width")->value().toInt() : DisplayManager::PANEL_RES_X;
          int height = request->hasParam("height") ? request->getParam("height")->value().toInt() : DisplayManager::PANEL_RES_Y;
          DisplayManager::displayImage(imageBuffer, imageSize, width, height);
        } else {
          Serial.println("✗ 图片缓冲区为空，无法显示");
        }
        
        if (imageBuffer) {
          free(imageBuffer);
          imageBuffer = nullptr;
        }
        imageSize = 0;
      }
    }
  );
  
  server.on("/brightness", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("value", true)) {
      int brightness = request->getParam("value", true)->value().toInt();
      if (brightness >= 0 && brightness <= 255) {
        DisplayManager::setBrightness(brightness);
        request->send(200, "application/json", "{\"status\":\"ok\",\"brightness\":" + String(brightness) + "}");
      } else {
        request->send(400, "application/json", "{\"error\":\"brightness must be 0-255\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"missing value parameter\"}");
    }
  });
  
  server.on("/text", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("content", true)) {
      String text = request->getParam("content", true)->value();
      int x = request->hasParam("x", true) ? request->getParam("x", true)->value().toInt() : 0;
      int y = request->hasParam("y", true) ? request->getParam("y", true)->value().toInt() : 0;
      
      DisplayManager::dma_display->clearScreen();
      DisplayManager::dma_display->setCursor(x, y);
      DisplayManager::dma_display->setTextColor(DisplayManager::dma_display->color565(255, 255, 255));
      DisplayManager::dma_display->print(text);
      
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing content parameter\"}");
    }
  });
}

void WebServer::handleLoop() {
  // Web服务器的循环处理（如果需要）
}