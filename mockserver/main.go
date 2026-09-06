package main

/**
    SUPER SLOPPY WEB SERVER TO SIMULATE PROTOPANDA WEB INTERFACE
    just do a:
    go run .
**/
import (

	"fmt"
	"html/template"
	"io"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"
)

const (
	Port                   = 8080
	PandaVersion           = "2.0.0"
	BasePath               = "../"
	DownloadSpeedPerSecond = 1028 * 10
	MaxConcurrentRequests  = 3
	ChunkSize              = 2048
)

var (
	indexTemplate = template.Must(template.New("index").Parse(`<html>
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
    <div class="breadcrumb">
        {{range .Breadcrumbs}}<a href="/{{.Path}}">{{.Name}}</a> > {{end}}{{.CurrentDir}}
    </div>
    
    {{if eq .Path "."}}
        <div class="logo-container">
            <img src="/doc/logoprotopanda.png" alt="Protopanda Logo" class="logo-img">
        </div>
        <div class="main-header">
            <p>Welcome to ProtoPanda</p>
            <div class="header-links">
                <a href="/editor.html" class="btn btn-primary editor-btn">Static Frame Editor</a>
                <a href="/modeleditor.html" class="btn btn-primary editor-btn">Model and Keyframe Editor</a>
            </div>
        </div>
    {{end}}

    <h1>Directory Listing: /{{.Path}}</h1>
    
    <div class="table-responsive">
    <table>
      <tr>
        <th>Name</th>
        <th>Size</th>
        <th>Type</th>
        <th>Action</th>
      </tr>
      {{range .Files}}
      <tr>
        <td><a href="/{{.URL}}">{{.Name}}</a></td>
        <td>{{.Size}}</td>
        <td>{{.Type}}</td>
        <td>
          {{if .Name}}<button class='btn btn-danger' onclick="deleteFile('/{{.URL}}')">Delete</button>{{end}}
        </td>
      </tr>
      {{end}}
    </table>
    </div>
    
    <div class="action-buttons">
      <div class="action-box">
        <h3>Upload File</h3>
        <form id="uploadForm" method="post" action="/upload" enctype="multipart/form-data">
          <input type="hidden" name="path" value="{{.Path}}">
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
          <input type="hidden" name="path" value="{{.Path}}">
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
    <footer>ProtoPanda v{{.Version}} | Pixel Art Editor</footer>
  </body>
</html>`))
)

type FileInfo struct {
	Name string
	Size string
	Type string
	URL  string
}

type Breadcrumb struct {
	Name string
	Path string
}

type TemplateData struct {
	Path        string
	CurrentDir  string
	Files       []FileInfo
	Version     string
	Breadcrumbs []Breadcrumb
}

type BandwidthLimiter struct {
	sync.Mutex
	tokens       int64
	lastUpdate   time.Time
	maxTokens    int64
	tokensPerSec int64
}

// Composition state
var (
	composeInProgress bool
	composeProgress   int
	composeMutex      sync.Mutex
)

func (b *BandwidthLimiter) Wait(bytes int64) {
	b.Lock()
	defer b.Unlock()

	now := time.Now()
	elapsed := now.Sub(b.lastUpdate)

	// Add tokens based on elapsed time
	tokensToAdd := int64(elapsed.Seconds() * float64(b.tokensPerSec))
	b.tokens += tokensToAdd

	// Cap at max tokens
	if b.tokens > b.maxTokens {
		b.tokens = b.maxTokens
	}

	b.lastUpdate = now

	// If not enough tokens, wait
	if b.tokens < bytes {
		needed := bytes - b.tokens
		waitTime := time.Duration(float64(needed) / float64(b.tokensPerSec) * float64(time.Second))

		// Release lock while waiting
		b.Unlock()
		fmt.Printf("Waiting for %v to get %d bytes\n", waitTime, bytes)
		time.Sleep(waitTime)
		b.Lock()

		// Update after waiting
		b.tokens = b.maxTokens - bytes
		b.lastUpdate = time.Now()
	} else {
		b.tokens -= bytes
	}
}

