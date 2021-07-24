

#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <esp_camera.h>
#include <HTTPClient.h>
#include <LovyanGFX.hpp>
#include "makerfabs_pin.h"
#define CAMERA_MODEL_MAKERFABS
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

const char* ssid = "jongwooksi"; 
const char* password = "0000000000";

SoftwareSerial dustSensor(DUST_RX, DUST_TX); // RX, TX
              
void setup()
{
    esp32_init();
    wifi_init();
    dustSensor_init();
  
}

void loop()
{
  get_pos(pos);
  pos_rotation(pos, SCRENN_ROTATION);
  touchDisplaySet();
 
  if ((conn == 1) && (stage==0))
  {
    delay(100);
    drawTime(); 
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
        int PM10 =(pms[14]<<8)|pms[15];

        
        
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


void checkAbovPWM()
{
  
  if (volume == 0)
    Wire.write(0x00);

  else if (volume == 1)
    Wire.write(0x01);

  else if (volume == 2)
    Wire.write(0x02);

  else if (volume == 3)
    Wire.write(0x04);
  
  else if (volume == 4)
    Wire.write(0x08);

  else if (volume == 5)
    Wire.write(0x10);


  Serial.println(Wire.endTransmission());


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
    WiFi.begin(ssid,password);

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
      drawConnect();
  
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
        tft.setRotation(1);
        SPI_OFF_TFT;
        delay(10);
        print_img(SD, "/back.bmp", 480, 320);
        SPI_ON_TFT;
        delay(1000);
        tft.setRotation(3);
    }

    draw_button();
    
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
