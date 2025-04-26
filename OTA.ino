#include <Update.h>


const char* passwordHash = "72ae575e33508b334aa2bed6d3613ea7";

/*
 * Server Index Page
 */

const char serverIndex[] PROGMEM = R"rawliteral(
<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>
<script src='https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4'></script>
<div class="flex flex-col items-center bg-gray-100 p-6 h-screen">
  <form method="POST" action="#" enctype="multipart/form-data" id="upload_form" class="flex flex-col gap-4 p-4 border border-gray-300 rounded shadow-md w-64 bg-white">
    <input type="password" name="password" placeholder="Enter your password" class="px-3 py-2 border rounded focus:outline-none focus:ring-2 focus:ring-blue-500" />
    <input type="file" name="update" class="px-3 py-2 border rounded focus:outline-none focus:ring-2 focus:ring-blue-500" />
    <input type="submit" value="Update" class="px-4 py-2 bg-blue-500 text-white font-bold rounded hover:bg-blue-600 cursor-pointer" />
  </form>
  
  <div class="mt-4 w-64">
    <div class="flex items-center justify-between mb-1">
      <span class="text-sm font-medium text-gray-700">Upload progress</span>
      <span id="prg-text" class="text-sm font-medium text-gray-700">0%</span>
    </div>
    <div class="w-full bg-gray-200 rounded-full h-2.5">
      <div id="prg-bar" class="bg-blue-600 h-2.5 rounded-full w-0 transition-all duration-300"></div>
    </div>
  </div>
</div>

<script>
  function updateProgress(percent) {
    document.getElementById('prg-text').textContent = percent + '%';
    document.getElementById('prg-bar').style.width = percent + '%';
  }

  $('form').submit(function(e){
    e.preventDefault();
    var form = $('#upload_form')[0];
    var data = new FormData(form);
    $.ajax({
      url: '/update',
      type: 'POST',
      data: data,
      contentType: false,
      processData: false,
      xhr: function() {
        var xhr = new window.XMLHttpRequest();
        xhr.upload.addEventListener('progress', function(evt) {
          if (evt.lengthComputable) {
            var per = evt.loaded / evt.total;
            updateProgress(Math.round(per*100));
          }
        }, false);
        return xhr;
      },
      success: function(d, s) {
        console.log('success!')
      },
      error: function (a, b, c) {
        console.error('Error during upload', a, b, c);
      }
    });
  });
</script>
 )rawliteral";

void initOTA() {
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  server.on(
    "/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    },
    []() {
      HTTPUpload& upload = server.upload();
      // Check password at the beginning of the upload process
      if (upload.status == UPLOAD_FILE_START) {
        // Check if password field exists and matches
        bool isAuthenticated = false;

        // The password is sent as a form field before the file upload begins
        if (server.hasArg("password")) {
          String password = server.arg("password");
          if (password.equals(passwordHash)) {
            isAuthenticated = true;
          }
        }

        if (!isAuthenticated) {
          Update.end(true);
          Serial.println("Authentication failed - incorrect password");
          return;
        }

        // Password is correct, proceed with update
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {  //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {  //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
    });
}