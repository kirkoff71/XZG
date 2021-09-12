const char HTTP_HEADER[] PROGMEM =
    "<head>"
    "<script type='text/javascript' src='web/js/jquery-min.js'></script>"
    "<script type='text/javascript' src='web/js/bootstrap.min.js'></script>"
    "<script type='text/javascript' src='web/js/functions.js'></script>"
    "<link href='web/css/bootstrap.min.css' rel='stylesheet' type='text/css' />"
    "<link href='web/css/glyphicons.css' rel='stylesheet' type='text/css' />"
    "<link href='web/css/style.css' rel='stylesheet' type='text/css' />"
    " </head>"
    "<body>"
    "<nav class='navbar navbar-expand-lg navbar-light bg-info rounded'><a class='navbar-brand' href='/'><img src='web/img/logo.png'/> <strong>ZigStar GW</strong>"
    "</a>"
    "<button class='navbar-toggler' type='button' data-toggle='collapse' data-target='#navbarNavDropdown' aria-controls='navbarNavDropdown' aria-expanded='false' aria-label='Toggle navigation'>"
    "<span class='navbar-toggler-icon'></span>"
    "</button>"
    "<div id='navbarNavDropdown' class='collapse navbar-collapse justify-content-md-end'>"
    "<ul class='navbar-nav'>"
    "<li class='nav-item'>"
    "<a class='nav-link' href='/'><i class='glyphicon glyphicon-home'></i>Status</a>"
    "</li>"
    "<li class='nav-item dropdown'>"
    "<a class='nav-link dropdown-toggle' href='#' id='navbarDropdown' role='button' data-toggle='dropdown' aria-haspopup='true' aria-expanded='false'><i  class='glyphicon glyphicon-cog'></i>Config</a>"
    "<div class='dropdown-menu' aria-labelledby='navbarDropdown'>"
    "<a class='dropdown-item' href='/general'><i class='glyphicon glyphicon-list-alt'></i>General</a>"
    "<a class='dropdown-item' href='/serial'><i class='glyphicon glyphicon-wrench'></i>Serial</a>"
    "<a class='dropdown-item' href='/ethernet'><i class='glyphicon glyphicon-globe'></i>Ethernet</a>"
    "<a class='dropdown-item' href='/wifi'><i class='glyphicon glyphicon-signal'></i>WiFi</a>"
    "</div>"
    "</li>"
    "<li class='nav-item dropdown'>"
    "<a class='nav-link dropdown-toggle' href='#' id='navbarDropdown' role='button' data-toggle='dropdown' aria-haspopup='true' aria-expanded='false'><i  class='glyphicon glyphicon-wrench'></i>Tools</a>"
    "<div class='dropdown-menu' aria-labelledby='navbarDropdown'>"
    "<a class='dropdown-item' href='/logs'><i  class='glyphicon glyphicon-transfer'></i>Console</a>"
    "<a class='dropdown-item' href='/fsbrowser'><i class='glyphicon glyphicon-floppy-disk'></i>FSbrowser</a>"
    "<a class='dropdown-item' href='/esp_update'><i class='glyphicon glyphicon-open'></i>Update ESP32</a>"
    "<a class='dropdown-item' href='/updates'><i class='glyphicon glyphicon-open'></i>Update Zigbee</a>"
    "<a class='dropdown-item' href='/reboot'><i class='glyphicon glyphicon-repeat'></i>Reboot</a>"
    "</div>"
    "</li>"
    "<li class='nav-item'>"
    "<a class='nav-link' href='/help'><i class='glyphicon glyphicon-info-sign'></i>Help</a>"
    "</li>"
    "</ul></div>"
    "</nav>";

