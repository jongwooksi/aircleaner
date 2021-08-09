

#include <SPI.h>
#include <SD.h>
#include <FS.h>
//#include <esp_camera.h>
#include <HTTPClient.h>
#include <LovyanGFX.hpp>
#include "makerfabs_pin.h"
//#define CAMERA_MODEL_MAKERFABS
#define WIFI_MODE
#define ARRAY_LENGTH 320 * 240 * 3
#define SCRENN_ROTATION 3
#define FT6236_TOUCH //Capacitive screen driver

#ifdef NS2009_TOUCH
#include "NS2009.h"
const int i2c_touch_addr = NS2009_ADDR;
#define get_pos ns2009_pos
#endif

#ifdef FT6236_TOUCH
#include "FT6236.h"
const int i2c_touch_addr = TOUCH_I2C_ADD;
#define get_pos ft6236_pos
#endif

//SPI control
#define SPI_ON_TFT digitalWrite(ESP32_TSC_9488_LCD_CS, LOW)
#define SPI_OFF_TFT digitalWrite(ESP32_TSC_9488_LCD_CS, HIGH)
#define SPI_ON_SD digitalWrite(ESP32_TSC_9488_SD_CS, LOW)
#define SPI_OFF_SD digitalWrite(ESP32_TSC_9488_SD_CS, HIGH)

#define APIKEY "Tmb64Dp1SPy1WJ3AP2saRYuVvtK52LPamtbsJG1chVGGMGckh99%2F9dyES7U2GuPKEMk7UbK%2B9b2kXnsbLX%2FQMQ%3D%3D"
#define CITY "종로구"
#define VERSION "1.3"
const char* server = "apis.data.go.kr";
WiFiClient client;

#include <SoftwareSerial.h> 
#define DUST_TX 36
#define DUST_RX 39
#define DUST_ON_TX digitalWrite(DUST_TX, HIGH)
#define DUST_OFF_RX digitalWrite(DUST_RX, LOW)
#define DUST_ON_RX digitalWrite(DUST_TX, HIGH)
#define DUST_OFF_TX digitalWrite(DUST_RX, LOW)

#define PWM_TX 19
#define PWM_RX 18
#define PWM_ON_TX digitalWrite(PWM_TX, HIGH)
#define PWM_OFF_RX digitalWrite(PWM_RX, LOW)
#define PWM_ON_RX digitalWrite(PWM_TX, HIGH)
#define PWM_OFF_TX digitalWrite(PWM_RX, LOW)


#include <FFat.h>
#include "WiFi.h"
#include <ESP32Ping.h>
#include <lvgl.h>
//#include <TFT_eSPI.h>
#include <FT6236.h>
#define I2C_SDA 26
#define I2C_SCL 27
#define TOUCH_THRESHOLD 80
#define STAR_WIDTH 70
#define STAR_HEIGHT 80

FT6236 ts = FT6236();

int screenWidth = 480;
int screenHeight = 320;
static lv_color_t buf[LV_HOR_RES_MAX * LV_VER_RES_MAX / 10];
static lv_disp_buf_t disp_buf;
TaskHandle_t ntScanTaskHandler;
TaskHandle_t ntConnectTaskHandler;
TaskHandle_t gui_task;

//String ssid, password;
const char* remote_host = "www.google.com";
unsigned long timeout = 10000; // 10sec
int pingCount = 5; 

/* LVGL objects */

static lv_obj_t * ddlist;
static lv_obj_t * bg_top;
static lv_obj_t * bg_middle;
static lv_obj_t * bg_bottom;
static lv_obj_t * ta_password;
static lv_obj_t * kb;
static lv_obj_t * mbox_connect;
static lv_obj_t * gauge;
static lv_obj_t * label_status;





struct LGFX_Config
{
    static constexpr spi_host_device_t spi_host = ESP32_TSC_9488_LCD_SPI_HOST;
    static constexpr int dma_channel = 1;
    static constexpr int spi_sclk = ESP32_TSC_9488_LCD_SCK;
    static constexpr int spi_mosi = ESP32_TSC_9488_LCD_MOSI;
    static constexpr int spi_miso = ESP32_TSC_9488_LCD_MISO;
};

static lgfx::LGFX_SPI<LGFX_Config> tft;
static LGFX_Sprite sprite(&tft);
static lgfx::Panel_ILI9488 panel;

