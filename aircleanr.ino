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

    if (320 < pos[0] && pos[0] < 480)
    {
        if (10 < pos[1] && pos[1] < 80)
          airPollution();

        if (pos[1] > 90 && pos[1] < 150)
          show_log(2);
            
        if (pos[1] < 230 && pos[1] > 160)
          show_log(3);

        if (pos[1] < 310 && pos[1] > 240)
        {
          checkDust();
        }
        
    }
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

         if (pms[29] != 0x00)
         {
            Serial.printf("Error Code 0 \n");
            continue;
         }
        
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

        tft.setCursor(250, 270);
        tft.println(pms[0]);
        tft.setCursor(250, 300);
        tft.println(pms[1]);


        DUST_OFF_TX;
        DUST_OFF_RX;
        break;
      }    
    }
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
    WiFi.begin("jongwooksi", "0000000000");

    //while (WiFi.status() != WL_CONNECTED)
    for (int i = 0; i < 100; i++)
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
      tft.println("connected");
  
    }
  
    //if (!client.connected()) 
    else
    {
      Serial.println();
      Serial.println("disconnecting from server.");
  
      client.stop();
  
      tft.setCursor(20, 40);
      tft.println("Not connected");
  
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

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1.5);

    tft.setCursor(340, 50);
    tft.println("Air pollution");

    tft.setCursor(340, 120);
    tft.println("Motor On");

    tft.setCursor(340, 190);
    tft.println("Motor Off");

    tft.setCursor(340, 260);
    tft.println("Indoor Dust ");
    
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
      case 2:
          tft.fillRect(160, 160, 160, 160, TFT_WHITE);
          tft.println("Motor ON");
  
          break;

      case 3:
          tft.fillRect(160, 160, 160, 160, TFT_WHITE);
          tft.println("Motor OFF");
  
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