func NewBandwidthLimiter(bytesPerSec int64) *BandwidthLimiter {
	return &BandwidthLimiter{
		tokens:       bytesPerSec,
		lastUpdate:   time.Now(),
		maxTokens:    bytesPerSec,
		tokensPerSec: bytesPerSec,
	}
}

var (
	bandwidthLimiter = NewBandwidthLimiter(DownloadSpeedPerSecond)
	semaphore        = make(chan struct{}, 3)
)

func main() {
	http.HandleFunc("/", serveDirectoryListing)
	http.HandleFunc("/mkdir", handleMkdir)
	http.HandleFunc("/upload", handleUpload)
	http.HandleFunc("/delete", handleDelete)
	http.HandleFunc("/copy", handleCopy)
	http.HandleFunc("/mv", handleMv)
	http.HandleFunc("/lua", handleLua)
	http.HandleFunc("/compose_start", handleComposeStart)
	http.HandleFunc("/compose_progress", handleComposeProgress)

	fmt.Printf("Server started on port %d\n", Port)
	fmt.Printf("Serving files from: %s\n", BasePath)
	fmt.Println("Available endpoints:")
	fmt.Println("  GET  /                - Directory listing")
	fmt.Println("  POST /mkdir          - Create directory")
	fmt.Println("  POST /upload         - Upload file")
	fmt.Println("  DELETE /delete       - Delete file/directory")
	fmt.Println("  PUT  /copy           - Copy file")
	fmt.Println("  PUT  /mv             - Move file")
	fmt.Println("  POST /lua            - Execute Lua code (simulated)")
	fmt.Println("  POST /compose_start  - Start composition")
	fmt.Println("  GET  /compose_progress - Get composition progress")
	log.Fatal(http.ListenAndServe(fmt.Sprintf(":%d", Port), nil))
}

func serveFileWithRateLimit(w http.ResponseWriter, r *http.Request, filePath string, info os.FileInfo) {
	semaphore <- struct{}{}
	defer func() {
		fmt.Printf("Freed\n")
		<-semaphore
	}()
	time.Sleep(time.Millisecond * 800)
	file, err := os.Open(filePath)
	if err != nil {
		http.Error(w, "Error opening file: "+err.Error(), http.StatusInternalServerError)
		return
	}
	defer file.Close()

	contentType := getContentType(filePath)
	w.Header().Set("Content-Type", contentType)

	if info.Size() >= 0 {
		w.Header().Set("Content-Length", fmt.Sprintf("%d", info.Size()))
	}

	fmt.Printf("Serving file: %s (size: %d bytes, type: %s) with global rate limiting\n",
		info.Name(), info.Size(), contentType)

	buffer := make([]byte, ChunkSize)
	totalSent := int64(0)

	for {
		n, err := file.Read(buffer)
		if n > 0 {
			bandwidthLimiter.Wait(int64(n))
			time.Sleep(time.Millisecond * 100)

			if _, writeErr := w.Write(buffer[:n]); writeErr != nil {
				fmt.Printf("Error writing to response: %v\n", writeErr)
				break
			}

			totalSent += int64(n)

			if flusher, ok := w.(http.Flusher); ok {
				flusher.Flush()
			}
		}

		if err != nil {
			if err != io.EOF {
				fmt.Printf("Error reading file: %v\n", err)
			}
			break
		}
	}

	fmt.Printf("Finished serving: %s, total sent: %d bytes\n", info.Name(), totalSent)
}

