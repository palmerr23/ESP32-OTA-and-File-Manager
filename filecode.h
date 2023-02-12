/*
 * filesystem code - local and via webserver
*/
#ifndef FILECODE_H_
#define FILECODE_H_

void fsList(void);
bool initFS(bool format, bool force);

String logStr = "Session start:\n";
char tempBuf[256];

#if FILESYSTYPE == LITTLE_FS
	#include "LittleFS.h"
	#define FILESYS LittleFS
  char fsName[] = "LittleFS";
#else
	#include "FS.h"
  #include <SPIFFS.h>
	#define FILESYS SPIFFS
  char fsName[] = "SPIFFS";
#endif

//holds the current upload
File fsUploadFile;

void handleFileSysFormat() {
	FILESYS.format();
	server.send(200, "text/json", "format complete");
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  Serial.printf_P(PSTR("handleFileRead: %s\r\n"), path.c_str());
  if(path.endsWith("/")) path += "";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(FILESYS.exists(pathWithGz) || FILESYS.exists(path))
	{
    if(FILESYS.exists(pathWithGz))
      path += ".gz";
    File file = FILESYS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
		Serial.println("Read OK");
    return true;
  }
	Serial.printf("Read failed '%s', type '%s'\n", path.c_str(), contentType.c_str()) ;
  return false;
}

// uses edit mode uploader
void handleFileUpload(){
  bool OK = false;
  if(server.uri() != "/edit") return;
 
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.printf_P(PSTR("handleFileUpload Name: %s\r\n"), filename.c_str());
    fsUploadFile = FILESYS.open(filename, "w");
    filename = String();
  } 
  else if(upload.status == UPLOAD_FILE_WRITE)
  {
    Serial.printf_P(PSTR("handleFileUpload Data: %d\r\n"), upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } 
  else if(upload.status == UPLOAD_FILE_END)
  {
    if(fsUploadFile)
      fsUploadFile.close();    
    OK = true;
    Serial.printf_P(PSTR("handleFileUpload Size: %d\r\n"), upload.totalSize);
    sprintf(tempBuf,"File upload [%s] %s\n", upload.filename.c_str(), (OK)? "OK" : "failed");
    logStr += tempBuf;
  }
}

void handleFileDelete(){
	 if(!server.hasArg("file")) {server.send(500, "text/html", "<meta http-equiv='refresh' content='1;url=/main'>Bad arguments. <a href=/main>Back to list</a>"); return;}
  //if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");  
	String path = server.arg("file");
	//String path = server.arg(0);
  Serial.printf_P(PSTR("handleFileDelete: '%s'\r\n"),path.c_str());
  if(path == "/")
    return server.send(500, "text/html", "<meta http-equiv='refresh' content='1;url=/main>Can't delete root directory. <a href=/main>Back to list</a>");
  if(!FILESYS.exists(path))
    return server.send(200, "text/html", "<meta http-equiv='refresh' content='1;url=/main'>File not found. <a href=/main>Back to list</a>");
  FILESYS.remove(path);
  server.send(200, "text/html", "<meta http-equiv='refresh' content='1;url=/main'>File deleted. <a href=/main>Back to list</a>");
  logStr += "Deleted ";
  logStr += path;
  logStr +="\n";
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.printf_P(PSTR("handleFileCreate: %s\r\n"),path.c_str());
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(FILESYS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  if(!path.startsWith("/")) path = "/"+path;    // is this needed for LittleFS?
  File file = FILESYS.open(path, "w");
  if(file)
	{
    file.close();
		Serial.printf("Created file [%s]\n",path.c_str());
	}
  else
	{
		Serial.printf("Create file [%s] failed\n",path.c_str());
    return server.send(500, "text/plain", "CREATE FAILED");
	}
  server.send(200, "text/html", "<meta http-equiv='refresh' content='1;url=/main'>File created. <a href=/main>Back to list</a>");
  logStr += "Created ";
  logStr += path;
  logStr +="\n";
  path = String();
}

// main page JS scripts 
const char edit_script[] PROGMEM = R"rawliteral(
<script>
  var fName; 
  function doEdit(item) 
  {
    console.log('clicked', item); 
    fName = item;
    var fe = document.getElementById("nameSave");
    fe.value = fName;
  } 
  function saveFile()
  {
    console.log('Save', fName);
  }
  function scrollToBottom(el) // log window
  {
    //console.log("scrolling", el);
    var element = document.getElementById(el);
    element.scrollTop = element.scrollHeight;   
  }
</script>
)rawliteral";