const uint8_t img_rgb888_320_240_head[54] = {
    0x42, 0x4d, 0x36, 0x84, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x36, 0x0, 0x0, 0x0, 0x28, 0x0,
    0x0, 0x0, 0x40, 0x1, 0x0, 0x0, 0xf0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x18, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x84, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

int img_index = 0;
int pos[2] = {0, 0};
int stream_flag = 1;
int volume = 0;
int stage = 0;
int conn = 0;
int year = -1;
int mon = -1;
int day = -1;
int wday = 0;
int startwday = -1;
int endwday = -1;
bool initWashFlag = true;

int t_mon = 0;
int t_year = 0;

int scheduledMonth = 3; // initilalized value
int scheduledDate[5] = {0};

int days[12] = { 31,29,31,30,31,30,31,31,30,31,30,31 };

const char *wname[7] = { "Sun","Mon","Tue","Wed","Thr","Fri","Sat" };
const char *bars[2] = 
{

    "======================================\n",

    "--------------------------------------\n"

};

//const char* ssid = "jongwooksi"; 
//const char* password = "0000000000";

String ssid = "";
String password = "";

int updatecount = 0;

int PM10 = -1;

SoftwareSerial dustSensor(DUST_RX, DUST_TX); // RX, TX
SoftwareSerial ABOVBoard(PWM_RX, PWM_TX); // RX, TX
                    
void setup()
{
  
    esp32_init();
    setWifi(SD);
    wifi_init();

    if (conn == 1)
    {
      dustSensor_init();
      ABOVBoard_init();
      sendDustValue();
    }
   
}

void loop()
{
  if (conn == 1)
  {
    get_pos(pos);
    pos_rotation(pos, SCRENN_ROTATION);
    touchDisplaySet();
   
    if ((conn == 1) && (stage==0))
    {
      delay(100);
      drawTime();
      /*
      updatecount ++;
  
      if (updatecount == 600)
      {
        updatecount = 0;
        checkDust(); 
        sendDustValue();
      }*/
    }
  
    if (initWashFlag)
    {
      if (year != 1970)
      {
        initScheduledWashing();
        updateScheduledWash(3);
        t_mon = mon;
      } 
    }
  
    else
    {
      if (checkDateWash() )
      {
        tft.println("alarm"); // will insert code ASAP 
        initWashFlag = true;
      }
       
    }
  }
  
}

bool checkDateWash()
{
  if ((scheduledDate[0] == year)&&(scheduledDate[1] == mon)&&(scheduledDate[1] == day)&& (conn == 1))
    return true;

  else
    return false;
}

void setBackground()
{
  tft.setRotation(1);
  SPI_OFF_TFT;
  delay(10);
    
  if (stage == 0) // main
    print_img(SD, "/back.bmp", 480, 320);

  else if (stage == 1)// setting
    print_img(SD, "/setting.bmp", 480, 320);

  else if (stage == 2) // calender
    tft.fillScreen(TFT_WHITE);
  
  else if (stage == 3) // wash
    print_img(SD, "/wash.bmp", 480, 320);
  
  else if (stage == 4) // information
    tft.fillScreen(TFT_WHITE);
    
  SPI_ON_TFT;
  delay(100);
  tft.setRotation(3);
}


void touchDisplaySet()
{
  if (stage == 0)
  {
    if (320 < pos[0] && pos[0] < 480)
    {
        if (10 < pos[1] && pos[1] < 80)
          airPollution();

        if (pos[1] > 90 && pos[1] < 150)
          checkDust();
            
        if (pos[1] < 230 && pos[1] > 160)
          show_log(3);

        if (pos[1] < 310 && pos[1] > 240)
        {
          stage = 1;
          setBackground();    
        }       
    }
  }

  else if (stage == 1) // menu
  {
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(3);
    
    tft.setCursor(130, 30);
    tft.println("Setting Menu");
    
    if (100 < pos[1] && pos[1] < 220)
    {
        if (30 < pos[0] && pos[0] < 150)
        {
          stage = 2;
          setBackground();
          drawCalendar(year, mon, 0);
        }
                 
        else if (180 < pos[0] && pos[0] < 300)
        {
          stage = 3;
          setBackground();
          draw_button();
          drawScheduledWash();
        }
             
        else if (330 < pos[0] && pos[0] < 450)
        {
          stage = 4;
          setBackground();
        }
          
    }

    else if (250 < pos[1] && pos[1] < 300)
    {
      if (40 < pos[0] && pos[0] < 80)
        {
          stage = 0;
          setBackground();
          draw_button();
          drawConnect();
        }
    }
        
          
  }

  else if (stage == 2)
  {
    
    
  
    if (0 < pos[1] && pos[1] < 35)
    {
        if (160 < pos[0] && pos[0] < 320)
        {
          stage = 1;
          setBackground();
        }
        
        else if (400 < pos[0] && pos[0] < 480)
        {
          setBackground();
          drawCalendar(t_year, t_mon+1, 1);
        }
          
        else if (0 < pos[0] && pos[0] < 80)
        {
          setBackground();
          drawCalendar(t_year, t_mon-1, -1);
        }
          
        
    }


    else if (35 < pos[1] && pos[1] < 275)
    {
        if (50 < pos[0] && pos[0] < 430)
        {
          int col = int((pos[0] - 50)/60);
          int row = int((pos[1]-100)/35);

          int clickDay = 7*row - startwday + col + 1;

          if ((clickDay >0) && (clickDay <= days[t_mon - 1]))
          {
            tft.fillRect(200, 300, 60, 20, TFT_WHITE);
            tft.setTextColor(TFT_BLACK);
            tft.setTextSize(2);
          
            tft.setCursor(200, 300);
            tft.println(t_mon);
          
            tft.setCursor(230, 300);
            tft.println(clickDay);
          

          }
        }
    }
  }

  else if (stage == 3)
  {
  
    if (100 < pos[1] && pos[1] < 220)
    {
        
        if (30 < pos[0] && pos[0] < 150)
        {
          updateScheduledWash(1);
          drawScheduledWash();
        }
         
        
        else if (180 < pos[0] && pos[0] < 300)
        {
          updateScheduledWash(3);
          drawScheduledWash();
        }
             
        else if (330 < pos[0] && pos[0] < 450)
        {
          updateScheduledWash(6);
          drawScheduledWash();
        }
          
    }
    
    else if (250 < pos[1] && pos[1] < 300)
    {
      if (40 < pos[0] && pos[0] < 80)
        {
          stage = 1;
          setBackground();
        }
    }  
  }
  
  else if (stage == 4)
  {
    if (160 < pos[0] && pos[0] < 320)
    {
        if (120 < pos[1] && pos[1] < 240)
        {
          stage = 1;
          setBackground();
        }   
    }
  }
  
}

void drawScheduledWash()
{
  tft.fillRect(200, 300, 120, 20, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(1);

  tft.setCursor(200, 300);
  tft.println(scheduledDate[0]);

  tft.setCursor(250, 300);
  tft.println(scheduledDate[1]);

  tft.setCursor(270, 300);
  tft.println(scheduledDate[2]);

  tft.setCursor(290, 300);
  tft.println(scheduledDate[3]);

  tft.setCursor(310, 300);
  tft.println(scheduledDate[4]);
  
}


void updateScheduledWash(int s_mon)
{
  if (conn != 1)
    return;
    
  scheduledMonth = s_mon;

  if ((mon + scheduledMonth) > 12)
  {
    scheduledDate[0] = year + 1;
    scheduledDate[1] = mon + scheduledMonth - 12;
  }

  else
  {
    scheduledDate[1] =  mon + scheduledMonth;
  }

  
}

void drawCalendar(int y, int m, int direct)
{
  if (conn == 0)
    return;

  if (m == 13)
  {
    m = 1;
    y += 1;
  }
    
  else if (m == 0)
  {
    m = 12;
    y -= 1;
  }

  if ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)))
    days[1] = 29;

  tft.setTextSize(2);
  tft.setCursor(200, 20);
  tft.println(y);
  tft.setCursor(260, 20);
  tft.println("/");

  tft.setCursor(280, 20);
  tft.println(m);
 
  tft.setCursor(10, 40);
  tft.println(bars[0]);

  for (int i = 0; i<7; i++)
  {
    if (i == 0)
      tft.setTextColor(TFT_RED);

    else if (i == 6)
      tft.setTextColor(TFT_BLUE);

    else
      tft.setTextColor(TFT_BLACK);
        
    tft.setCursor( 50 + 60*i, 60);
    tft.println(wname[i]);
  }
  
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(10, 80);
  tft.println(bars[1]);

  int start = wday - (day - 1);
  int space = 0;

  if (direct == 0)
    while (start < 0)
      start = start+ 7;
  
  if ((direct == 1)&& (endwday != -1))
      start = endwday;

  else if ((direct == -1)&& (startwday != -1))
  {
    start = startwday - days[m-1] ;
      while (start < 0)
        start = start+ 7;
  }
      
      
  startwday = start;
   
  for (int i = 0; i<days[m-1]; i++)
  {
      if (i < 9)
        tft.setCursor( 60 + 60*(start), 100+35*space);

      else
        tft.setCursor( 50 + 60*(start), 100+35*space);


      if (start == 0)
        tft.setTextColor(TFT_RED);

      else if (start == 6)
        tft.setTextColor(TFT_BLUE);

      else
        tft.setTextColor(TFT_BLACK);
        
      tft.println(i+1);
  
      start++;
      
      if (start == 7)
      {
          start = 0;
          space++;      
      }

      endwday = start;
      
  }
  
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(10, 300);
  tft.println(bars[0]);

  t_mon = m;
  t_year = y;
}