const char HTTP_WIFI[] PROGMEM =

    "<h2>Config WiFi</h2>"
    "<div class='row justify-content-md-center' >"
    "<div class='col-sm-6'><form method='POST' action='saveWifi'>"
    "<div class='form-check'>"

    "<input class='form-check-input' id='wifiEnable' type='checkbox' name='wifiEnable' {{checkedWiFi}}>"
    "<label class='form-check-label' for='wifiEnable'>Enable</label>"
    "</div>"
    "<div class='form-group'>"
    "<label for='ssid'>SSID</label>"
    "<input class='form-control' id='ssid' type='text' name='WIFISSID' value='{{ssid}}'> <a onclick='scanNetwork();' class='btn btn-primary mb-2'>Scan</a><div id='networks'></div>"
    "</div>"
    "<div class='form-group'>"
    "<label for='pass'>Password</label>"
    "<input class='form-control' id='pass' type='password' name='WIFIpassword' value=''>"
    "</div>"
    "<div class='form-check'>"
    "<input class='form-check-input' id='dhcpWiFi' type='checkbox' name='dhcpWiFi' {{modeWiFi}}>"
    "<label class='form-check-label' for='dhcpWiFi'>DHCP</label>"
    "</div>"
    "<div class='form-group'>"
    "<label for='ip'>IP</label>"
    "<input class='form-control' id='ip' type='text' name='ipAddress' value='{{ip}}'>"
    "</div>"
    "<div class='form-group'>"
    "<label for='mask'>Mask</label>"
    "<input class='form-control' id='mask' type='text' name='ipMask' value='{{mask}}'>"
    "</div>"
    "<div class='form-group'>"
    "<label for='gateway'>Gateway</label>"
    "<input type='text' class='form-control' id='gateway' name='ipGW' value='{{gw}}'>"
    "</div>"
    "<button type='submit' class='btn btn-primary mb-2' name='save'>Save</button>"
    "</form>";

const char HTTP_SERIAL[] PROGMEM =
    "<h2>Config Serial</h2>"
    "<div class='row justify-content-md-center' >"
    "<div class='col-sm-6'><form method='POST' action='saveSerial'>"

    "<div class='form-group'>"
    "<label for='baud'>Serial Speed</label>"
    "<select class='form-control' id='baud' name='baud'>"
    "<option value='9600' {{selected9600}}>9600 bauds</option>"
    "<option value='19200' {{selected19200}}>19200 bauds</option>"
    "<option value='38400' {{selected38400}}>38400 bauds</option>"
    "<option value='57600' {{selected57600}}>57600 bauds</option>"
    "<option value='115200' {{selected115200}}>115200 bauds</option>"
    "</select>"
    "<label for='port'>Socket Port</label>"
    "<input class='form-control' id='port' type='number' name='port' min='100' max='65000' value='{{socketPort}}'>"
    "</div>"
    "<br><br>"
    "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
    "</form>";

const char HTTP_HELP[] PROGMEM =
    "<h2>Help</h2>"
    "<div class='row justify-content-md-center'>"
    "<div class='col-sm-6'>"
    "<p><img id='photo'  loading='lazy' src='https://zig-star.com/images/zigbee-gw-lan/Default.png'></p>"
    "<h2 id='features'>Features:</h2>"
    "<ul>"
    "<li>Wide range of Zigbee Modules (CC2652P, CC2652RB, EFR32MG1B, EFR32MG21, etc)</li>"
    "<li><b> 2 WORKING MODES: </b> LAN Coordinator or USB Coordinator/Router</li>"
    "<li>BOOT,FLASH,RESET Buttons</li>"
    "<li>2 LED for indication( only RF-BM-2652P2 ) <a href='https://github.com/jethome-ru/zigbee-firmware/tree/master/ti/coordinator/cc2652' target='_blank'>Custom firmware</a></li>"
    "<li>Communicates via USB or LAN network with <a href='https://www.zigbee2mqtt.io/' target='_blank'>Z2M</a> and <a href='https://www.home-assistant.io/integrations/zha/' target='_blank'>ZHA</a></li>"
    "<li>SMA antenna port for an external antenna</li>"
    "<li>Programming via <a href='https://github.com/JelmerT/cc2538-bsl' target='_blank'>cc2538-bsl</a> . No external programmer needed!Push BSL button or trigger flash mode using Web Interface</li>"
    "<li>Plug &amp; Play, comes pre-flashed</li>"
    "</ul>"
    "</div>"
    "</div>";

const char HTTP_ETHERNET[] PROGMEM =
    "<h2>Config Ethernet</h2>"
    "<div class='row justify-content-md-center' >"
    "<div class='col-sm-6'><form method='POST' action='saveEther'>"
    "<div class='form-check'>"

    "<input class='form-check-input' id='dhcp' type='checkbox' name='dhcp' {{modeEther}}>"
    "<label class='form-check-label' for='dhcp'>DHCP</label>"
    "</div>"
    "<div class='form-group'>"
    "<label for='ip'>IP</label>"
    "<input class='form-control' id='ip' type='text' name='ipAddress' value='{{ipEther}}'>"
    "</div>"
    "<div class='form-group'>"
    "<label for='mask'>Mask</label>"
    "<input class='form-control' id='mask' type='text' name='ipMask' value='{{maskEther}}'>"
    "</div>"
    "<div class='form-group'>"
    "<label for='gateway'>Gateway</label>"
    "<input type='text' class='form-control' id='gateway' name='ipGW' value='{{GWEther}}'>"
    "</div>"
    "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
    "</form>";