func getContentType(filePath string) string {
	ext := strings.ToLower(filepath.Ext(filePath))

	switch ext {
	case ".html", ".htm":
		return "text/html; charset=utf-8"
	case ".css":
		return "text/css; charset=utf-8"
	case ".js":
		return "application/javascript"
	case ".json":
		return "application/json"
	case ".png":
		return "image/png"
	case ".jpg", ".jpeg":
		return "image/jpeg"
	case ".gif":
		return "image/gif"
	case ".svg":
		return "image/svg+xml"
	case ".pdf":
		return "application/pdf"
	case ".txt":
		return "text/plain; charset=utf-8"
	case ".xml":
		return "application/xml"
	case ".zip":
		return "application/zip"
	case ".tar":
		return "application/x-tar"
	case ".gz":
		return "application/gzip"
	case ".mp4":
		return "video/mp4"
	case ".mp3":
		return "audio/mpeg"
	case ".wav":
		return "audio/wav"
	case ".avi":
		return "video/x-msvideo"
	case ".mov":
		return "video/quicktime"
	case ".webm":
		return "video/webm"
	case ".webp":
		return "image/webp"
	case ".ico":
		return "image/x-icon"
	case ".csv":
		return "text/csv"
	case ".doc", ".docx":
		return "application/msword"
	case ".xls", ".xlsx":
		return "application/vnd.ms-excel"
	case ".ppt", ".pptx":
		return "application/vnd.ms-powerpoint"
	default:
		return "application/octet-stream"
	}
}

func serveDirectoryListing(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	requestedPath := r.URL.Path

	if len(requestedPath) > 0 && requestedPath[0] == '/' {
		requestedPath = requestedPath[1:]
	}

	requestedPath = filepath.Clean(requestedPath)
	fmt.Printf("REQ : %s\n", requestedPath)
	fullPath := filepath.Join(BasePath, requestedPath)
	fmt.Printf("fullPath : %s\n", fullPath)
	info, err := os.Stat(fullPath)
	if err != nil {
		if os.IsNotExist(err) {
			requestedPath = ""
			fullPath = BasePath
		} else {
			http.Error(w, "Error accessing path: "+err.Error(), http.StatusInternalServerError)
			return
		}
	} else if !info.IsDir() {
		fmt.Printf("Serving file: %s\n", fullPath)
		serveFileWithRateLimit(w, r, fullPath, info)
		return
	}

	fmt.Printf("Listing dir: %s\n", fullPath)

	files, err := os.ReadDir(fullPath)
	if err != nil {
		http.Error(w, "Error reading directory: "+err.Error(), http.StatusInternalServerError)
		return
	}

	var fileList []FileInfo
	for _, file := range files {
		if strings.HasPrefix(file.Name(), ".") {
			continue
		}

		fileInfo, err := file.Info()
		if err != nil {
			continue
		}

		var size string
		var fileType string
		var url string

		if file.IsDir() {
			size = "-"
			fileType = "DIR"
			if requestedPath == "" {
				url = file.Name()
			} else {
				url = filepath.Join(requestedPath, file.Name())
			}
		} else {
			size = formatFileSize(fileInfo.Size())
			fileType = "FILE"
			if requestedPath == "" {
				url = file.Name()
			} else {
				url = filepath.Join(requestedPath, file.Name())
			}
		}

		url = strings.ReplaceAll(url, "\\", "/")

		fmt.Printf("Added the: %s\n", url)

		fileList = append(fileList, FileInfo{
			Name: file.Name(),
			Size: size,
			Type: fileType,
			URL:  url,
		})
	}

	breadcrumbs := generateBreadcrumbs(requestedPath)

	currentDir := "/"
	if requestedPath != "" {
		parts := strings.Split(requestedPath, "/")
		if len(parts) > 0 {
			currentDir = parts[len(parts)-1]
		}
	}

	displayPath := requestedPath
	if displayPath == "" {
		displayPath = "."
	}

	data := TemplateData{
		Path:        displayPath,
		CurrentDir:  currentDir,
		Files:       fileList,
		Version:     PandaVersion,
		Breadcrumbs: breadcrumbs,
	}

	if err := indexTemplate.Execute(w, data); err != nil {
		http.Error(w, "Error rendering template: "+err.Error(), http.StatusInternalServerError)
	}
}

func generateBreadcrumbs(path string) []Breadcrumb {
	var breadcrumbs []Breadcrumb

	breadcrumbs = append(breadcrumbs, Breadcrumb{Name: "/", Path: ""})

	if path == "" {
		return breadcrumbs
	}

	parts := strings.Split(path, "/")
	currentPath := ""

	for i, part := range parts {
		if part == "" {
			continue
		}

		if currentPath == "" {
			currentPath = part
		} else {
			currentPath = currentPath + "/" + part
		}

		if i < len(parts)-1 {
			breadcrumbs = append(breadcrumbs, Breadcrumb{
				Name: part,
				Path: currentPath,
			})
		}
	}

	return breadcrumbs
}