void drawConnect()
{
  tft.setCursor(20, 20);
  tft.println("WIFI");
   
  tft.setCursor(20, 40);

  if (conn == 0)
    tft.println("Not connected");
  else
    tft.println("connected");
    
}

void setTime()
{
  configTime(9 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("\nWaiting for time");
  
  while (!time(nullptr)){
    Serial.print(".");
    delay(100);
  }
  
  Serial.println("");
  
}


void drawTime()
{
  String date[6];

  tft.setTextColor(TFT_BLACK);
  tft.fillRect(20, 300, 200, 20, TFT_WHITE);
  tft.setTextSize(1);

  
  time_t now = time(nullptr);
  struct tm * timeinfo;
  timeinfo = localtime(&now); 

  year = timeinfo->tm_year+1900;
  mon = timeinfo->tm_mon+1;
  day = timeinfo->tm_mday;  
  wday = timeinfo->tm_wday;

  if (year == 1970)
  {
    tft.drawString("Time connecting...", 20, 300);
    return ;
  }
  
  date[0] = String(timeinfo->tm_year+1900, DEC);
  date[1] = String(timeinfo->tm_mon+1, DEC);
  date[2] = String(timeinfo->tm_mday, DEC);
  date[3] = String(timeinfo->tm_hour, DEC); 
  date[4] = String(timeinfo->tm_min, DEC); 
  date[5] = String(timeinfo->tm_sec, DEC); 
  
   
  if(timeinfo->tm_sec<10)      date[5]="0"+date[5];
  if(timeinfo->tm_min<10)      date[4]="0"+date[4];
  if(timeinfo->tm_hour<10)     date[3]="0"+date[3];
  if(timeinfo->tm_mday<10)     date[2]="0"+date[2];
  if(timeinfo->tm_mon<10)      date[1]="0"+date[1];
  

  tft.drawString(date[0]+"-", 20, 300);
  tft.drawString(date[1]+"-", 50, 300);
  tft.drawString(date[2]+" ", 70, 300);
  tft.drawString(date[3]+":", 90, 300);
  tft.drawString(date[4]+":", 110, 300);
  tft.drawString(date[5], 130, 300);

  //Serial.println(ctime(&now));
  
}


void initScheduledWashing()
{
  initWashFlag = false;
  
  scheduledDate[0] = year;
  scheduledDate[1] = mon;
  scheduledDate[2] = day;
  scheduledDate[3] = 0;
  scheduledDate[4] = 0;
}

void dustSensor_init()
{
  dustSensor.begin(9600);
}

void checkDust()
{
  while (1)
  {
      DUST_ON_TX;
      DUST_ON_RX;
      
      unsigned char  pms[32];
      tft.fillRect(160, 160, 160, 160, TFT_WHITE);
      tft.setCursor(180, 180);
      tft.println(dustSensor.available());
      tft.setTextColor(TFT_RED);
      tft.setTextSize(2);
      delay(100);
      if(dustSensor.available()>=32){
        int dustlength = dustSensor.available();
        int check = 0;

        for (int j = 0 ; j < 32 ; j++)
        {
          pms[j]=dustSensor.read();
     
        }

        if( ((pms[0] != 0x42)) || ( (pms[1] != 0x4d)) ) 
        {
          Serial.printf("Invalid Header \n");
          continue;
        }

        /*
        if (pms[29] != 0x00)
        {
          Serial.printf("Error Code 0 \n");
          continue;
        }*/
        
        int PM1_0=(pms[10]<<8)|pms[11];
        int PM2_5=(pms[12]<<8)|pms[13];
        PM10 =(pms[14]<<8)|pms[15];

        
        
        Serial.printf("Home Atmospheric PM1.0 : %d,  PM2.5 : %d,  PM10 : %d  \r\n", PM1_0, PM2_5,PM10);

        tft.fillRect(160, 160, 160, 160, TFT_WHITE);
        tft.setCursor(180, 180);
        tft.println("PM1.0");
  
        tft.setCursor(250, 180);
        tft.println(PM1_0);
  
        tft.setCursor(180, 210);
        tft.println("PM2.5");
  
        tft.setCursor(250, 210);
        tft.println(PM2_5);

        tft.setCursor(180, 240);
        tft.println("PM10");
  
        tft.setCursor(250, 240);
        tft.println(PM10);


        DUST_OFF_TX;
        DUST_OFF_RX;
        break;
      }    
    }
}

void ABOVBoard_init()
{
  ABOVBoard.begin(9600);
}

void sendDustValue()
{
  if ( (PM10 >= 0 ) && (PM10 < 30 )) // good
    ABOVBoard.write((byte)0x1F);
  else if ( (PM10 >= 30 ) && (PM10 < 80 )) // soso
    ABOVBoard.write((byte)0x2F);
  else if ( (PM10 >= 80 ) && (PM10 < 150 )) // bad
    ABOVBoard.write((byte)0x3F);
  else if ( PM10 >= 150 ) // very bad
    ABOVBoard.write((byte)0x4F);
  else if ( PM10 == -1 ) // power on
    ABOVBoard.write((byte)0x0F);
  
}
void checkAbovPWM()
{
  
  if (volume == 0)
    ABOVBoard.write((byte)0xF0);

  else if (volume == 1)
    ABOVBoard.write((byte)0xF1);

  else if (volume == 2)
    ABOVBoard.write((byte)0xF2);

  else if (volume == 3)
    ABOVBoard.write((byte)0xF3);
  
  else if (volume == 4)
    ABOVBoard.write((byte)0xF4);

  else if (volume == 5)
    ABOVBoard.write((byte)0xF5);





  delay(100);

   
}


void airPollution()
{ 
#ifdef WIFI_MODE

  HTTPClient myhttp; 
  
  String url = "http://apis.data.go.kr/B552584/ArpltnInforInqireSvc/getMsrstnAcctoRltmMesureDnsty?serviceKey=Tmb64Dp1SPy1WJ3AP2saRYuVvtK52LPamtbsJG1chVGGMGckh99%2F9dyES7U2GuPKEMk7UbK%2B9b2kXnsbLX%2FQMQ%3D%3D&returnType=xml&numOfRows=10&pageNo=1&stationName=%EC%A2%85%EB%A1%9C%EA%B5%AC&dataTerm=DAILY&ver=1.3";
  
  if(myhttp.begin(client, url)) {

    int httpCode = myhttp.GET();

    if(httpCode > 0) {
      tft.setTextColor(TFT_RED);
      tft.setTextSize(2);


      String payload = myhttp.getString();
      int start_point = payload.indexOf("<pm10Value>");
      int end_point = payload.indexOf("</pm10Value>");    
      int start_point_2 = payload.indexOf("<pm25Value>");
      int end_point_2 = payload.indexOf("</pm25Value>");

      String pm10 = payload.substring(start_point + 11, end_point);
      String pm25 = payload.substring(start_point_2 + 11, end_point_2);
      
      tft.fillRect(160, 160, 160, 160, TFT_WHITE);
      tft.setCursor(180, 180);
      tft.println("PM10");

      tft.setCursor(250, 180);
      tft.println(pm10);

      tft.setCursor(180, 210);
      tft.println("PM25");

      tft.setCursor(250, 210);
      tft.println(pm25);
      
    }

    else {
      tft.setCursor(180, 180);
      tft.println("failed");

    }
  }
  #endif 
  
}

void wifi_init()
{
  #ifdef WIFI_MODE

    WiFi.mode(WIFI_OFF);

    for (uint8_t t = 4; t > 0; t--)
    {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(200);
    }

    Serial.println("Connecting to WIFI");

    // wait for WiFi connection

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(),password.c_str());

    //while (WiFi.status() != WL_CONNECTED)
    for (int i = 0; i < 300; i++)
    {
        Serial.print(".");
        delay(50);

        if (WiFi.status() == WL_CONNECTED)
        {
          Serial.println("\n   WIFI CONNECTED");
          break;
        }
    }


    if (client.connect(server, 80)) 
    {
      Serial.println("connected to server");
      tft.setCursor(20, 40);

      tft.setRotation(1);
      SPI_OFF_TFT;
      delay(10);
      print_img(SD, "/back.bmp", 480, 320);
      SPI_ON_TFT;
      delay(1000);
      tft.setRotation(3);
      draw_button();
    
      conn = 1;
      drawConnect();
      setTime();
    }
  
    //if (!client.connected()) 
    else
    {
      Serial.println();
      Serial.println("disconnecting from server.");
  
      client.stop();
    
      conn = 0;
      wifi_setup(); 
      //drawConnect();
      
      
    }

  #endif

  
}

