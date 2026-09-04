#include "editmode/editmode.hpp"
#ifdef ENABLE_EDIT_MODE
#include "tools/config_default.hpp"
#include "tools/devices.hpp"
#include "drawing/framerepository.hpp"
#include "lua/luainterface.hpp"
#include "drawing/animation.hpp"


#if PANDA_SD_MODE == 1
#include <SD.h>
#elif PANDA_SD_MODE == 2
#include <SD_MMC.h>
#else
#error "NO SD_MODE Mode defined (set PANDA_SD_MODE to 1 for SD or 2 for SD_MMC)"
#endif

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer *server;
#ifdef ENABLE_LUA
extern LuaInterface g_lua;
#endif
extern FrameRepository g_frameRepo;
extern Animation g_animation;

bool createDirectories(String path){
  String currentPath = "";
  int startIdx = 0;
  int slashIdx;

  while ((slashIdx = path.indexOf('/', startIdx)) != -1){
    currentPath = path.substring(0, slashIdx);
    if (!PANDA_SD.exists(currentPath)){
      PANDA_SD.mkdir(currentPath);
    }
    startIdx = slashIdx + 1;
  }

  if (!PANDA_SD.exists(path)){
    PANDA_SD.mkdir(path);
  }
  return true;
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  static File uploadFile;
  static String filePath;

  if (!index){
    String path = "/";
    if (request->hasParam("path", true)){
      path = request->getParam("path", true)->value();
    }
    filePath = path + "/" + filename;

    String dirPath = filePath.substring(0, filePath.lastIndexOf('/'));
    if (dirPath.length() > 0 && !PANDA_SD.exists(dirPath)){
      if (!createDirectories(dirPath)){
        request->send(500, "text/plain", "{\"success\": false, \"error\": \"Failed to open file for writing\"}");
      }
    }

    uploadFile = PANDA_SD.open(filePath, FILE_WRITE);
    if (!uploadFile){
      request->send(500, "text/plain", "{\"success\": false, \"error\": \"Failed to open file for writing\"}");
      Serial.println("Failed to open file for writing");
      return;
    }
  }

  if (uploadFile && len){
    uploadFile.write(data, len);
  }

  if (final && uploadFile){
    uploadFile.close();
    request->send(200, "text/plain", "{\"success\": true}");
  }
}

void handleCopy(AsyncWebServerRequest *request){
  if (!request->hasParam("src") || !request->hasParam("dst")){
    request->send(400, "text/plain", "Missing src or dst parameter");
    return;
  }

  String srcPath = request->getParam("src")->value();
  String dstPath = request->getParam("dst")->value();

  if (!PANDA_SD.exists(srcPath)){
    request->send(404, "text/plain", "Source file not found");
    return;
  }

  File src = PANDA_SD.open(srcPath);
  if (src.isDirectory()){
    src.close();
    request->send(400, "text/plain", "Source is a directory");
    return;
  }

  // Extract directory path from dstPath
  int lastSlash = dstPath.lastIndexOf('/');
  if (lastSlash > 0){
    String dstDir = dstPath.substring(0, lastSlash);
    
    // Create directory if it doesn't exist
    if (!PANDA_SD.exists(dstDir)){
      // Create all necessary parent directories
      String currentPath = "";
      for (int i = 0; i < dstDir.length(); i++){
        currentPath += dstDir[i];
        if (dstDir[i] == '/' && i > 0) // Found a directory level
        {
          if (!PANDA_SD.exists(currentPath.substring(0, currentPath.length() - 1))){
            if (!PANDA_SD.mkdir(currentPath.substring(0, currentPath.length() - 1))){
              src.close();
              request->send(500, "text/plain", "Failed to create directory: " + currentPath);
              return;
            }
          }
        }
      }
      
      // Create the final directory
      if (!PANDA_SD.mkdir(dstDir)){
        src.close();
        request->send(500, "text/plain", "Failed to create destination directory");
        return;
      }
    }
  }

  // Check if destination already exists (file, not directory)
  if (PANDA_SD.exists(dstPath)){
    File dstCheck = PANDA_SD.open(dstPath);
    if (!dstCheck.isDirectory()) // Only fail if it's a file
    {
      src.close();
      dstCheck.close();
      request->send(409, "text/plain", "Destination already exists");
      return;
    }
    dstCheck.close();
  }

  File dst = PANDA_SD.open(dstPath, FILE_WRITE);
  if (!dst){
    src.close();
    request->send(500, "text/plain", "Failed to create destination file");
    return;
  }

  uint8_t buffer[512];
  size_t bytesRead;
  bool error = false;
  
  while ((bytesRead = src.read(buffer, sizeof(buffer))) > 0){
    if (dst.write(buffer, bytesRead) != bytesRead){
      error = true;
      break;
    }
  }

  src.close();
  dst.close();

  if (error){
    PANDA_SD.remove(dstPath);
    request->send(500, "text/plain", "Copy failed");
    return;
  }

  request->send(200, "text/plain", "OK");
}

