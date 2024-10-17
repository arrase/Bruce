#include "core/globals.h"
#include "core/sd_functions.h" // using sd functions called to rename and manage sd files
#include "core/wifi_common.h"  // using common wifisetup
#include "core/mykeyboard.h"   // using keyboard when calling rename
#include "core/display.h"      // using displayRedStripe as error msg
#include "core/serialcmds.h"
#include "core/passwords.h"
#include "core/settings.h"
#include "webInterface.h"
#include "miniz.h"

File uploadFile;
  // WiFi as a Client
const int default_webserverporthttp = 80;

//WiFi as an Access Point
IPAddress AP_GATEWAY(172, 0, 0, 1);  // Gateway

WebServer* server=nullptr;               // initialise webserver
const char* host = "bruce";
String uploadFolder="";



/**********************************************************************
**  Function: webUIMyNet
**  Display options to launch the WebUI
**********************************************************************/
void webUIMyNet() {
  if (WiFi.status() != WL_CONNECTED) {
    if(wifiConnectMenu()) startWebUi(false);
    else {
      displayError("Wifi Offline");
    }
  } else {
    //If it is already connected, just start the network
    startWebUi(false);
  }
  // On fail installing will run the following line
}


/**********************************************************************
**  Function: loopOptionsWebUi
**  Display options to launch the WebUI
**********************************************************************/
void loopOptionsWebUi() {
  // Definição da matriz "Options"
  options = {
      {"my Network", [=]() { webUIMyNet(); }},
      {"AP mode", [=]()    { startWebUi(true); }},
  };
  delay(200);

  loopOptions(options);
  // On fail installing will run the following line
}