void esp32_init()

{

    int SD_init_flag = 0;

    Serial.begin(115200);

    Serial.println("ILI9488 Test!");

    //I2C init

    Wire.begin(ESP32_TSC_9488_I2C_SDA, ESP32_TSC_9488_I2C_SCL);

    byte error, address;

    Wire.beginTransmission(i2c_touch_addr);

    error = Wire.endTransmission();

    if (error == 0)

    {

        Serial.print("I2C device found at address 0x");

        Serial.print(i2c_touch_addr, HEX);

        Serial.println("  !");

    }

    else if (error == 4)

    {

        Serial.print("Unknown error at address 0x");

        Serial.println(i2c_touch_addr, HEX);

    }

    //SPI init

    pinMode(ESP32_TSC_9488_SD_CS, OUTPUT);

    pinMode(ESP32_TSC_9488_LCD_CS, OUTPUT);

    SPI_OFF_SD;

    SPI_OFF_TFT;

    SPI.begin(ESP32_TSC_9488_HSPI_SCK, ESP32_TSC_9488_HSPI_MISO, ESP32_TSC_9488_HSPI_MOSI);

    //SD(SPI) init

    SPI_ON_SD;

    if (!SD.begin(ESP32_TSC_9488_SD_CS, SPI, 40000000))

    {

        Serial.println("Card Mount Failed");

        SD_init_flag = 1;

    }

    else

    {

        Serial.println("Card Mount Successed");

    }

    SPI_OFF_SD;

    Serial.println("SD init over.");

    //TFT(SPI) init

    SPI_ON_TFT;

    set_tft();

    tft.begin();

    tft.setRotation(SCRENN_ROTATION);

    tft.fillScreen(TFT_BLACK);

    //if SD init failed

    if (SD_init_flag == 1)

    {

        tft.setTextColor(TFT_RED);
        tft.setTextSize(3);
        tft.setCursor(10, 10);
        tft.println("  SD card initialization failed, please re-insert.");
        tft.setCursor(10, 60);
        tft.println("  Or touch the screen using only the camera (showing only the output stream)");
        tft.fillRect(100, 160, 280, 160, TFT_BLUE);
        tft.setCursor(130, 190);
        tft.println("    TOUCH");
        tft.setCursor(140, 220);
        tft.println("     TO");
        tft.setCursor(120, 250);
        tft.println("   CONTINUE");

        int pos[2] = {0, 0};

        while (1)

        {
            get_pos(pos);
            if (pos[0] > 100 && pos[0] < 380 && pos[1] > 160 && pos[1] < 320)
                break;
            delay(100);
        }
  
    }

    else
    {
       tft.setTextColor(TFT_WHITE);
       tft.setTextSize(3);
       tft.setCursor(110, 10);
       tft.println("Booting Mode");

       tft.setCursor(110, 100);
       tft.println("Wifi Connecting...");

       tft.setTextColor(TFT_BLACK);
       tft.setTextSize(1);
    }
    
    SPI_OFF_TFT;

    Serial.println("TFT init over.");



}