void handleMv(AsyncWebServerRequest *request){
  if (!request->hasParam("src") || !request->hasParam("dst")){
    request->send(400, "text/plain", "Missing src or dst parameter");
    return;
  }

  String srcPath = request->getParam("src")->value();
  String dstPath = request->getParam("dst")->value();

  if (!PANDA_SD.exists(srcPath)){
    request->send(404, "text/plain", "Source file not found");
    return;
  }

  File src = PANDA_SD.open(srcPath);
  if (src.isDirectory()){
    src.close();
    request->send(400, "text/plain", "Source is a directory");
    return;
  }

  // Extract directory path from dstPath
  int lastSlash = dstPath.lastIndexOf('/');
  if (lastSlash > 0){
    String dstDir = dstPath.substring(0, lastSlash);
    
    // Create directory if it doesn't exist
    if (!PANDA_SD.exists(dstDir)){
      // Create all necessary parent directories
      String currentPath = "";
      for (int i = 0; i < dstDir.length(); i++){
        currentPath += dstDir[i];
        if (dstDir[i] == '/' && i > 0) // Found a directory level
        {
          if (!PANDA_SD.exists(currentPath.substring(0, currentPath.length() - 1))){
            if (!PANDA_SD.mkdir(currentPath.substring(0, currentPath.length() - 1))){
              src.close();
              request->send(500, "text/plain", "Failed to create directory: " + currentPath);
              return;
            }
          }
        }
      }
      
      // Create the final directory
      if (!PANDA_SD.mkdir(dstDir)){
        src.close();
        request->send(500, "text/plain", "Failed to create destination directory");
        return;
      }
    }
  }

  // Check if destination already exists (file, not directory)
  if (PANDA_SD.exists(dstPath)){
    File dstCheck = PANDA_SD.open(dstPath);
    if (!dstCheck.isDirectory()) // Only fail if it's a file
    {
      src.close();
      dstCheck.close();
      request->send(409, "text/plain", "Destination already exists");
      return;
    }
    dstCheck.close();
  }

  File dst = PANDA_SD.open(dstPath, FILE_WRITE);
  if (!dst){
    src.close();
    request->send(500, "text/plain", "Failed to create destination file");
    return;
  }

  uint8_t buffer[512];
  size_t bytesRead;
  bool error = false;
  
  while ((bytesRead = src.read(buffer, sizeof(buffer))) > 0){
    if (dst.write(buffer, bytesRead) != bytesRead){
      error = true;
      break;
    }
  }

  src.close();
  dst.close();

  if (error){
    PANDA_SD.remove(dstPath);
    request->send(500, "text/plain", "Copy failed");
    return;
  }

  PANDA_SD.remove(srcPath);

  request->send(200, "text/plain", "OK");
}