const char HTTP_GENERAL[] PROGMEM =
    "<h2>General</h2>"
    "<div class='row justify-content-md-center' >"
    "<div class='col-sm-6'><form method='POST' action='saveGeneral'>"
    "<div class='form-group'>"
    "<label for='hostname'>Hostname</label>"
    "<input class='form-control' id='hostname' type='text' name='hostname' value='{{hostname}}'>"
    "</div>"
    "<div class='form-check'>"
    "<input class='form-check-input' id='disableWeb' type='checkbox' name='disableWeb' {{disableWeb}}>"
    "<label class='form-check-label' for='disableWeb'>Disable web server when socket is connected</label>"
    "<br>"
    "<input class='form-check-input' id='enableHeartBeat' type='checkbox' name='enableHeartBeat' {{enableHeartBeat}}>"
    "<label class='form-check-label' for='enableHeartBeat'>Enable HeartBeat (send ping to TCP when no trafic)</label>"
    "<br>"
    "<label for='refreshLogs'>Refresh console log</label>"
    "<input class='form-control' id='refreshLogs' type='text' name='refreshLogs' value='{{refreshLogs}}'>"
    "<br>"
    "</div>"
    "<button type='submit' class='btn btn-primary mb-2' name='save'>Save</button>"
    "</form></div>"
    "</div>";

const char HTTP_ROOT[] PROGMEM =
    "<h2>Status</h2>"
    "<div class='row justify-content-md-center'>"
    "<div class='col-sm-6'>"
    "<div class='card'>"
    "<div class='card-header'>General</div>"
    "<div class='card-body'>"
    "<div id='genConfig'>"
    "<strong>Socket connected : </strong>{{connectedSocket}}"
    "<br><strong>Uptime : </strong>{{uptime}}"
    "<br><strong>Version : </strong>" VERSION
    "</div>"
    "</div>"
    "</div>"
    "</div>"
    "</div>"
    "<div class='row justify-content-md-center'>"
    "<div class='col-sm-6'>"
    "<div class='card'>"
    "<div class='card-header'>Ethernet</div>"
    "<div class='card-body'>"
    "<div id='ethConfig'>"
    "<strong>Connected : </strong>{{connectedEther}}"
    "<br><strong>Mode : </strong>{{modeEther}}"
    "<br><strong>IP : </strong>{{ipEther}}"
    "<br><strong>Mask : </strong>{{maskEther}}"
    "<br><strong>GW : </strong>{{GWEther}}"
    "</div>"
    "</div>"
    "</div>"
    "</div>"
    "</div>"
    "<div class='row justify-content-md-center'>"
    "<div class='col-sm-6'>"
    "<div class='card'>"
    "<div class='card-header'>Wifi</div>"
    "<div class='card-body'>"
    "<div id='wifiConfig'>"
    "<strong>Enable : </strong>{{enableWifi}}"
    "<br><strong>SSID : </strong>{{ssidWifi}}"
    "<br><strong>Mode : </strong>{{modeWiFi}}"
    "<br><strong>IP : </strong>{{ipWifi}}"
    "<br><strong>Mask : </strong>{{maskWifi}}"
    "<br><strong>GW : </strong>{{GWWifi}}"
    "</div>"
    "</div>"
    "</div>"
    "</div>"
    "</div>";

const char HTTP_UPDATE[] PROGMEM =
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
    "<label id='file-input' for='file'>   Choose file...</label>"
    "<input type='submit' class='btn btn-primary mb-2' value='Update'>"
    "<br><br>"
    "<div id='prg'></div>"
    "<br><div id='prgbar'><div id='bar'></div></div><br></form>"
    "<script>"
    "function sub(obj){"
    "var fileName = obj.value.split('\\\\');"
    "document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
    "};"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    "$.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "$('#bar').css('width',Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!') "
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>";