int print_img(fs::FS &fs, String filename, int x, int y)

{

    SPI_ON_SD;

    File f = fs.open(filename, "r");
    if (!f)
    {
        Serial.println("Failed to open file for reading");
        f.close();
        return 0;
    }

    f.seek(54);

    int X = x;
    int Y = y;
    uint8_t RGB[3 * X];

    for (int row = 0; row < Y; row++)
    {
        f.seek(54 + 3 * X * row);
        f.read(RGB, 3 * X);
        
        SPI_OFF_SD;
        tft.pushImage(0, row, X, 1, (lgfx::rgb888_t *)RGB);
        SPI_ON_SD;
    }

    f.close();
    
    SPI_OFF_SD;

    return 0;
}


int setWifi(fs::FS &fs)

{
    String wifiIdFile = "/wifiId.txt";
    String wifiPwFile = "/wifiPw.txt";
    
    SPI_ON_SD;

    File f = fs.open(wifiIdFile, "r");
    if (!f)
    {
        Serial.println("Failed to open id file for reading");
        f.close();
        return 0;
    }

   
    while(f.available()){
      char temp = f.read();
      ssid += temp; 
    }

    f.close();


    File f2 = fs.open(wifiPwFile, "r");
    if (!f2)
    {
        Serial.println("Failed to open pw file for reading");
        f2.close();
        return 0;
    }

   
    while(f2.available()){
      char temp = f2.read();
      password += temp; 
    }

    f2.close();


    Serial.println("id");
    Serial.println(ssid.c_str());
    Serial.println("pw");
    Serial.println(password.c_str());
    
    SPI_OFF_SD;

    return 0;
}