void handleRm(AsyncWebServerRequest *request){
  if (!request->hasParam("path")){
    request->send(400, "text/plain", "Missing path parameter");
    return;
  }

  String path = request->getParam("path")->value();

  if (!PANDA_SD.exists(path)){
    request->send(404, "text/plain", "Path not found");
    return;
  }

  File file = PANDA_SD.open(path);
  if (file.isDirectory()){
    bool isEmpty = true;
    File entry = file.openNextFile();
    while (entry){
      if (String(entry.name()) != "." && String(entry.name()) != ".."){
        isEmpty = false;
        break;
      }
      entry = file.openNextFile();
    }
    entry.close();

    if (!isEmpty){
      file.close();
      request->send(400, "text/plain", "Directory is not empty");
      return;
    }

    file.close();
    if (!PANDA_SD.rmdir(path)){
      request->send(500, "text/plain", "Failed to delete directory");
      return;
    }
  }else{
    file.close();
    if (!PANDA_SD.remove(path)){
      request->send(500, "text/plain", "Failed to delete file");
      return;
    }
  }

  request->send(200, "text/plain", "Deleted successfully");
}

void handleMkdir(AsyncWebServerRequest *request){
  if (request->hasParam("path", true) && request->hasParam("dirName", true)){
    String basePath = request->getParam("path", true)->value();
    String dirName = request->getParam("dirName", true)->value();
    String fullPath = basePath + "/" + dirName;

    fullPath.replace("//", "/");
    if (PANDA_SD.exists(fullPath)){
      request->send(400, "text/plain", "Directory already exists");
    }
    else if (PANDA_SD.mkdir(fullPath)){
      request->send(200, "text/plain", "Directory created successfully");
    }else{
      request->send(500, "text/plain", "Failed to create directory");
    }
  }else{
    request->send(400, "text/plain", "Missing parameters");
  }
}
void serveDirectoryListing(AsyncWebServerRequest *request){
  String path = request->url();
  path.replace("//", "/");
  while (path.endsWith("/") && path.length() > 1){
    path.remove(path.length() - 1);
  }

  if (!PANDA_SD.exists(path)){
    path = "/";
  }

  String output = R"(
  <html>
  <head>
    <title>Directory Listing</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
      * {
        box-sizing: border-box;
      }
      body {
        font-family: 'Courier New', monospace;
        margin: 15px;
        padding: 0;
        display: flex;
        flex-direction: column;
        min-height: 100vh;
        background: #1a1a1a;
        color: #ccc;
        font-size: 18px;
      }
      @media (min-width: 768px) {
        body {
          margin: 30px;
          font-size: 20px;
        }
      }
      h1 {
        color: #ff9900;
        margin-bottom: 20px;
        font-size: 28px;
        font-weight: bold;
        word-break: break-word;
      }
      @media (min-width: 768px) {
        h1 {
          font-size: 36px;
          margin-bottom: 25px;
        }
      }
      .table-responsive {
        overflow-x: auto;
        width: 100%;
      }
      table {
        border-collapse: collapse;
        width: 100%;
        margin-bottom: 20px;
        box-shadow: 0 1px 3px rgba(0,0,0,0.3);
        background: #2d2d2d;
      }
      th, td {
        border: 1px solid #444;
        padding: 12px 10px;
        text-align: left;
        font-size: 16px;
      }
      @media (min-width: 768px) {
        th, td {
          padding: 15px;
          font-size: 18px;
        }
      }
      th {
        background-color: #3c3c3c;
        font-weight: bold;
        color: #ff9900;
      }
      tr:nth-child(even) {
        background-color: #252525;
      }
      tr:hover {
        background-color: #3a3a3a;
      }
      .action-buttons {
        display: flex;
        flex-direction: column;
        gap: 15px;
        margin-top: 20px;
      }
      @media (min-width: 768px) {
        .action-buttons {
          flex-direction: row;
          gap: 25px;
        }
      }
      .action-box {
        flex: 1;
        padding: 20px;
        background-color: #2d2d2d;
        border-radius: 8px;
        box-shadow: 0 1px 3px rgba(0,0,0,0.3);
        border: 1px solid #444;
      }
      @media (min-width: 768px) {
        .action-box {
          padding: 25px;
        }
      }
      .action-box h3 {
        margin-top: 0;
        color: #ff9900;
        font-size: 20px;
        margin-bottom: 15px;
      }
      @media (min-width: 768px) {
        .action-box h3 {
          font-size: 24px;
          margin-bottom: 20px;
        }
      }
      .form-row {
        margin-bottom: 15px;
      }
      footer {
        margin-top: 30px;
        padding: 15px 0;
        text-align: center;
        color: #666;
        font-size: 12px;
        border-top: 1px solid #444;
      }
      @media (min-width: 768px) {
        footer {
          margin-top: 40px;
          padding: 20px 0;
          font-size: 14px;
        }
      }
      .form-row label {
        display: block;
        margin-bottom: 6px;
        font-weight: bold;
        color: #aaa;
        font-size: 15px;
      }
      @media (min-width: 768px) {
        .form-row label {
          margin-bottom: 8px;
          font-size: 16px;
        }
      }
      input[type="text"],
      input[type="file"] {
        width: 100%;
        padding: 10px;
        border: 1px solid #555;
        border-radius: 4px;
        box-sizing: border-box;
        background: #1e1e1e;
        color: #fff;
        font-family: 'Courier New', monospace;
        font-size: 16px;
      }
      @media (min-width: 768px) {
        input[type="text"],
        input[type="file"] {
          padding: 12px;
          font-size: 18px;
        }
      }
      input[type="text"]:focus,
      input[type="file"]:focus {
        outline: none;
        border-color: #ff9900;
      }
      .btn {
        padding: 10px 20px;
        border: none;
        border-radius: 4px;
        cursor: pointer;
        font-weight: bold;
        transition: all 0.1s ease;
        text-align: center;
        font-family: 'Courier New', monospace;
        font-size: 16px;
        width: 100%;
      }
      @media (min-width: 768px) {
        .btn {
          padding: 12px 24px;
          font-size: 18px;
          width: auto;
        }
      }
      .btn-primary {
        background-color: #ff9900;
        color: #111;
      }
      .btn-primary:hover {
        background-color: #ffaa33;
        transform: translateY(-1px);
      }
      .btn-danger {
        background-color: #ff6666;
        color: #111;
      }
      .btn-danger:hover {
        background-color: #ff4444;
        transform: translateY(-1px);
      }
      .status {
        margin-top: 10px;
        padding: 10px;
        border-radius: 4px;
        font-size: 14px;
        word-break: break-word;
      }
      @media (min-width: 768px) {
        .status {
          margin-top: 15px;
          padding: 12px;
          font-size: 15px;
        }
      }
      .status-success {
        background-color: #1e3a1e;
        color: #4CAF50;
        border: 1px solid #4CAF50;
      }
      .status-error {
        background-color: #3a1e1e;
        color: #ff6666;
        border: 1px solid #ff6666;
      }
      a {
        color: #9cdcfe;
        text-decoration: none;
        font-size: 16px;
        word-break: break-word;
      }
      @media (min-width: 768px) {
        a {
          font-size: 18px;
        }
      }
      a:hover {
        color: #ff9900;
        text-decoration: underline;
      }
      .breadcrumb {
        margin-bottom: 20px;
        font-size: 18px;
        color: #ff9900;
        font-weight: bold;
        padding: 12px;
        background: #2d2d2d;
        border-radius: 6px;
        border: 1px solid #444;
        word-break: break-word;
        overflow-x: auto;
      }
      @media (min-width: 768px) {
        .breadcrumb {
          margin-bottom: 25px;
          font-size: 24px;
          padding: 15px;
        }
      }
      .breadcrumb a {
        color: #9cdcfe;
        text-decoration: none;
        font-size: 18px;
        font-weight: normal;
      }
      @media (min-width: 768px) {
        .breadcrumb a {
          font-size: 24px;
        }
      }
      .breadcrumb a:hover {
        color: #ff9900;
        text-decoration: underline;
      }
      .main-header {
        background-color: #2d2d2d;
        padding: 20px;
        border-radius: 8px;
        margin-bottom: 20px;
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 15px;
        border: 1px solid #444;
      }
      @media (min-width: 768px) {
        .main-header {
          padding: 35px;
          margin-bottom: 25px;
          gap: 25px;
        }
      }
      .main-header p {
        margin: 0;
        font-size: 22px;
        color: #ff9900;
        font-weight: bold;
        text-align: center;
      }
      @media (min-width: 768px) {
        .main-header p {
          font-size: 28px;
        }
      }
      .header-links {
        display: flex;
        flex-direction: column;
        gap: 15px;
        align-items: center;
        width: 100%;
      }
      @media (min-width: 768px) {
        .header-links {
          flex-direction: row;
          gap: 30px;
        }
      }
      .editor-btn {
        padding: 12px 20px;
        font-size: 16px;
        background-color: #ff9900 !important;
        color: #111 !important;
        width: 100%;
        text-align: center;
      }
      @media (min-width: 768px) {
        .editor-btn {
          padding: 15px 30px;
          font-size: 18px;
          width: auto;
        }
      }
      .editor-btn:hover {
        background-color: #ffaa33 !important;
        transform: translateY(-2px);
      }
      .logo-container {
        display: flex;
        justify-content: center;
        margin-bottom: 20px;
      }
      .logo-img {
        max-width: 100%;
        height: auto;
        border-radius: 5px;
      }
      td .btn-danger {
        width: auto;
        padding: 6px 12px;
        font-size: 13px;
      }
      @media (min-width: 768px) {
        td .btn-danger {
          padding: 8px 16px;
          font-size: 15px;
        }
      }
    </style>
  </head>
  <body>
    <div class="breadcrumb">)";

  if (path != "/"){
    output += "<a href='/'>/</a> > ";

    String currentPath = "";
    String parts = path.substring(1);
    int lastSlash = 0;

    while (lastSlash != -1){
      int nextSlash = parts.indexOf('/', lastSlash);
      String part = nextSlash == -1 ? parts.substring(lastSlash) : parts.substring(lastSlash, nextSlash);
      currentPath += "/" + part;

      if (nextSlash != -1){
        output += "<a href='" + currentPath + "'>" + part + "</a> > ";
        lastSlash = nextSlash + 1;
      }else{
        output += part;
        lastSlash = -1;
      }
    }
  }else{
    output += "Root Directory";
  }

  output += R"(</div>)";
  
  // Header Section Logic for Root Path
  if (path == "/"){
    // Logo container
    output += R"(
    <div class="logo-container">
      <img src="/doc/logoprotopanda.png" alt="Protopanda Logo" class="logo-img">
    </div>)";
    
    // Welcome message and buttons
    output += R"(
    <div class="main-header">
      <p>Welcome to ProtoPanda</p>
      <div class="header-links">
        <a href="/editor.html" class="btn btn-primary editor-btn">Static Frame Editor</a>
        <a href="/modeleditor.html" class="btn btn-primary editor-btn">Model and Keyframe Editor</a>
      </div>
    </div>)";
  }

  output += R"(
    <h1>Directory Listing: )";
  output += path;
  output += R"(</h1>
    
    <div class="table-responsive">
    <table>
      <tr>
        <th>Name</th>
        <th>Size</th>
        <th>Type</th>
        <th>Action</th>
      </td>)";

  File root = PANDA_SD.open(path);
  File file = root.openNextFile();
  if (path == "/"){
    path = "";
  }

  while (file){
    String fileName = file.name();
    // Skip hidden files and special directories
    if (!fileName.startsWith(".") && fileName.length() > 0){
      String filePath = path + "/" + fileName;
      output += "<tr>";
      output += "<td><a href='" + filePath + "'>" + fileName + "</a></td>";
      output += "<td>" + String(file.isDirectory() ? "-" : String(file.size())) + "</td>";
      output += "<td>" + String(file.isDirectory() ? "DIR" : "FILE") + "</td>";
      output += "<td><button class='btn btn-danger' onclick=\"deleteFile('" + filePath + "')\">Delete</button></td>";
      output += "</tr>";
    }
    file = root.openNextFile();
  }

  output += R"(
    </table>
    </div>
    
    <div class="action-buttons">
      <div class="action-box">
        <h3>Upload File</h3>
        <form id="uploadForm" method="post" action="/upload" enctype="multipart/form-data">
          <input type="hidden" name="path" value=")";
  output += path;
  output += R"(">
          <div class="form-row">
            <label for="fileInput">Select file:</label>
            <input type="file" name="file" id="fileInput" required>
          </div>
          <button type="submit" class="btn btn-primary">Upload</button>
        </form>
        <div id="uploadStatus" class="status"></div>
      </div>
      
      <div class="action-box">
        <h3>Create Directory</h3>
        <form id="createDirForm">
          <input type="hidden" name="path" value=")";
  output += path;
  output += R"(">
          <div class="form-row">
            <label for="dirName">Directory name:</label>
            <input type="text" name="dirName" id="dirName" required>
          </div>
          <button type="submit" class="btn btn-primary">Create</button>
        </form>
        <div id="createDirStatus" class="status"></div>
      </div>
    </div>
    
    <script>
      function deleteFile(path) {
        if (confirm('Are you sure you want to delete ' + path + '?')) {
          fetch('/delete?path=' + encodeURIComponent(path), { method: 'DELETE' })
            .then(response => { 
              if (response.ok) location.reload(); 
              else response.text().then(text => alert('Error: ' + text));
            })
            .catch(error => console.error('Error:', error));
        }
      }
      
      function setStatus(element, message, isError) {
        element.innerHTML = message;
        element.className = isError ? 'status status-error' : 'status status-success';
      }
      
      document.getElementById('uploadForm').addEventListener('submit', function(e) {
        e.preventDefault();
        const formData = new FormData(this);
        const statusDiv = document.getElementById('uploadStatus');
        setStatus(statusDiv, 'Uploading...', false);
        
        fetch('/upload', {
          method: 'POST',
          body: formData
        })
        .then(response => {
          if (response.ok) {
            setStatus(statusDiv, 'Upload completed!', false);
            setTimeout(() => location.reload(), 500);
          } else {
            response.text().then(text => setStatus(statusDiv, 'Error: ' + text, true));
          }
        })
        .catch(error => {
          setStatus(statusDiv, 'Error: ' + error, true);
        });
      });
      
      document.getElementById('createDirForm').addEventListener('submit', function(e) {
        e.preventDefault();
        const formData = new FormData(this);
        const statusDiv = document.getElementById('createDirStatus');
        setStatus(statusDiv, 'Creating directory...', false);
        
        fetch('/mkdir', {
          method: 'POST',
          body: new URLSearchParams(formData)
        })
        .then(response => {
          if (response.ok) {
            setStatus(statusDiv, 'Directory created successfully!', false);
            setTimeout(() => location.reload(), 500);
          } else {
            response.text().then(text => setStatus(statusDiv, 'Error: ' + text, true));
          }
        })
        .catch(error => {
          setStatus(statusDiv, 'Error: ' + error, true);
        });
      });
    </script>
    <footer>ProtoPanda v)";

  output += PANDA_VERSION;

  output += R"( | Pixel Art Editor</footer>
  </body>
  </html>)";

  request->send(200, "text/html", output);
}

