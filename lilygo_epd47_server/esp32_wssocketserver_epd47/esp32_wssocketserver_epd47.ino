#include <ArduinoJson.h>
#include "epd_driver.h"
#include <WebSocketsServer.h>
#include "hz3500_36.h"
#include <SPIFFS.h>
#include "Button2.h";


#define FILESYSTEM SPIFFS
#define TXTDATA_FILENAME "/config.data"

#define BUTTON_SAVE_DATA_PIN  39
#define BUTTON_LOAD_DATA_PIN 34
#define BUTTON_DELETE_DATA_PIN 35


const char *ssid = "CMCC-r3Ff";
const char *password =  "9999900000";

WebSocketsServer webSocket = WebSocketsServer(81);

//有一行数据不要，定义7，实际显示6行数据
const int TXT_LIST_NUM = 7;
String txt_list[TXT_LIST_NUM];
int txt_list_index = 0;

Button2 BUTTON_SAVE_DATA = Button2(BUTTON_SAVE_DATA_PIN);
Button2 BUTTON_LOAD_DATA = Button2(BUTTON_LOAD_DATA_PIN);
Button2 BUTTON_DELETE_DATA = Button2(BUTTON_DELETE_DATA_PIN);


void init_txt_list()
{
  int i;
  for ( i = 0; i < TXT_LIST_NUM; i++)
    txt_list[i] = "";
  txt_list_index = 0;
}

void append_txt_list(String txt)
{
  txt_list[txt_list_index] = txt;
  txt_list_index = txt_list_index + 1;
  if (txt_list_index > TXT_LIST_NUM - 1)
    txt_list_index = 0;

  //当前此行清空
  txt_list[txt_list_index] = "";
}


//文字显示
void Show_hz(String rec_text, bool loadbutton)
{

  Serial.println("Showhz:" + rec_text);

  epd_poweron();
  volatile uint32_t t1 = millis();
  epd_clear();
  volatile uint32_t t2 = millis();
  //printf("EPD clear took %dms.\n", t2 - t1);
  int cursor_x = 10;
  int cursor_y = 80;

  //多行文本换行显示算法。
  if (!loadbutton)
    append_txt_list(rec_text);

  String now_string = "";
  int i;
  int now_index = txt_list_index + 1;
  //当前行不要，所以最终会少一行数据
  for ( i = 0; i < TXT_LIST_NUM-1; i++)
  {
    now_string = txt_list[(now_index + i) % TXT_LIST_NUM];
    //Serial.println("line:" + String((now_index + i) % TXT_LIST_NUM) + " " + now_string);

    if (now_string.length() > 0)
    {
      //加">"字符，规避epd47的bug,当所有字库不在字库时，esp32会异常重启
      // “Guru Meditation Error: Core 1 panic'ed (LoadProhibited). Exception was unhandled."
      now_string = ">" + now_string;
      //墨水屏writeln不支持自动换行
      //Serial.println("writeln:" + now_string);
      //delay(200);
      writeln((GFXfont *)&msyh36, (char *)now_string.c_str(), &cursor_x, &cursor_y, NULL);
      //writeln调用后，cursor_x会改变，需要重新赋值
      cursor_x = 10;
      cursor_y = cursor_y + 85;
    }
  }
  //delay(500);
  epd_poweroff();
}


//num 连接身份编号
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    //客户连接
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        //pinStatus();
        webSocket.broadcastTXT("welcome you!");
      }
      break;
    //收到文本
    case WStype_TEXT: {
        Serial.printf("[%u] get Text: %s\n", num, payload);
        if (length > 0)
          Show_hz(String((char *)payload), false);
        webSocket.broadcastTXT(">");
        break;
      }
  }
}

void connectwifi()
{
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.disconnect();
  delay(200);


  WiFi.config(IPAddress(192, 168, 1, 200), //设置静态IP位址
              IPAddress(192, 168, 1, 1),
              IPAddress(255, 255, 255, 0),
              IPAddress(192, 168, 1, 1)
             );

  //  WiFi.config(IPAddress(192, 168, 0, 101), //设置静态IP位址
  //              IPAddress(192, 168, 0, 1),
  //              IPAddress(255, 255, 255, 0),
  //              IPAddress(192, 168, 0, 1)
  //             );

  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WIFI");
  int lasttime = millis() / 1000;
  WiFi.begin(ssid, password);

  while ((!(WiFi.status() == WL_CONNECTED))) {
    delay(1000);
    Serial.print(".");

    //5分钟连接不上，自动重启
    if ( abs(millis() / 1000 - lasttime ) > 300 )
    {
      ets_printf("reboot\n");
      esp_restart();
    }
  }
  Serial.println("Connected");
  Serial.println("My Local IP is : ");
  Serial.println(WiFi.localIP());
}