void draw_button()
{
    SPI_ON_TFT;
    
    
    if (stage == 0)
    {
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(1.5);
    
      tft.setCursor(340, 50);
      tft.println("Air pollution");
  
      tft.setCursor(340, 120);
      tft.println(" Indoor Dust");
  
      tft.setCursor(340, 190);
      tft.println("  Pan Motor");
  
      tft.setCursor(340, 260);
      tft.println("   Setting");
    }
    
    else if (stage == 3)
    {
      tft.setTextColor(TFT_BLACK);
      tft.setTextSize(3);
    
      tft.setCursor(150, 30);
      tft.println("Scheduled Washing");
      
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(1.5);
    
      tft.setCursor(55, 160);
      tft.println("+ 1 month");
  
      tft.setCursor(200, 160);
      tft.println("+ 3 month");
  
      tft.setCursor(345, 160);
      tft.println("+ 6 month");
      
    }
    SPI_OFF_TFT;

}

 
void show_log(int cmd_type)

{
    SPI_ON_TFT;
    tft.fillRect(160, 160, 160, 160, TFT_WHITE);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(180, 180);

    switch (cmd_type)
    {
      case 3:
          tft.fillRect(160, 160, 160, 160, TFT_WHITE);

          if (volume == 5)
            volume = -1;

          volume++;
          
          tft.println("Air Volume: ");
          tft.setCursor(180, 210);      
          tft.println(volume);
          
          checkAbovPWM();
          
          break;

    default:
        break;

    }

    SPI_OFF_TFT;

}



#ifdef WIFI_MODE

void TestPostFileStream(String file_name)

{

    SPI_ON_SD;

    HTTPClient http;

    String filename = String(img_index - 1) + "write.bmp";

    String pathname = file_name;

    String archDomainAddress = "http://192.168.1.128:5002/json";

    http.begin(archDomainAddress);

    http.addHeader("Content-Type", "text/plain");

    File payloadFile = SD.open(pathname, FILE_READ);

    if (payloadFile)

    {

        Serial.println("File exists, starting POST");

        http.addHeader("File-Name", filename);

        int httpCode = http.sendRequest("POST", &payloadFile, payloadFile.size());

        if (httpCode > 0)

        {

            // HTTP header has been send and Server response header has been handled

            Serial.printf("[HTTP] POST... code: %d\n", httpCode);

            // file found at server

            if (httpCode >= 200 && httpCode < 300)

            {

                String payload = http.getString();

                Serial.println("SUCCESS! DATA RECEIVED: ");

                Serial.println(payload);

            }

        }

        else

        {

            Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());

        }

    }

    else

    {

        Serial.println("File does not exist!");

    }

    payloadFile.close();

    http.end();

    SPI_OFF_SD;

    Serial.println("POST over");

}

#endif

void set_tft()

{

    panel.freq_write = 40000000;

    panel.freq_fill = 40000000;

    panel.freq_read = 16000000;

    panel.spi_cs = ESP32_TSC_9488_LCD_CS;

    panel.spi_dc = ESP32_TSC_9488_LCD_DC;

    panel.gpio_rst = ESP32_TSC_9488_LCD_RST;

    panel.gpio_bl = ESP32_TSC_9488_LCD_BL;

    tft.setPanel(&panel);

}

//transform touch screen pos

void pos_rotation(int pos[2], int rotation)

{

    if (pos[0] == -1)

        return;

    if (rotation == 0)

    {

        return;

    }

    if (rotation == 3)

    {

        int tempx = 480 - pos[1];

        int tempy = pos[0];

        pos[0] = tempx;

        pos[1] = tempy;

    }

}






void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)

{

  uint16_t c;

  tft.startWrite(); /* Start new tft transaction */

  tft.setAddrWindow(area->x1, area->y1, (area->x2 - area->x1 + 1), (area->y2 - area->y1 + 1)); /* set the working window */

  for (int y = area->y1; y <= area->y2; y++) {

    for (int x = area->x1; x <= area->x2; x++) {

      c = color_p->full;

      tft.writeColor(c, 1);

      color_p++;

    }

  }

  tft.endWrite(); /* terminate tft transaction */

  lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */

}

bool my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data)

{

    uint16_t touchX, touchY;

    bool touched = false;


    bool needRefresh = true;

    

    for(int i=0; i< ts.touched(); i++){

      TS_Point p = ts.getPoint(i);

      //uint16_t x = tft.width() - p.x;

      //uint16_t y = tft.height() - p.y;

      uint16_t y = p.x;

      uint16_t x = tft.width()- p.y;

       

      //int16_t temp;

      //temp = x;

      //x = y;

      //y = temp;

     // drawStar(i, x, y, random(0x10000));

      needRefresh = false;

      touchX = x;

      touchY = y;

      touched = true;

      break;

    }

    

    if(!touched)

    {

      return false;

    }

    if(touchX>screenWidth || touchY > screenHeight)

    {

      Serial.println("Y or y outside of expected parameters..");

      Serial.print("y:");

      Serial.print(touchX);

      Serial.print(" x:");

      Serial.print(touchY);

    }

    else

    {

      data->state = touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL; 

      data->point.x = touchX;

      data->point.y = touchY;

    }

    return false; /*Return `false` because we are not buffering and no more data to read*/

}