void handleLuaExecution(AsyncWebServerRequest *request){
  if (request->method() != HTTP_POST)  {
    request->send(405, "text/plain", "Method Not Allowed");
    return;
  }

  if (!request->hasParam("body", true))  {
    request->send(400, "text/plain", "Missing Lua code in body");
    return;
  }

  String luaCode = request->getParam("body", true)->value();
  #ifdef ENABLE_LUA
  if (!g_lua.DoString(luaCode.c_str(), 1)){
    String error = g_lua.getLastError();
    request->send(500, "text/plain", "Lua Error: " + error);
    return;
  }
  
  request->send(200, "text/plain", g_lua.getLastReturnAsString());
  #else
  request->send(500, "text/plain", "Lua Error not enabled");
  #endif
}

TaskHandle_t composeTaskHandle = NULL;
TaskHandle_t managedFramesHandle = NULL;
bool compositionComplete = false;
static bool isManaged = false;
static uint32_t managedDuration = 0;
static uint32_t lastCompose = 0;
void composeBulkFileTask(void *parameter){
  if (lastCompose > millis()){
    return;
  }
  lastCompose = millis() + 10*1000;
  g_frameRepo.composeBulkFile();
  compositionComplete = true;
  vTaskDelay(pdMS_TO_TICKS(1000)); // Short delay before cleanup
  composeTaskHandle = NULL;
  lastCompose = millis() + 10*1000;
  vTaskDelete(NULL);
}