func handleMkdir(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	if err := r.ParseForm(); err != nil {
		http.Error(w, "Error parsing form", http.StatusBadRequest)
		return
	}

	basePath := r.FormValue("path")
	dirName := r.FormValue("dirName")
	if basePath == "" || dirName == "" {
		http.Error(w, "Missing parameters", http.StatusBadRequest)
		return
	}

	basePath = filepath.Clean(basePath)
	dirName = filepath.Clean(dirName)

	fullPath := filepath.Join(BasePath, basePath, dirName)

	fmt.Printf("Creating dir: %s\n", fullPath)

	if _, err := os.Stat(fullPath); err == nil {
		http.Error(w, "Directory already exists", http.StatusBadRequest)
		return
	}

	if err := os.MkdirAll(fullPath, 0755); err != nil {
		http.Error(w, "Failed to create directory: "+err.Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
	w.Write([]byte("Directory created successfully"))
}

func handleUpload(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	if err := r.ParseMultipartForm(100 << 20); err != nil {
		http.Error(w, "Error parsing form: "+err.Error(), http.StatusBadRequest)
		return
	}

	basePath := r.FormValue("path")
	file, handler, err := r.FormFile("file")
	if err != nil {
		http.Error(w, "Error retrieving file: "+err.Error(), http.StatusBadRequest)
		return
	}
	defer file.Close()

	basePath = filepath.Clean(basePath)

	fullPath := filepath.Join(BasePath, basePath)
	if err := os.MkdirAll(fullPath, 0755); err != nil {
		http.Error(w, "Error creating directories: "+err.Error(), http.StatusInternalServerError)
		return
	}

	dstPath := filepath.Join(fullPath, handler.Filename)

	fmt.Printf("Uploading dir: %s\n", dstPath)
	dst, err := os.Create(dstPath)
	if err != nil {
		http.Error(w, "Error creating file: "+err.Error(), http.StatusInternalServerError)
		return
	}
	defer dst.Close()

	if _, err := io.Copy(dst, file); err != nil {
		http.Error(w, "Error saving file: "+err.Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
	w.Write([]byte("File uploaded successfully"))
}

func handleDelete(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodDelete {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	path := r.URL.Query().Get("path")
	if path == "" {
		http.Error(w, "Missing path parameter", http.StatusBadRequest)
		return
	}

	if strings.HasPrefix(path, "/") {
		path = path[1:]
	}

	path = filepath.Clean(path)
	fullPath := filepath.Join(BasePath, path)

	fmt.Printf("Deleting path: %s\n", fullPath)

	info, err := os.Stat(fullPath)
	if err != nil {
		if os.IsNotExist(err) {
			http.Error(w, "Path not found", http.StatusNotFound)
		} else {
			http.Error(w, "Error accessing path: "+err.Error(), http.StatusInternalServerError)
		}
		return
	}

	if info.IsDir() {
		entries, err := os.ReadDir(fullPath)
		if err != nil {
			http.Error(w, "Error reading directory: "+err.Error(), http.StatusInternalServerError)
			return
		}

		hasEntries := false
		for _, entry := range entries {
			if !strings.HasPrefix(entry.Name(), ".") {
				hasEntries = true
				break
			}
		}

		if hasEntries {
			http.Error(w, "Directory is not empty", http.StatusBadRequest)
			return
		}

		if err := os.Remove(fullPath); err != nil {
			http.Error(w, "Failed to delete directory: "+err.Error(), http.StatusInternalServerError)
			return
		}
	} else {
		if err := os.Remove(fullPath); err != nil {
			http.Error(w, "Failed to delete file: "+err.Error(), http.StatusInternalServerError)
			return
		}
	}

	w.WriteHeader(http.StatusOK)
	w.Write([]byte("Deleted successfully"))
}

// New: Copy endpoint - matches ESP32 behavior
func handleCopy(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPut {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	srcPath := r.URL.Query().Get("src")
	dstPath := r.URL.Query().Get("dst")

	if srcPath == "" || dstPath == "" {
		http.Error(w, "Missing src or dst parameter", http.StatusBadRequest)
		return
	}

	if strings.HasPrefix(srcPath, "/") {
		srcPath = srcPath[1:]
	}
	if strings.HasPrefix(dstPath, "/") {
		dstPath = dstPath[1:]
	}

	srcPath = filepath.Clean(srcPath)
	dstPath = filepath.Clean(dstPath)

	fullSrcPath := filepath.Join(BasePath, srcPath)
	fullDstPath := filepath.Join(BasePath, dstPath)

	fmt.Printf("Copying from %s to %s\n", fullSrcPath, fullDstPath)

	// Check source exists
	srcInfo, err := os.Stat(fullSrcPath)
	if err != nil {
		if os.IsNotExist(err) {
			http.Error(w, "Source file not found", http.StatusNotFound)
		} else {
			http.Error(w, "Error accessing source: "+err.Error(), http.StatusInternalServerError)
		}
		return
	}

	if srcInfo.IsDir() {
		http.Error(w, "Source is a directory", http.StatusBadRequest)
		return
	}

	// Create destination directory if needed
	dstDir := filepath.Dir(fullDstPath)
	if err := os.MkdirAll(dstDir, 0755); err != nil {
		http.Error(w, "Failed to create destination directory: "+err.Error(), http.StatusInternalServerError)
		return
	}

	// Check if destination already exists
	if _, err := os.Stat(fullDstPath); err == nil {
		http.Error(w, "Destination already exists", http.StatusConflict)
		return
	}

	// Copy file
	srcFile, err := os.Open(fullSrcPath)
	if err != nil {
		http.Error(w, "Failed to open source file: "+err.Error(), http.StatusInternalServerError)
		return
	}
	defer srcFile.Close()

	dstFile, err := os.Create(fullDstPath)
	if err != nil {
		http.Error(w, "Failed to create destination file: "+err.Error(), http.StatusInternalServerError)
		return
	}
	defer dstFile.Close()

	buffer := make([]byte, 512)
	var copyErr error
	for {
		n, err := srcFile.Read(buffer)
		if n > 0 {
			if _, writeErr := dstFile.Write(buffer[:n]); writeErr != nil {
				copyErr = writeErr
				break
			}
		}
		if err != nil {
			if err != io.EOF {
				copyErr = err
			}
			break
		}
	}

	if copyErr != nil {
		os.Remove(fullDstPath)
		http.Error(w, "Copy failed: "+copyErr.Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
	w.Write([]byte("OK"))
}

// New: Move endpoint - matches ESP32 behavior
func handleMv(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPut {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	srcPath := r.URL.Query().Get("src")
	dstPath := r.URL.Query().Get("dst")

	if srcPath == "" || dstPath == "" {
		http.Error(w, "Missing src or dst parameter", http.StatusBadRequest)
		return
	}

	if strings.HasPrefix(srcPath, "/") {
		srcPath = srcPath[1:]
	}
	if strings.HasPrefix(dstPath, "/") {
		dstPath = dstPath[1:]
	}

	srcPath = filepath.Clean(srcPath)
	dstPath = filepath.Clean(dstPath)

	fullSrcPath := filepath.Join(BasePath, srcPath)
	fullDstPath := filepath.Join(BasePath, dstPath)

	fmt.Printf("Moving from %s to %s\n", fullSrcPath, fullDstPath)

	// Check source exists
	srcInfo, err := os.Stat(fullSrcPath)
	if err != nil {
		if os.IsNotExist(err) {
			http.Error(w, "Source file not found", http.StatusNotFound)
		} else {
			http.Error(w, "Error accessing source: "+err.Error(), http.StatusInternalServerError)
		}
		return
	}

	if srcInfo.IsDir() {
		http.Error(w, "Source is a directory", http.StatusBadRequest)
		return
	}

	// Create destination directory if needed
	dstDir := filepath.Dir(fullDstPath)
	if err := os.MkdirAll(dstDir, 0755); err != nil {
		http.Error(w, "Failed to create destination directory: "+err.Error(), http.StatusInternalServerError)
		return
	}

	// Check if destination already exists
	if _, err := os.Stat(fullDstPath); err == nil {
		http.Error(w, "Destination already exists", http.StatusConflict)
		return
	}

	// Copy file
	srcFile, err := os.Open(fullSrcPath)
	if err != nil {
		http.Error(w, "Failed to open source file: "+err.Error(), http.StatusInternalServerError)
		return
	}
	defer srcFile.Close()

	dstFile, err := os.Create(fullDstPath)
	if err != nil {
		http.Error(w, "Failed to create destination file: "+err.Error(), http.StatusInternalServerError)
		return
	}
	defer dstFile.Close()

	buffer := make([]byte, 512)
	var copyErr error
	for {
		n, err := srcFile.Read(buffer)
		if n > 0 {
			if _, writeErr := dstFile.Write(buffer[:n]); writeErr != nil {
				copyErr = writeErr
				break
			}
		}
		if err != nil {
			if err != io.EOF {
				copyErr = err
			}
			break
		}
	}

	if copyErr != nil {
		os.Remove(fullDstPath)
		http.Error(w, "Move failed: "+copyErr.Error(), http.StatusInternalServerError)
		return
	}

	// Delete source after successful copy
	if err := os.Remove(fullSrcPath); err != nil {
		os.Remove(fullDstPath)
		http.Error(w, "Move failed: could not remove source file", http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
	w.Write([]byte("OK"))
}

// New: Lua execution endpoint (simulated)
func handleLua(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	// Read body
	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Error reading body", http.StatusBadRequest)
		return
	}
	defer r.Body.Close()

	luaCode := string(body)
	fmt.Printf("Lua execution (simulated): %s\n", luaCode[:min(len(luaCode), 100)])

	// Simulate Lua execution
	if strings.Contains(luaCode, "error") {
		http.Error(w, "Lua Error: simulated error", http.StatusInternalServerError)
		return
	}

	// Simulate return value
	w.WriteHeader(http.StatusOK)
	w.Write([]byte("Lua executed successfully (simulated)"))
}

// New: Compose start endpoint
func handleComposeStart(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	composeMutex.Lock()
	defer composeMutex.Unlock()

	if composeInProgress {
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("Status: Composition already in progress"))
		return
	}

	composeInProgress = true
	composeProgress = 0

	// Start composition in background
	go func() {
		fmt.Println("Starting composition...")
		for i := 0; i <= 100; i += 10 {
			time.Sleep(500 * time.Millisecond)
			composeMutex.Lock()
			composeProgress = i
			composeMutex.Unlock()
			fmt.Printf("Composition progress: %d%%\n", i)
		}
		composeMutex.Lock()
		composeInProgress = false
		composeMutex.Unlock()
		fmt.Println("Composition completed!")
	}()

	w.WriteHeader(http.StatusOK)
	w.Write([]byte("Status: Composition started successfully"))
}

// New: Compose progress endpoint
func handleComposeProgress(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	composeMutex.Lock()
	progress := composeProgress
	inProgress := composeInProgress
	composeMutex.Unlock()

	var response string
	if inProgress {
		response = fmt.Sprintf("%d", progress)
	} else {
		response = "100"
	}

	w.WriteHeader(http.StatusOK)
	w.Write([]byte(response))
}

func formatFileSize(size int64) string {
	if size < 1024 {
		return fmt.Sprintf("%d B", size)
	} else if size < 1024*1024 {
		return fmt.Sprintf("%.1f KB", float64(size)/1024)
	} else if size < 1024*1024*1024 {
		return fmt.Sprintf("%.1f MB", float64(size)/(1024*1024))
	} else {
		return fmt.Sprintf("%.1f GB", float64(size)/(1024*1024*1024))
	}
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}