void wifi_setup() {

  
  //Serial.begin(115200);


  
  if (!ts.begin(TOUCH_THRESHOLD, I2C_SDA, I2C_SCL)){
      Serial.println("Unable to start the capacitive touchscreen.");
  }  

  

  
  xTaskCreate(guiTask, "gui", 4096*2, NULL,2,&gui_task);

  networkScanner();                 

}

 

void networkScanner(){

  vTaskDelay(500);
  xTaskCreate(scanWIFITask, "ScanWIFITask",4096, NULL,1,&ntScanTaskHandler);

}


void guiTask(void *pvParameters) {

    lv_init();
    tft.begin();
    tft.setRotation(3);

    uint16_t calData[5] = { 295, 3493, 320, 3602, 2 };

    //tft.setTouch(calData);

 
    lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX / 10);

    lv_disp_drv_t disp_drv;

    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = screenWidth;

    disp_drv.ver_res = screenHeight;

    disp_drv.flush_cb = my_disp_flush;

    disp_drv.buffer = &disp_buf;

    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_t indev_drv;

    lv_indev_drv_init(&indev_drv);             /*Descriptor of a input device driver*/

    indev_drv.type = LV_INDEV_TYPE_POINTER;    /*Touch pad is a pointer-like device*/

    indev_drv.read_cb = my_touchpad_read;      /*Set your driver function*/

    lv_indev_drv_register(&indev_drv);         /*Finally register the driver*/

  

    lv_main();

  

    while (1) {

         lv_task_handler();

    }

}
static void lv_main(){
    
    //LV_THEME_MATERIAL_FLAG_LIGHT
    //LV_THEME_MATERIAL_FLAG_DARK
    
    lv_theme_t * th = lv_theme_material_init(LV_THEME_DEFAULT_COLOR_PRIMARY, LV_THEME_DEFAULT_COLOR_SECONDARY, LV_THEME_MATERIAL_FLAG_LIGHT, LV_THEME_DEFAULT_FONT_SMALL , LV_THEME_DEFAULT_FONT_NORMAL, LV_THEME_DEFAULT_FONT_SUBTITLE, LV_THEME_DEFAULT_FONT_TITLE);     
    lv_theme_set_act(th);

    lv_obj_t * scr = lv_obj_create(NULL, NULL);
    lv_scr_load(scr);

    bg_top = lv_obj_create(scr, NULL);
    lv_obj_clean_style_list(bg_top, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bg_top, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bg_top, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,LV_COLOR_NAVY);
    lv_obj_set_size(bg_top, LV_HOR_RES, 50);
    
    bg_middle = lv_obj_create(scr, NULL);
    lv_obj_clean_style_list(bg_middle, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bg_middle, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bg_middle, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,LV_COLOR_WHITE);
    lv_obj_set_pos(bg_middle, 0, 50);
    lv_obj_set_size(bg_middle, LV_HOR_RES, 400);

    bg_bottom = lv_obj_create(scr, NULL);
    lv_obj_clean_style_list(bg_bottom, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bg_bottom, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bg_bottom, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,LV_COLOR_ORANGE);
    lv_obj_set_pos(bg_bottom, 0, 290);
    lv_obj_set_size(bg_bottom, LV_HOR_RES, 30);

    label_status = lv_label_create(bg_bottom, NULL);
    lv_label_set_long_mode(label_status, LV_LABEL_LONG_SROLL_CIRC);
    lv_obj_set_width(label_status, LV_HOR_RES - 20);
    lv_label_set_text(label_status, "");
    lv_obj_align(label_status, NULL, LV_ALIGN_CENTER, 0, 0);
    
    makeDropDownList();
    makeGaugeIndicator();
    makeKeyboard();
    makePWMsgBox();
 }


static void updateBottomStatus(lv_color_t color, String text){

  lv_obj_set_style_local_bg_color(bg_bottom, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,color);

  lv_label_set_text(label_status, text.c_str());

}

static void makeDropDownList(void){

    ddlist = lv_dropdown_create(bg_top, NULL);

    lv_dropdown_set_show_selected(ddlist, false);

    lv_dropdown_set_text(ddlist, "WIFI");

    lv_dropdown_set_options(ddlist, "...Searching...");

    lv_obj_align(ddlist, NULL, LV_ALIGN_IN_TOP_LEFT, 4, 8);

    lv_obj_set_event_cb(ddlist, dd_event_handler);

}

static void dd_event_handler(lv_obj_t * obj, lv_event_t event){

  

  if(event == LV_EVENT_VALUE_CHANGED) {

        char buf[32];

        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));

        ssid = String(buf);

        

        for (int i = 0; i < ssid.length()-1; i++) {

          if (ssid.substring(i, i+2) == " (") {

              ssid = ssid.substring(0, i);

            break;

          }

        }

        

        popupPWMsgBox();

    }

}

static void makeGaugeIndicator(void){

    gauge = lv_gauge_create(bg_middle, NULL);

    lv_obj_set_size(gauge, 180, 180);

    lv_obj_align(gauge, NULL, LV_ALIGN_CENTER, 0, -70);

    lv_gauge_set_value(gauge, 0, 100);

    lv_obj_t * label = lv_label_create(gauge, NULL);

    lv_obj_align(label, gauge, LV_ALIGN_CENTER, 0, 70);

    lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_theme_get_font_title());

    setGaugeValue(100, 0);

}