String  readData(char * myFileName) {
  uint8_t readbuff[513];
  long readnum = 0;
  File file = SPIFFS.open(myFileName, FILE_READ);
  readnum = file.read(readbuff, 512);
  file.close();
  readbuff[readnum] = '\0';

  Serial.println("readData=" +  String((char *)readbuff));
  return String((char *)readbuff);
}

int load_list()
{
  int i;
  if (!FILESYSTEM.exists(TXTDATA_FILENAME)) {
    Serial.println("load configure fail");
    return -1;
  }
  StaticJsonBuffer<513> jsonBuffer;
  String tmp = readData(TXTDATA_FILENAME);
  JsonArray& array =  jsonBuffer.parseArray(tmp);
  char * str1;
  int last_index = -1;
  for ( i = 0; i < TXT_LIST_NUM - 1; i++)
  {
    str1 = (char *)array.get<char*>(i);

    txt_list[i] = String(str1);
    if (txt_list[i].length() > 0)
      last_index = i;

    //Serial.println("txt_list[" + String(i) + "]=" + txt_list[i]);
  }
  txt_list_index =  TXT_LIST_NUM - 1;
  return last_index;
}

void save_list()
{
  int i;
  File file = FILESYSTEM.open(TXTDATA_FILENAME, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }
  StaticJsonBuffer<513> jsonBuffer;
  JsonArray& array = jsonBuffer.createArray();
  String now_string;
  int now_index = txt_list_index + 1;
  //当前行不要，所以最终会少一行数据
  int add_line = 0;
  for ( i = 0; i < TXT_LIST_NUM - 1; i++)
  {
    now_string = txt_list[(now_index + i) % TXT_LIST_NUM];
    Serial.println("line:" + String((now_index + i) % TXT_LIST_NUM) + " " + now_string);
    //array.add( now_string.c_str());
    if (now_string.length() > 0)
    {
      array.add( txt_list[(now_index + i) % TXT_LIST_NUM]);
      add_line = add_line + 1;
    }
  }

  //没有数据项的填空
  for ( i = 0; i < TXT_LIST_NUM - 1 - add_line; i++)
  {
    array.add("");
  }

  array.printTo(now_string);
  Serial.println("save_list:" + now_string);

  if (array.printTo(file) == 0)
  {
    Serial.println(F("Failed to write to file"));
  }
  file.close();
}

void tap(Button2& btn) {
  if (btn == BUTTON_SAVE_DATA)
  {
    save_list();
    Serial.println("txt_list_index=" + String(txt_list_index));
    Serial.println("BUTTON_SAVE_DATA OK!");
  }
  else if (btn == BUTTON_LOAD_DATA)
  {
    int last_index = load_list();
    if (last_index > -1)
    {
      txt_list_index = last_index + 1;
      Show_hz("", true);
    }
    else
    {
      txt_list_index = 0;
      Show_hz("", true);
    }
    Serial.println("txt_list_index=" + String(txt_list_index));
    Serial.println("BUTTON_LOAD_DATA OK!");
  }
  else if (btn == BUTTON_DELETE_DATA)
  {
    if (FILESYSTEM.exists(TXTDATA_FILENAME)) {
      FILESYSTEM.remove(TXTDATA_FILENAME);
      Serial.println(String("remove:") + TXTDATA_FILENAME );
    }
    Serial.println("BUTTON_DELETE_DATA OK!");
    init_txt_list();
    Show_hz("", true);
    Serial.println("txt_list_index=" + String(txt_list_index));
  }
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial) continue;

  //初始化SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS init failed");
    Serial.println("SPIFFS format ...");
    if (SPIFFS.format())
    {
      Serial.println("SPIFFS format ok");
      Serial.println("SPIFFS re_init");
      if (SPIFFS.begin(true))
      {
      }
      else
      {
        Serial.println("SPIFFS re_init error");
        ets_printf("reboot\n");
        esp_restart();
        return;
      }
    }
    else
    {
      Serial.println("SPIFFS format failed");
      ets_printf("reboot\n");
      esp_restart();
      return;
    }
  }
  Serial.println("SPIFFS init ok");
  epd_init();
  init_txt_list();
  int last_index = load_list();
  if (last_index > -1)
  {
    txt_list_index = last_index + 1;
    Show_hz("", true);
  }
  else
  {
    txt_list_index = 0;
    Show_hz("启动...", false);
  }
  Serial.println("txt_list_index=" + String(txt_list_index));

  BUTTON_SAVE_DATA.setTapHandler(tap);
  BUTTON_DELETE_DATA.setTapHandler(tap);
  BUTTON_LOAD_DATA.setTapHandler(tap);
  connectwifi();

  webSocket.begin();
  Serial.println("WebSocket server started");
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  BUTTON_SAVE_DATA.loop();
  BUTTON_DELETE_DATA.loop();
  BUTTON_LOAD_DATA.loop();
  connectwifi();
  webSocket.loop();
}