void handleComposeStart(AsyncWebServerRequest *request){
  // Check if task is already running
  if (composeTaskHandle != NULL){
    request->send(200, "text/plain", "Status: Composition already in progress");
    return;
  }
  if (lastCompose > millis()){
    request->send(200, "text/plain", "Status: Composition already in progress");
    return;
  }

  compositionComplete = false;

  BaseType_t result = xTaskCreate(
      composeBulkFileTask,
      "BulkCompose",
      64192,
      NULL,
      tskIDLE_PRIORITY,
      &composeTaskHandle);

  if (result == pdPASS){
    request->send(200, "text/plain", "Status: Composition started successfully");
  }else{
    request->send(500, "text/plain", "Error: Failed to start composition task");
  }
}

void managedLoop(void *){
  
  for (;;){
    Devices::BeginAutoFrame();
    g_animation.Update(Devices::getAutoDeltaTime());
    Devices::EndAutoFrame();
    vTaskDelay(5);
    if (millis() > managedDuration){
      break;
    }
  }
  isManaged = false;
  vTaskDelete(NULL);
}

void handleSetManaged(AsyncWebServerRequest *request){
  
  if (!isManaged){
    managedDuration = millis() + 10*1000;
    isManaged = true;
    xTaskCreate(
        managedLoop,
        "second loop",
        10000,
        NULL,
        tskIDLE_PRIORITY,
        &managedFramesHandle);

    
    request->send(200, "text/plain", "ok");
  }else{
    request->send(400, "text/plain", "already running");
  }
}
void handleComposeGet(AsyncWebServerRequest *request){
  request->send(200, "text/plain", String(g_frameRepo.getBulkComposingPercentage()));
}