static void setGaugeValue(float prev, float curt){

    lv_anim_t a;

    lv_anim_init(&a);

    lv_anim_set_var(&a, gauge);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)gauge_anim);

    lv_anim_set_values(&a, prev, curt);

    lv_anim_set_time(&a, 1000);

    lv_anim_set_repeat_count(&a, 0);

    lv_anim_start(&a);

}

static void gauge_anim(lv_obj_t * gauge, lv_anim_value_t value){

    lv_gauge_set_value(gauge, 0, value);

    static char buf[64];

    lv_snprintf(buf, sizeof(buf), "%d ms", value);

    lv_obj_t * label = lv_obj_get_child(gauge, NULL);

    lv_label_set_text(label, buf);

    lv_obj_align(label, gauge, LV_ALIGN_CENTER, 0, 70);

}

static void makeKeyboard(){

  kb = lv_keyboard_create(lv_scr_act(), NULL);

  lv_obj_set_size(kb,  LV_HOR_RES, LV_VER_RES / 2);

  lv_keyboard_set_cursor_manage(kb, true);

  

  lv_keyboard_set_textarea(kb, ta_password);

  lv_obj_set_event_cb(kb, keyboard_event_cb);

  lv_obj_move_background(kb);

}

static void keyboard_event_cb(lv_obj_t * kb, lv_event_t event){

  lv_keyboard_def_event_cb(kb, event);

  if(event == LV_EVENT_APPLY){

    lv_obj_move_background(kb);

    

  }else if(event == LV_EVENT_CANCEL){

    lv_obj_move_background(kb);

  }

}

static void makePWMsgBox(){

  mbox_connect = lv_msgbox_create(lv_scr_act(), NULL);

  static const char * btns[] ={"Connect", "Cancel", ""};

  

  ta_password = lv_textarea_create(mbox_connect, NULL);

  lv_obj_set_size(ta_password, 200, 40);

  lv_textarea_set_text(ta_password, "");

 

  lv_msgbox_add_btns(mbox_connect, btns);

  lv_obj_set_width(mbox_connect, 200);

  lv_obj_set_event_cb(mbox_connect, mbox_event_handler);

  lv_obj_align(mbox_connect, NULL, LV_ALIGN_CENTER, 0, -90);

  lv_obj_move_background(mbox_connect);

}

static void mbox_event_handler(lv_obj_t * obj, lv_event_t event){

    if(event == LV_EVENT_VALUE_CHANGED) {

      lv_obj_move_background(kb);

      lv_obj_move_background(mbox_connect);

      

          if(strcmp(lv_msgbox_get_active_btn_text(obj), "Connect")==0){

            password = lv_textarea_get_text(ta_password);

            password.trim();

            connectWIFI();

          }

    

    }

}

static void popupPWMsgBox(){

  if(ssid == NULL || ssid.length() == 0){

    return;

  }

    lv_textarea_set_text(ta_password, ""); 

    lv_msgbox_set_text(mbox_connect, ssid.c_str());

    lv_obj_move_foreground(mbox_connect);

    

    lv_obj_move_foreground(kb);

    lv_keyboard_set_textarea(kb, ta_password);

}

/*

 * NETWORK TASKS

 */

void scanWIFITask(void *pvParameters) {  

  vTaskDelay(1000); 

  while (1) {

    updateBottomStatus(LV_COLOR_ORANGE, "::: Searching Available WIFI :::");        

    int n = WiFi.scanNetworks();

    if (n <= 0) {

      updateBottomStatus(LV_COLOR_RED, "Sorry no networks found!");        

    }else{

      lv_dropdown_clear_options(ddlist);  

      vTaskDelay(10);

      for (int i = 0; i < n; ++i) {

                           

        String item = WiFi.SSID(i) + " (" + WiFi.RSSI(i) +") " + ((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");

        lv_dropdown_add_option(ddlist,item.c_str(),LV_DROPDOWN_POS_LAST);

        vTaskDelay(10);

      }

     updateBottomStatus(LV_COLOR_GREEN, String(n) + " networks found!");                     

    }

    vTaskDelay(30000); 

   } 

}

 

void connectWIFI(){

  if(ssid == NULL || ssid.length() <1 || password == NULL || password.length() <1){
    return;
  }

  vTaskDelete(ntScanTaskHandler);
  vTaskDelay(500);
  xTaskCreate(beginWIFITask,"BeginWIFITask", 4096, NULL, 0,&ntConnectTaskHandler);                


}


void checkWIFI()
{
  if(WiFi.status() == WL_CONNECTED) 
  {
    String wifiIdFile = "/wifiId.txt";
    String wifiPwFile = "/wifiPw.txt";
    
    SPI_ON_SD;

    // 파일에다가 쓰고 Rebooting
    // ssid랑 password를 위에 파일에 기입하면 됨
    // read한 부분은 setwifi() 함수를 참고
    
    SPI_OFF_SD;

  }
}



void beginWIFITask(void *pvParameters) {

  updateBottomStatus(LV_COLOR_TEAL,"Connecting WIFI: " + ssid);

  unsigned long startingTime = millis();

  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED && (millis() - startingTime) < timeout)

  {

    vTaskDelay(250);

  }

   if(WiFi.status() != WL_CONNECTED) {

    updateBottomStatus(LV_COLOR_RED, "Please check your wifi password and try again.");

    vTaskDelay(2500);

    networkScanner();

    vTaskDelete(NULL);

  }

 
  updateBottomStatus(LV_COLOR_GREEN, "WIFI is Connected! Local IP: " +  WiFi.localIP().toString());

  checkWIFI();
  


}