/**********************************************************************
**  Function: humanReadableSize
** Make size of files human readable
** source: https://github.com/CelliesProjects/minimalUploadAuthESP32
**********************************************************************/
String humanReadableSize(uint64_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " kB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}



/**********************************************************************
**  Function: listFiles
**  list all of the files, if ishtml=true, return html rather than simple text
**********************************************************************/
String listFiles(FS fs, bool ishtml, String folder, bool isLittleFS) {
  String returnText = "";
  Serial.println("Listing files stored on SD");

  if (ishtml) {
    returnText += "<table><tr><th align='left'>Name</th><th style=\"text-align=center;\">Size</th><th></th></tr>\n";
  }
  File root = fs.open(folder);
  File foundfile = root.openNextFile();
  String fileSys="SD";
  if (isLittleFS) fileSys="LittleFS";
  if (folder=="//") folder = "/";
  uploadFolder = folder;
  String PreFolder = folder;
  PreFolder = PreFolder.substring(0, PreFolder.lastIndexOf("/"));
  if(PreFolder=="") PreFolder= "/";
  returnText += "<tr><th align='left'><a onclick=\"listFilesButton('"+ PreFolder + "', '"+ fileSys +"')\" href='javascript:void(0);'>... </a></th><th align='left'></th><th></th></tr>\n";

  if (folder=="/") folder = "";
  while (foundfile) {
    if(ESP.getFreeHeap()<1024) break;
    if(foundfile.isDirectory()) {
      if (ishtml) {
        returnText += "<tr align='left'><td><a onclick=\"listFilesButton('"+ String(foundfile.path()) + "', '"+ fileSys +"')\" href='javascript:void(0);'>\n" + String(foundfile.name()) + "</a></td>";
        returnText += "<td></td>\n";
        returnText += "<td><i style=\"color: #ffabd7;\" class=\"gg-folder\" onclick=\"listFilesButton('" + String(foundfile.path()) + "')\"></i>&nbsp&nbsp";
        returnText += "<i style=\"color: #ffabd7;\" class=\"gg-rename\"  onclick=\"renameFile(\'" + String(foundfile.path()) + "\', \'" + String(foundfile.name()) + "\')\"></i>&nbsp&nbsp\n";
        returnText += "<i style=\"color: #ffabd7;\" class=\"gg-trash\"  onclick=\"downloadDeleteButton(\'" + String(foundfile.path()) + "\', \'delete\')\"></i></td></tr>\n\n";
      } else {
        returnText += "Folder: " + String(foundfile.name()) + " Size: " + humanReadableSize(foundfile.size()) + "\n";
      }
    }
    foundfile = root.openNextFile();
  }
  root.close();
  foundfile.close();

  if (folder=="") folder = "/";
  root = fs.open(folder);
  foundfile = root.openNextFile();
  while (foundfile) {
    if(ESP.getFreeHeap()<1024) break;
    if(!(foundfile.isDirectory())) {
      if (ishtml) {
        returnText += "<tr align='left'><td>" + String(foundfile.name());
        returnText += "</td>\n";
        returnText += "<td style=\"font-size: 10px; text-align=center;\">" + humanReadableSize(foundfile.size()) + "</td>\n";
        returnText += "<td><i class=\"gg-arrow-down-r\" onclick=\"downloadDeleteButton(\'"+ String(foundfile.path()) + "\', \'download\')\"></i>&nbsp&nbsp\n";
        //if (String(foundfile.name()).substring(String(foundfile.name()).lastIndexOf('.') + 1).equalsIgnoreCase("bin")) returnText+= "<i class=\"gg-arrow-up-r\" onclick=\"startUpdate(\'" + String(foundfile.path()) + "\')\"></i>&nbsp&nbsp\n";
        if (String(foundfile.name()).substring(String(foundfile.name()).lastIndexOf('.') + 1).equalsIgnoreCase("sub")) returnText+= "<i class=\"gg-data\" onclick=\"sendSubFile(\'" + String(foundfile.path()) + "\')\"></i>&nbsp&nbsp\n";
        if (String(foundfile.name()).substring(String(foundfile.name()).lastIndexOf('.') + 1).equalsIgnoreCase("ir")) returnText+= "<i class=\"gg-data\" onclick=\"sendIrFile(\'" + String(foundfile.path()) + "\')\"></i>&nbsp&nbsp\n";
        if (String(foundfile.name()).substring(String(foundfile.name()).lastIndexOf('.') + 1).equalsIgnoreCase("js")) returnText+= "<i class=\"gg-data\" onclick=\"runJsFile(\'" + String(foundfile.path()) + "\')\"></i>&nbsp&nbsp\n";
        if (String(foundfile.name()).substring(String(foundfile.name()).lastIndexOf('.') + 1).equalsIgnoreCase("bjs")) returnText+= "<i class=\"gg-data\" onclick=\"runJsFile(\'" + String(foundfile.path()) + "\')\"></i>&nbsp&nbsp\n";
        if (String(foundfile.name()).substring(String(foundfile.name()).lastIndexOf('.') + 1).equalsIgnoreCase("bjs")) returnText+= "<i class=\"gg-data\" onclick=\"unzip(\'" + String(foundfile.path()) + "\')\"></i>&nbsp&nbsp\n";
        #if defined(USB_as_HID)
          if (String(foundfile.name()).substring(String(foundfile.name()).lastIndexOf('.') + 1).equalsIgnoreCase("txt")) returnText+= "<i class=\"gg-data\" onclick=\"runBadusbFile(\'" + String(foundfile.path()) + "\')\"></i>&nbsp&nbsp\n";
          if (String(foundfile.name()).substring(String(foundfile.name()).lastIndexOf('.') + 1).equalsIgnoreCase("enc")) returnText+= "<i class=\"gg-data\" onclick=\"decryptAndType(\'" + String(foundfile.path()) + "\')\"></i>&nbsp&nbsp\n";
        #endif
        returnText += "<i class=\"gg-rename\"  onclick=\"renameFile(\'" + String(foundfile.path()) + "\', \'" + String(foundfile.name()) + "\')\"></i>&nbsp&nbsp\n";
        returnText += "<i class=\"gg-trash\"  onclick=\"downloadDeleteButton(\'" + String(foundfile.path()) + "\', \'delete\')\"></i></td></tr>\n\n";
      } else {
        returnText += "File: " + String(foundfile.name()) + " Size: " + humanReadableSize(foundfile.size()) + "\n";
      }
    }
    foundfile = root.openNextFile();
  }
  root.close();
  foundfile.close();

  if (ishtml) {
    returnText += "</table>";
  }

  return returnText;
}

/**********************************************************************
**  Function: processor
** parses and processes webpages if the webpage has %SOMETHING%
** or %SOMETHINGELSE% it will replace those strings with the ones defined
**********************************************************************/

String processor(const String& var) {
  String processedHtml = var;
  processedHtml.replace("%FIRMWARE%", String(BRUCE_VERSION));
  processedHtml.replace("%FREESD%", humanReadableSize(SD.totalBytes() - SD.usedBytes()));
  processedHtml.replace("%USEDSD%", humanReadableSize(SD.usedBytes()));
  processedHtml.replace("%TOTALSD%", humanReadableSize(SD.totalBytes()));

  processedHtml.replace("%FREELittleFS%", humanReadableSize(LittleFS.totalBytes() - LittleFS.usedBytes()));
  processedHtml.replace("%USEDLittleFS%", humanReadableSize(LittleFS.usedBytes()));
  processedHtml.replace("%TOTALLittleFS%", humanReadableSize(LittleFS.totalBytes()));

  return processedHtml;
}


/**********************************************************************
**  Function: checkUserWebAuth
** used by server->on functions to discern whether a user has the correct
** httpapitoken OR is authenticated by username and password
**********************************************************************/
bool checkUserWebAuth() {
  bool isAuthenticated = false;
  if (server->authenticate(wui_usr.c_str(), wui_pwd.c_str())) {
    isAuthenticated = true;
  }
  return isAuthenticated;
}



/**********************************************************************
**  Function: handleUpload
** handles uploads to the filserver
**********************************************************************/
void handleFileUpload(FS fs) {
  HTTPUpload& upload = server->upload();
  String filename = upload.filename;
  if (server->hasArg("password")) filename = filename + ".enc";
  if (upload.status == UPLOAD_FILE_START) {
    if (!filename.startsWith("/")) filename = "/" + filename;
    if (uploadFolder != "/") filename = uploadFolder + filename;
    fs.remove(filename);
    uploadFile = fs.open(filename, "w");
    Serial.println("Upload Start: " + filename);
  } else if (upload.status == UPLOAD_FILE_WRITE && uploadFile) {
      if (server->hasArg("password")) {
        // encryption requested
        static int chunck_no = 0;
        if(chunck_no != 0) {
          // TODO: handle multiple chunks
          server->send(404, "text/html", "file is too big");
          return;
        } else chunck_no += 1;
        String enc_password = server->arg("password");
        // upload to ram, encrypt, then write cypertext
        //Serial.println(enc_password);
        String plaintext = String((char*)upload.buf).substring(0, upload.currentSize);
        //Serial.println(plaintext);
        //Serial.println(upload.currentSize);
        String cyphertxt = encryptString(plaintext, enc_password);
        if(cyphertxt=="") return;
        uploadFile.write((const uint8_t*) cyphertxt.c_str(), cyphertxt.length());
      } else {
        // write directly
        uploadFile.write(upload.buf, upload.currentSize);
      }
  } else if (upload.status == UPLOAD_FILE_END && uploadFile) {
      uploadFile.close();
      Serial.println("Upload End: " + filename);
      server->sendHeader("Location", "/"); // Redireciona para a raiz
      server->send(303);
  }
}
/**********************************************************************
**  Function: drawWebUiScreen
**  Draw information on screen of WebUI.
**********************************************************************/
void drawWebUiScreen(bool mode_ap) {
  tft.fillScreen(BGCOLOR);
  tft.fillScreen(BGCOLOR);
  tft.drawRoundRect(5,5,WIDTH-10,HEIGHT-10,5,ALCOLOR);
  if(mode_ap) {
    setTftDisplay(0,0,BGCOLOR,FM);
    tft.drawCentreString("BruceNet/brucenet",WIDTH/2,7,1);
  }
  setTftDisplay(0,0,ALCOLOR,FM);
  tft.drawCentreString("BRUCE WebUI",WIDTH/2,27,1);
  String txt;
  if(!mode_ap) txt = WiFi.localIP().toString();
  else txt = WiFi.softAPIP().toString();
  tft.setTextColor(FGCOLOR);

  tft.drawCentreString("http://bruce.local", WIDTH/2,45,1);
  setTftDisplay(7,67);

  tft.setTextSize(FM);
  tft.print("IP: ");   tft.println(txt);
  tft.setCursor(7,tft.getCursorY());
  tft.println("Usr: " + String(wui_usr));
  tft.setCursor(7,tft.getCursorY());
  tft.println("Pwd: " + String(wui_pwd));
  tft.setCursor(7,tft.getCursorY());
  tft.setTextColor(TFT_RED);
  tft.setTextSize(FP);

  #ifdef CARDPUTER
  tft.drawCentreString("press Esc to stop", WIDTH/2,HEIGHT-15,1);
  #else
  tft.drawCentreString("press Pwr to stop", WIDTH/2,HEIGHT-15,1);
  #endif
}

/**********************************************************************
**  Function: configureWebServer
**  configure web server
**********************************************************************/
void configureWebServer() {
  MDNS.begin(host);

  // Configura rota padrão para arquivo não encontrado
  server->onNotFound([]() {
    server->send(404, "text/html", "Nothing in here, sharky!");
  });

  // Visitar esta página fará com que você seja solicitado a se autenticar
  server->on("/logout", HTTP_GET, []() {
    server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server->sendHeader("Location", "/logged-out", true); // Redireciona para a página de login
    server->requestAuthentication();
    server->send(302); // Código de status para redirecionamento
  });

  // Página que apresenta que você está desconectado
  server->on("/logged-out", HTTP_GET, []() {
    String logMessage = "Cliente desconectado.";
    Serial.println(logMessage);
    server->send(200, "text/html", logout_html);
  });

  // Uploadfile handler
  server->on("/uploadSD", HTTP_POST, []() {
    server->send(200, "text/plain", "Upload iniciado");
  }, []() {handleFileUpload(SD);});

  // Uploadfile handler
  server->on("/uploadLittleFS", HTTP_POST, []() {
    server->send(200, "text/plain", "Upload iniciado");
  }, []() { handleFileUpload(LittleFS); });

  // Index page
  server->on("/", HTTP_GET, []() {
    if (checkUserWebAuth()) {
      // WIP: custom webui page serving
      /*
      FS* fs = NULL;
      File custom_index_html_file = NONE;
      if(SD.exists("/webui.html")) fs = &SD;
      if(LittleFS.exists("/webui.html")) fs = &LittleFS;
      if(fs) {
        // try to read the custom page and serve that
        File custom_index_html_file =  fs->open("/webui.html", FILE_READ);
        if(custom_index_html_file) {
          // read the whole file
          //server->send(200, "text/html", processor(custom_index_html));
        }
      }
      */
      // just serve the hardcoded page
      server->send(200, "text/html", processor(index_html));
    } else {
      server->requestAuthentication();
    }
  });
  server->on("/style.css", HTTP_GET, []() {
    if (checkUserWebAuth()) {
      server->send_P(200, "text/css", index_css);
    } else {
      server->requestAuthentication();
    }
  });
server->on("/script.js", HTTP_GET, []() {
    if (checkUserWebAuth()) {
      server->send_P(200, "application/javascript", index_js);
    } else {
      server->requestAuthentication();
    }
  });
  // Index page
  server->on("/Oc34N", HTTP_GET, []() {
      server->send(200, "text/html", page_404);
  });

  // Route to rename a file
  server->on("/rename", HTTP_POST, []() {
    if (server->hasArg("fileName") && server->hasArg("filePath"))  {
      String fs = server->arg("fs").c_str();
      String fileName = server->arg("fileName").c_str();
      String filePath = server->arg("filePath").c_str();
      String filePath2 = filePath.substring(0,filePath.lastIndexOf('/')+1) + fileName;
      // Rename the file of folder
      if(fs == "SD") {
        if (SD.rename(filePath, filePath2)) server->send(200, "text/plain", filePath + " renamed to " + filePath2);
        else server->send(200, "text/plain", "Fail renaming file.");
      } else {
        if (LittleFS.rename(filePath, filePath2)) server->send(200, "text/plain", filePath + " renamed to " + filePath2);
        else server->send(200, "text/plain", "Fail renaming file.");
      }

    }
  });

    server->on("/unzip", HTTP_POST, []() {
    if (server->hasArg("filePath"))  {
      String fs = server->arg("fs").c_str();
      String zipFilePath = server->arg("filePath").c_str();
      File zipFile;

      if(fs == "SD") {
        zipFile = SD.open(zipFilePath, FILE_READ);
      } else {
        zipFile = SPIFFS.open(zipFilePath, FILE_READ);
      }
      
      if (!zipFile) {
          Serial.println("Error al abrir el archivo ZIP");
          return;
      }

      // Leer el contenido del archivo ZIP en un buffer
      size_t zipSize = zipFile.size();
      uint8_t* zipBuffer = (uint8_t*)malloc(zipSize);
      if (!zipBuffer) {
          Serial.println("Error al reservar memoria");
          zipFile.close();
          return;
      }
      zipFile.read(zipBuffer, zipSize);
      zipFile.close();

      // Inicializar el archivo ZIP usando miniz
      mz_zip_archive zip_archive;
      memset(&zip_archive, 0, sizeof(zip_archive));

      if (!mz_zip_reader_init_mem(&zip_archive, zipBuffer, zipSize, 0)) {
          Serial.println("Error al inicializar el archivo ZIP");
          free(zipBuffer);
          return;
      }

      // Obtener el número de archivos en el archivo ZIP
      int numFiles = mz_zip_reader_get_num_files(&zip_archive);
      Serial.printf("Número de archivos en el ZIP: %d\n", numFiles);

      // Descomprimir cada archivo del ZIP
      for (int i = 0; i < numFiles; i++) {
          mz_zip_archive_file_stat file_stat;
          if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
              Serial.println("Error al obtener información del archivo en el ZIP");
              continue;
          }

          // Leer contenido del archivo
          size_t uncomp_size;
          void* p = mz_zip_reader_extract_file_to_heap(&zip_archive, file_stat.m_filename, &uncomp_size, 0);
          if (!p) {
              Serial.printf("Error al descomprimir el archivo: %s\n", file_stat.m_filename);
              continue;
          }

          Serial.printf("Archivo: %s, Tamaño descomprimido: %d bytes\n", file_stat.m_filename, uncomp_size);

          // Puedes guardar el contenido descomprimido en SPIFFS, o manejarlo según sea necesario
          // Guardar en SPIFFS:
          File outFile;
          if(fs == "SD") {
            outFile = SD.open(file_stat.m_filename, FILE_WRITE);
          } else {
            outFile = SPIFFS.open(file_stat.m_filename, FILE_WRITE);
          }

          if (outFile) {
              outFile.write((const uint8_t*)p, uncomp_size);
              outFile.close();
          } else {
              Serial.printf("Error al crear archivo descomprimido: %s\n", file_stat.m_filename);
          }

          // Liberar memoria utilizada
          mz_free(p);
      }

      // Finalizar la lectura del archivo ZIP
      mz_zip_reader_end(&zip_archive);

      // Liberar el buffer ZIP
      free(zipBuffer);

      Serial.println("Descompresión completada");
    }
  });

  // Route to send an generic command (Tasmota compatible API) https://tasmota.github.io/docs/Commands/#with-web-requests
  server->on("/cm", HTTP_POST, []() {
    if (server->hasArg("cmnd"))  {
      String cmnd = server->arg("cmnd");
      if( processSerialCommand( cmnd ) ) {
        setup_gpio(); // temp fix for menu inf. loop
        drawWebUiScreen(WiFi.getMode() == WIFI_MODE_AP ? true:false);
        server->send(200, "text/plain", "command " + cmnd + " success");
      } else {
        server->send(400, "text/plain", "command failed, check the serial log for details");
      }
    }
    server->send(400, "text/plain", "http request missing required arg: cmnd");
  });

  // Reinicia o ESP
  server->on("/reboot", HTTP_GET, []() {
    if (checkUserWebAuth()) {
      ESP.restart();
    } else {
      server->requestAuthentication();
    }
  });

  // List files of the LittleFS
  server->on("/listfiles", HTTP_GET, []() {
    if (checkUserWebAuth()) {
      String folder = "/";
      if (server->hasArg("folder")) {
        folder = server->arg("folder");
      }
      bool useSD = false;
      if (strcmp(server->arg("fs").c_str(), "SD") == 0) useSD = true;

      if (useSD) server->send(200, "text/plain", listFiles(SD, true, folder,false));
      else server->send(200, "text/plain", listFiles(LittleFS, true, folder, true));

    } else {
      server->requestAuthentication();
    }
  });

  // define route to handle download, create folder and delete
  server->on("/file", HTTP_GET, []() {
    if (checkUserWebAuth()) {
      if (server->hasArg("name") && server->hasArg("action")) {
        String fileName = server->arg("name").c_str();
        String fileAction = server->arg("action").c_str();
        String fileSys = server->arg("fs").c_str();
        bool useSD = false;
        if (fileSys == "SD") useSD = true;

        FS *fs;
        if (useSD) fs = &SD;
        else fs = &LittleFS;

        log_i("filename: %s", fileName);
        log_i("fileAction: %s", fileAction);

        if (!(*fs).exists(fileName)) {
          if (strcmp(fileAction.c_str(), "create") == 0) {
            if ((*fs).mkdir(fileName)) {
              server->send(200, "text/plain", "Created new folder: " + String(fileName));
            } else {
              server->send(200, "text/plain", "FAIL creating folder: " + String(fileName));
            }
          } else server->send(400, "text/plain", "ERROR: file does not exist");

        } else {
          if (strcmp(fileAction.c_str(), "download") == 0) {
            File downloadFile = (*fs).open(fileName, FILE_READ);
            if (downloadFile) {
              String contentType = "application/octet-stream";
              server->setContentLength(downloadFile.size());
              server->sendHeader("Content-Type", contentType, true);
              server->sendHeader("Content-Disposition", "attachment; filename=\"" + String(downloadFile.name()) + "\"");
              server->streamFile(downloadFile, contentType);
              downloadFile.close();
            } else {
              server->send(500, "text/plain", "Failed to open file for reading");
            }
          } else if (strcmp(fileAction.c_str(), "delete") == 0) {
            if (deleteFromSd(*fs, fileName)) {
              server->send(200, "text/plain", "Deleted : " + String(fileName));
            } else {
              server->send(200, "text/plain", "FAIL deleting: " + String(fileName));
            }
          } else if (strcmp(fileAction.c_str(), "create") == 0) {
            if (SD.mkdir(fileName)) {
              server->send(200, "text/plain", "Created new folder: " + String(fileName));
            } else {
              server->send(200, "text/plain", "FAIL creating folder: " + String(fileName));
            }
          } else {
            server->send(400, "text/plain", "ERROR: invalid action param supplied");
          }
        }
      } else {
        server->send(400, "text/plain", "ERROR: name and action params required");
      }
    } else {
      server->requestAuthentication();
    }
  });

  // Configuração de Wi-Fi via página web
  server->on("/wifi", HTTP_GET, []() {
    if (checkUserWebAuth()) {
      if (server->hasArg("usr") && server->hasArg("pwd")) {
        const char *usr = server->arg("usr").c_str();
        const char *pwd = server->arg("pwd").c_str();
        wui_usr= usr;
        wui_pwd = pwd;
        saveConfigs();
        server->send(200, "text/plain", "User: " + String(usr) + " configured with password: " + String(pwd));
      }
    } else {
      server->requestAuthentication();
    }
  });
  server->begin();
}
/**********************************************************************
**  Function: startWebUi
**  Start the WebUI
**********************************************************************/
void startWebUi(bool mode_ap) {
  setupSdCard();

  if (WiFi.status() != WL_CONNECTED) {
    // Choose wifi access mode
    wifiConnectMenu(mode_ap);
  }

  // configure web server
  Serial.println("Configuring Webserver ...");
  if(psramFound()) server=(WebServer*)ps_malloc(sizeof(WebServer));
  else server=(WebServer*)malloc(sizeof(WebServer));

  new (server) WebServer(default_webserverporthttp);

  configureWebServer();
  drawWebUiScreen(mode_ap);

  disableCore0WDT();
  disableCore1WDT();
  disableLoopWDT();
  options.clear(); // Clear this vector to free stack memory

  while (!checkEscPress()) {
      server->handleClient();
      // nothing here, just to hold the screen until the server is on.
  }
  server->close();
  server->~WebServer();
  free(server);
  server = nullptr;
  MDNS.end();

  delay(100);
  wifiDisconnect();

}