void startWifiServer(int port){
  server = new AsyncWebServer(port);

  server->on("/", HTTP_GET, serveDirectoryListing);
  server->serveStatic("/", PANDA_SD, "/", "max-age=0").setCacheControl("max-age=0");
  server->on("/mkdir", HTTP_POST, handleMkdir);
  server->on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){ request->send(200); }, handleUpload);
  server->on("/delete", HTTP_DELETE, handleRm);
  server->on("/copy", HTTP_PUT, handleCopy);
  server->on("/mv", HTTP_PUT, handleMv);
  server->on("/lua", HTTP_POST, handleLuaExecution);
  server->on("/compose_start", HTTP_POST, handleComposeStart);
  server->on("/compose_progress", HTTP_GET, handleComposeGet);
  server->on("/manage", HTTP_GET, handleSetManaged);
  server->onNotFound(serveDirectoryListing);
  uint32_t freeHeapBytes = ESP.getFreeHeap();  
  uint32_t totalHeapBytes = ESP.getHeapSize(); 
  uint32_t freePsramBytes = ESP.getFreePsram(); 
  uint32_t totalPsramBytes = ESP.getPsramSize(); 

  float percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;
  float percentagePsramFree = freePsramBytes* 100.0f / (float)totalPsramBytes;

  Serial.printf("[Memory] %.1f%% free - %lu of %lu bytes free (psram: %lu / %lu  -> %.1f%%)", percentageHeapFree, freeHeapBytes, totalHeapBytes, totalPsramBytes, freePsramBytes, percentagePsramFree);

  freeHeapBytes = ESP.getFreeHeap();  
  totalHeapBytes = ESP.getHeapSize(); 
  freePsramBytes = ESP.getFreePsram(); 
  totalPsramBytes = ESP.getPsramSize(); 

  percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;
  percentagePsramFree = freePsramBytes* 100.0f / (float)totalPsramBytes;
  Serial.printf("[Memory] %.1f%% free - %lu of %lu bytes free (psram: %lu / %lu  -> %.1f%%)", percentageHeapFree, freeHeapBytes, totalHeapBytes, totalPsramBytes, freePsramBytes, percentagePsramFree);
  server->begin();
}
#endif