// generate HTML for main page
void handleMain() {
  bool foundText = false;
  bool foundName = false;
  bool foundMode = false;
  bool foundSaveBut = false;
  char filebuf[FILEBUFSIZ];
  char fileName[128];
  File file;
	String path = "", bText = "", bName = "", bMode ="";
  String output = "";
  // check arguments
  if(server.hasArg("mode"))
  {
    bMode = server.arg("mode");
    if(bMode.length() > 0) 
      foundMode = true;
    Serial.printf("Mode %s\n",bMode.c_str());
  }
  if(server.hasArg("dir"))// {server.send(500, "text/plain", "BAD ARGS"); return;}  
		path = server.arg("dir");
	else
		path ="/";
  if(server.hasArg("editBlock"))
  {
    bText = server.arg("editBlock");
    if(bText.length() > 0) 
      foundText = true;
  }
  
  if(server.hasArg("nameSave"))
  {
    bName = server.arg("nameSave");
    if(bName.length() > 0) 
      foundName = true;
    if(!bName.startsWith("/")) bName = "/"+ bName;    // is this needed for LittleFS?   
  }
  
  if(server.hasArg("saveBut"))
  {
    foundSaveBut = true;
  }

  // write
  if(foundName && foundText && bMode == "save")
  {
    Serial.println("something to save");
    file = FILESYS.open(bName, "w");
    if(file)
    {
      file.write((uint8_t *)bText.c_str(), bText.length());
      file.close();
      logStr += "Saved ";
      logStr += bName;
      logStr +="\n";
    }  
  }
  Serial.printf_P(PSTR("handleMain: path [%s]\r\n"), path.c_str());
  Serial.printf_P(PSTR("fname:[%s], %i\r\n"), bName.c_str(), foundName);
  Serial.printf_P(PSTR("text [%s], %i\r\n"),  bText.c_str(), foundText);
  Serial.printf_P(PSTR("mode [%s], %i\r\n"),  bMode.c_str(), foundMode);

  File dir = FILESYS.open(path.c_str());
  if(!dir)
	  Serial.printf("Directory [%s]not found", path.c_str());
	else if(!dir.isDirectory())
        Serial.println(" - not a directory");
	//path = String();

  // Create HTML page
  output = "<html><head>";
  output += penguinIco;
  output += "</head><body onload='scrollToBottom(\"log\")'>\n";
  output += "<span style='text-align: center;'><h2>ESP32 OTA and File Management</H2></span>";
  output += "<table style='margin-left: auto;  margin-right: auto;border-collapse: collapse'>\n"; // style='border: 1px solid silver; border-collapse: collapse;'
  
  // OTA 
  output += "<tr><td style='background-color: #e0ffff; border: 1px solid silver; padding: 5px; vertical-align:top;' colspan='2'>";
  output += otaHTML;
  output += "</td></tr>\n";
  
  // FS format 
   if(bMode == "format" && foundMode)
   {
      fsFound = initFS(true, true);
      Serial.println("main: Done formatting");
      logStr += "Formatted FS ";
      logStr += fsName;
      logStr +="\n";      
   }
   output += "<tr><td style='background-color: #fff0ff; border: 1px solid silver;  padding: 5px;vertical-align:top;' colspan='2'><h3>";
   output += fsName;
   output += " file system</h3>";
   output += fsName;
   output += " filesystem ";
   if (!fsFound)
      output += "not ";
   output += "found.";
   // format form
   output += "<span style='text-align: right;'><form action='/main?mode=format' method='get' enctype='multipart/form-data'>"; 
   output += "<input type='hidden' name='mode' value='format'>";
   output += "<button>Format FS</button>";
   output += "</form></span>";  
   output += "</td></tr>\n";
   
   //file upload 
   output += "<tr><td style='background-color: #e4ffe4; border: 1px solid silver;  padding: 5px;vertical-align:top;' colspan='2'><h3>";
   output += "<h3>Upload files</h3><form action='/edit' method='post' enctype='multipart/form-data'><BR>"; // use post for PUT /edit server handler
   output += "Name: <input type='file' name='data' required>";
   output += "Path: <input type='text' name='path' value='/'>";
   output += "<input type='hidden' name='mode' value='upload'>";
   output += "<button>Upload</button>";
   output += "</form>";
   output += "</td></tr>\n";
  
  // file list and edit   
  output += "<tr style='background-color: #ffffde; border: 1px solid silver;'><td style='padding:5px; vertical-align:top;'>";
	output += "<h3>Files in directory '" + path + "'</h3>";
	if(FILESYSTYPE == 0)
		output += "<a href=/main>Back to root</a><br>"; // LittleFS only
	File entry;
  while(entry = dir.openNextFile())
  {  
	  bool isDir = entry.isDirectory(); // single level for SPIFFS, possibly not true for LittleFS
   // output += (isDir) ? "dir:" : "file: ";
   // output += "\t";
		if(isDir) 
		{
			output += "<a href=/main?dir=/" ;
			output +=  entry.name(); 
			output +=  ">";
		}
    strcpy(fileName, entry.name());
    output += String(entry.name());
		if(isDir) 
		  output += "</a>";
		output += " (";
    output += String(entry.size());
		output += ")&nbsp;&nbsp;";
    // edit
    output += "<a href=/main?mode=edit&nameSave="; 
    if(fileName[0] != '\\' && fileName[0] != '/') // avoid double \ or / in filename (on some OS)
   	 output += path;
    output += String(entry.name());
    output += ">Edit</a>&nbsp;&nbsp;";
    // delete
    output += "<a href=/delete?file="; 
    if(fileName[0] != '\\' && fileName[0] != '/') // avoid double \ or / in filename (on some OS)
	output += path;
    output += String(entry.name());
    output += ">Delete</a><BR>";
    entry.close();
  }	
   output += "</td>\n";  
   // edit form - text, filename and submit
   output += "<td style='padding: 5px;'><form action='/main' method='get'><textarea name=editBlock id=editBlock rows='30' cols='60'>";
  
   if(bMode = "edit")
   {
      // read file and insert content
      file = FILESYS.open(bName.c_str(), "r");
      if(file)
      {        
        Serial.printf("File read avail %i, ",file.available());
        int readlen = (file.available() < FILEBUFSIZ) ? file.available() : FILEBUFSIZ;   
        file.read((uint8_t *)filebuf, readlen);
        file.close();        
        filebuf[readlen] = '\0';
        output += filebuf;
        //Serial.printf("read len %i, text [%s]\n",readlen, filebuf);
        logStr += (foundSaveBut) ? "Saving " : "Editing ";
        logStr += bName;
        logStr +="\n";   
      }
   }
   output += "</textarea><BR>\n";
   output += "<input type=hidden name='mode' value='save'>";
   output += "Filename: <input type=text value='";
   if(bMode = "edit")
     output += bName;        
   output += "' name=nameSave id=nameSave> <input type=submit name=saveBut onclick=saveFile() value='Save'></form></td></tr>\n";
   output += "<tr style='background-color: #ececff; border: 1px solid silver; vertical-align:top;'><td colspan=2 style='padding:5px; vertical-align:top;'>";
   output += "Log: <form><textarea  name=log id=log rows='5' cols='85'>";
   output += logStr;
   output += "</textarea></form>";
   output += "<BR><a href='/main'>Reload page</a>";
   output += "</td></tr></table></body>";
   output += edit_script;
   output += "</html>";
  //server.send(200, "text/json", output);
	server.send(200, "text/html", output);
}

bool initFS(bool format = false, bool force = false) 
{
 bool fsFound = FILESYS.begin();
   if(!fsFound) 
     Serial.println(F("No file system found. Please format it."));
     
   if(!format)
     return fsFound;
     
  // format
	if(!fsFound || force) 
	{      
    Serial.println(F("Formatting FS."));
	  if(FILESYS.format()) 
    {
			  if(FILESYS.begin())
        {
          Serial.println(F("Format complete."));
          return true;
        }
    }      
    Serial.println(F("Format failed."));
    return false;              
	}
	//fsList();			
  return false; // shouldn't get here
}

#endif
