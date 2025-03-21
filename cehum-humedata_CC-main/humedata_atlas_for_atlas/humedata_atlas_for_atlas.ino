/*
   TITLE: CEHUM - HUMEDAT@ ATLAS
   AUTHOR: CHRISTIAN SANTIBÁÑEZ SOTO (V. 1.0)
   COMPANY: LEUFÜLAB
   DATE: 12/01/2022
   VERSION: 1.2 modified by C. Correa
   Centro de Humedales Río Cruces (CEHUM)
   Universidad Austral de Chile
   Contact: cristiancorrea@gmail.com
   Version: 1.2.1 modified by Ian Zamora 
   Used in atlas model due the different in the configuration and PCB than the toto model
*/

// Librerías  
#include <Wire.h>
#include <Arduino_MKRENV.h>
#include <SPI.h>
#include <RTClib.h>
#include <SD.h>
#include <MKRWAN.h>
#include <Adafruit_BMP280.h>
#include "ArduinoLowPower.h"
#include "TinyGPS++.h"
#include "io_definitions.h"


/*
 *           UNIDADES DE MEDIDA
 *           
 *[0]  --> DISSOLVED OXYGEN          [mg/L]
 *[1]  --> PH                        [-]
 *[2]  --> ELECTRICAL CONDUCTIVITY   [uS/cm]
 *[3]  --> TOTAL DISSOLVED SOLIDS    [ppm]
 *[4]  --> SALINITY                  [ppt]
 *[5]  --> RELATIVE DENSITY          [-]
 *[6]  --> WATER TEMPERATURE         [°C]
 *[7]  --> INTERNAL PRESSURE         [KPa]
 *[8]  --> ATMOSPHERIC PRESSURE      [KPa]
 *[9]  --> ATMOSPHERIC TEMPERATURE   [°C]
 *[10] --> GPS LATITUDE              [°]
 *[11] --> GPS LONGITUDE             [°]
 *[12] --> INTERNAL TEMPERATURE      [°C]
 *[13] --> INTERNAL HUMIDITY         [%]
 *[14] --> BATTERY LEVEL             [V]
 *[15] --> ORP                       [mV]
 */

/* RANGO SENSORES (OBTENIDOS DESDE HOJAS DE DATOS DE ATLAS SCIENTIFIC)
 *         
 * DISSOLVED OXYGEN        --> 0 - 100 [mg/L]     [1 byte]  [O]
 * pH                      --> 0 - 14             [1 byte]  [O]
 * ELECTRICAL CONDUCTIVITY --> 0,07 - 500.000     [4 bytes] [O]
 * TOTAL DISSOLVED SOLIDS  --> 5 - 500.000        [4 bytes] [O]
 * SALINITY                --> 0,00 - 42,00       [1 byte]  [O]
 * RELATIVE DENSITY        --> 1,00 - 1,300       [1 byte]  [O]
 * WATER TEMPERATURE       --> 0 - 60             [1 byte]  [O]
 * INTERNAL PRESSURE       --> 80 - 200           [1 byte]  [O]
 * ATMOSPHERIC PRESSURE    --> 80 - 120           [1 byte]  [O]
 * ATMOSPHERIC TEMPERATURE --> -20 - 60           [1 byte]  [O]     
 * GPS LATITUDE            -->                    [4 bytes] [O]
 * GPS LONGITUDE           -->                    [4 bytes] [O]
 * INTERNAL TEMPERATURE    --> -20 - 60           [1 byte]  [O]
 * INTERNAL HUMIDITY       --> 0 - 120            [1 byte]  [O]
 * BATTERY LEVEL           --> 8 - 13             [1 byte]  [O]
 * ORP                     --> -2000 - 2000       [4 bytes] [O]
 * 
 * TOTAL:                                         [31 bytes]
 */

// Función para transformar un flotante en sus 4 bytes componentes
// Forma de utilizar float2Bytes(flotante a transformar en bytes, arreglo en el que guardar los bytes)
void float2Bytes(float val,byte* bytes_array){
  // Create union of shared memory space
  union {
    float float_variable;
    byte temp_array[4];
  } u;
  // Overite bytes of union with float variable
  u.float_variable = val;
  // Assign bytes to input array
  memcpy(bytes_array, u.temp_array, 4);
}

// Tiempo de espera entre muestreado de datos (en minutos)
const int sleep_time = 2;

// Objeto GPS
TinyGPSPlus gps;

void setup() {
  // Se inicia la comunicación UART para debugging
  Serial.begin(115200);
  // Se inicia la comunicación UART para comunicarse con el módulo GPS 
  Serial1.begin(9600);
  // Se declaran las salidas de apagado de sensores Atlas Scientific
  pinMode(rtd_off_pin, OUTPUT);
  pinMode(ph_off_pin, OUTPUT);
  pinMode(orp_off_pin, OUTPUT);
  pinMode(ec_off_pin, OUTPUT);
  pinMode(do_off_pin, OUTPUT);
  pinMode(gps_switch_pin, OUTPUT);
  pinMode(reset_on_pin, OUTPUT);
  // Se inician los sensores Atlas Scientific en modo apagado
  digitalWrite(rtd_off_pin, HIGH);
  digitalWrite(ph_off_pin, HIGH);
  digitalWrite(orp_off_pin, HIGH);
  digitalWrite(ec_off_pin, HIGH);
  digitalWrite(do_off_pin, HIGH);
  digitalWrite(gps_switch_pin, LOW);
  digitalWrite(reset_on_pin,LOW);
  delay(1000);
  
  // Se inicia el módulo LoRaWAN para el estándar indicado en Chile (Australia 915 MHz)
  if (!modem.begin(AU915)) {
    Serial.println("-- NO SE HA PODIDO INICIAR EL MÓDULO LORAWAN --");
    while (1) {}
  };

  // Se imprime la versión del módulo LoRaWAN de Arduino
  Serial.print("-- LA VERSIÓN DE TU MÓDULO ES: ");
  Serial.print(modem.version());
  Serial.println(" --");

  // Se imprime el número único del dispositivo
  Serial.print("-- EL EUI DE TU DISPOSITIVO ES: ");
  Serial.print(modem.deviceEUI());
  Serial.println(" --");

  // Este comando se utiliza para fijar el rango de frecuencia de comunicación al aceptado por TTN (The Things Network)
  Serial.println(modem.sendMask("ff000001f000ffff00020000"));

  // Se conecta el módulo a la red LoRaWAN disponible a través de OTAA (Over The Air Activation)
  int connected = modem.joinOTAA(appEui, appKey);

  // Se imprime el estado de conexión con la red LoRaWAN
  Serial.print("-- CONNECTED STATUS: ");
  Serial.print(connected);
  Serial.println(" --");

  // Se declara el intervalo mínimo de envío de datos desde el módulo (esta función parece no tener ningún efecto en la ejecución del código)
   modem.minPollInterval(300);

  // Se inicia la librería Wire para la comunicación con los sensores I2C
  Wire.begin();
  //Se establece la fecha  y hora actual en el modulo RTC DS3231
  //set_real_time();            //Descomentar si se requiere actualizar la hora y volvera cargar comentandolo luego para que no haya una desincronización
  // Se obtiene el valor de tiempo real y la variable temporal del dia.
  get_real_time();
  _rday = _day;

  // Se inicia la librería SPI para poder almacenar los datos en la memoria SD
  SPI.begin();
  delay(100);
  // Se inicia la librería SD       para almacenar los datos en la memoria SD
  SD.begin(sd_cs_pin);
  dataFile = SD.open("log-0000.csv", FILE_WRITE);
  delay(1000);
  // Se almacena la cabecera de los datos a almacenar 
  dataFile.println("DissolvedOxygen,pH,ElectricalConductivity,TotalDissolvedSolids,Salinity,RelativeDensity,WaterTemperature,InternalPressure,AtmosphericPressure,AtmosphericTemperature,Latitude,Longitude,InternalTemperature,InternalHumidity,Batt_analog,ORP,Saturation,DissolvedOxygenTemp,SaturationTemp,ElectricalConductivityTemp,pHTemp,DO15C,Year,Month,Day,Hour,Minutes,Seconds,Batt_voltage");
  dataFile.close();
  delay(100);
  SPI.end();
}

// Inicio del ciclo infinito
void loop() {
  // Se encienden los sensores de Atlas Scientific
  digitalWrite(rtd_off_pin, HIGH);
  digitalWrite(ph_off_pin, HIGH);
  digitalWrite(orp_off_pin, HIGH);
  digitalWrite(ec_off_pin, HIGH);
  digitalWrite(do_off_pin, HIGH);
  delay(1000);
  Serial.println("on");
  //Se obtiene los valores de fecha y hora actual
  get_real_time();
  delay(500);
  //Se comprueba el cambio de dia para el reset automatico
  if (_rday != _day){
   digitalWrite(reset_on_pin,HIGH);
  }

  // Se obtiene la medición de temperatura del agua y se almacena el dato para utilizarlo posteriormente en la petición de datos con ajuste de temperatura
  rtd_wire_transmission();
  inst_temp = String(_data[6]); // Variable utilizada para el ajuste de temperatura que viene de rtd_wire_transmission()
  // Se solicitan las mediciones de los sensores Atlas Scientific (En orden: DO, pH, EC, ORP, DOtemp, ECtemp, DO15ºC, pHtemp)
  delay(1000);
  do_wire_transmission();
  delay(1000);
  ph_wire_transmission();
  delay(1000);
  ec_wire_transmission();
  delay(1000);
  orp_wire_transmission();
  delay(1000);
  do_temp_wire_transmission();
  delay(1000);
  ec_temp_wire_transmission();
  delay(1000);
  do_15_wire_transmission();
  delay(1000);
  ph_temp_wire_transmission();
  delay(1000);

  // Se lee la humedad, presión y temperatura interna
  env_pressure();

  // Se leen los datos de temperatura y presión del medio ambiente
  get_atm_values();

  // Se apagan los sensores Atlas Scientific
  sleep_sensors();
  digitalWrite(rtd_off_pin, LOW);
  digitalWrite(ph_off_pin, LOW);
  digitalWrite(orp_off_pin, LOW);
  digitalWrite(ec_off_pin, LOW);
  digitalWrite(do_off_pin, LOW);

  // Se leen los datos obtenidos desde el módulo GPS
  digitalWrite(gps_switch_pin, HIGH);
  get_gps_data();
  digitalWrite(gps_switch_pin, LOW);

  // Se lee el nivel de voltaje de la batería
  read_battery_level();

  // Se imprimen los datos obtenidos desde los sensores en el puerto Serial 
  Serial.println("FLOAT DATA:");
  Serial.print(" [");
  for (int i = 0; i < _data_size ; i++) {
    Serial.print(_data[i]);
    if (i < _data_size -1) {
      Serial.print(", ");
    }
  }
  Serial.println("]");

  // Se almacenan los datos en la memoria SD
  write_to_sd(_data[0],_data[1],_data[2],_data[3],_data[4],_data[5],_data[6],_data[7],_data[8],_data[9],
  _data[10],_data[11], _data[12], _data[13], _data[14], _data[15], _data[16], _data[17], _data[18], _data[19],
   _data[20], _data[21], _data[22], _data[23], _data[24], _data[25], _data[26], _data[27], _data[28]);

  // Se transforman algunos datos de flotante a sus bytes componentes para enviarlos a través de LoRaWAN 
  float2Bytes(gps_latitude,&gps_latitude_float_bytes[0]);
  float2Bytes(gps_longitude,&gps_longitude_float_bytes[0]);
  float2Bytes(_data[2] /*EC*/,&ec_float_bytes[0]);
  float2Bytes(_data[3] /*TDS*/,&tds_float_bytes[0]);
  float2Bytes(_data[15] /*ORP*/,&orp_float_bytes[0]);
  float2Bytes(_data[0] /*DO*/,&do_float_bytes[0]);
  float2Bytes(_data[17] /*DOTEMP*/,&do_temp_float_bytes[0]);
  float2Bytes(_data[19] /*ECTEMP*/,&ec_temp_float_bytes[0]);  
  float2Bytes(_data[21] /*DO15*/,&do_15_float_bytes[0]);  

  //_data_lorawan[0]  = do_float_bytes[0];                            // DO 
  _data_lorawan[0]  = do_float_bytes[1];                            // DO 
  _data_lorawan[1]  = do_float_bytes[2];                            // DO 
  _data_lorawan[2]  = do_float_bytes[3];                            // DO 
    
  _data_lorawan[3]  = uint8_t  (_data[1]  * 255/14.0);              // pH   
                       
  //_data_lorawan[5]  =   ec_float_bytes[0];                          // Electrical Conductivity
  _data_lorawan[4]  =   ec_float_bytes[1];                          // Electrical Conductivity
  _data_lorawan[5]  =   ec_float_bytes[2];                          // Electrical Conductivity
  _data_lorawan[6]  =   ec_float_bytes[3];                          // Electrical Conductivity
  
  //_data_lorawan[9]   =   tds_float_bytes[0];                         // Total Dissolved Solids    
  _data_lorawan[7]  =   tds_float_bytes[1];                        // Total Dissolved Solids    
  _data_lorawan[8]  =   tds_float_bytes[2];                        // Total Dissolved Solids    
  _data_lorawan[9]  =   tds_float_bytes[3];                        // Total Dissolved Solids   
   
  _data_lorawan[10]  = uint8_t  (_data[4]  * 255/42.0);             // Salinity  
     
  _data_lorawan[11]  = uint8_t  ((_data[5] - 1) * 255/0.3);         // Relative Density   
   
  _data_lorawan[12]  = uint8_t  (_data[6]  * 255/60.0);             // Water Temperature 
  
  _data_lorawan[13]  = uint8_t  ((_data[7] - 80) * 255/120.0);      // Internal Pressure
    
  _data_lorawan[14]  = uint8_t  ((_data[8]  - 80) * 255/40.0);      // Atmospheric Pressure 
   
  _data_lorawan[15]  = uint8_t  ((_data[9] + 20) * 255/80.0);       // Atmospheric Temperature   
  
  _data_lorawan[16] =   gps_longitude_float_bytes[0];               // GPS Longitude      
  _data_lorawan[17] =   gps_longitude_float_bytes[1];               // GPS Longitude  
  _data_lorawan[18] =   gps_longitude_float_bytes[2];               // GPS Longitude   
  _data_lorawan[19] =   gps_longitude_float_bytes[3];               // GPS Longitude   
   
  _data_lorawan[20] =   gps_latitude_float_bytes[0];                // GPS Latitude 
  _data_lorawan[21] =   gps_latitude_float_bytes[1];                // GPS Latitude 
  _data_lorawan[22] =   gps_latitude_float_bytes[2];                // GPS Latitude 
  _data_lorawan[23] =   gps_latitude_float_bytes[3];                // GPS Latitude 
  
  _data_lorawan[24] = uint8_t  ((_data[12] + 20) * 255/80.0);       // Internal Temperature
  
  _data_lorawan[25] = uint8_t  (_data[13] * 255/120.0);             // Internal Humidity 

  _data_lorawan[26] = uint8_t  ((batt_voltage -9) * 255/6);           // Battery voltage
  
  //_data_lorawan[30] =   orp_float_bytes[0];                            // ORP
  //_data_lorawan[31] =   orp_float_bytes[1];                            // ORP
  _data_lorawan[27] =   orp_float_bytes[2];                            // ORP
  _data_lorawan[28] =   orp_float_bytes[3];                            // ORP

  _data_lorawan[29] = uint8_t (_data[16] * 255/150.0);                   // SAT
  
  _data_lorawan[30] =   do_temp_float_bytes[0];                        // Temperature compensated DO
  _data_lorawan[31] =   do_temp_float_bytes[1];                        // Temperature compensated DO
  _data_lorawan[32] =   do_temp_float_bytes[2];                        // Temperature compensated DO
  _data_lorawan[33] =   do_temp_float_bytes[3];                        // Temperature compensated DO

  _data_lorawan[34] = uint8_t (_data[18] * 255/150.0);                   // Temperature compensated saturation 

  _data_lorawan[35] = ec_temp_float_bytes[0];                          // Temperature compensated EC
  _data_lorawan[36] = ec_temp_float_bytes[1];                          // Temperature compensated EC
  _data_lorawan[37] = ec_temp_float_bytes[2];                          // Temperature compensated EC
  _data_lorawan[38] = ec_temp_float_bytes[3];                          // Temperature compensated EC

  _data_lorawan[39] = uint8_t (_data[20] * 255/14.0);                  // Temperature compensated pH

  _data_lorawan[40] = do_15_float_bytes[0];
  _data_lorawan[41] = do_15_float_bytes[1];
  _data_lorawan[42] = do_15_float_bytes[2];
  _data_lorawan[43] = do_15_float_bytes[3];

  _data_lorawan[44] = uint8_t (_data[22] - 2000);                      // Year
  _data_lorawan[45] = uint8_t (_data[23]);                             // Month
  _data_lorawan[46] = uint8_t (_data[24]);                             // Day

  _data_lorawan[47] = uint8_t (_data[25]);                             // Hour
  _data_lorawan[48] = uint8_t (_data[26]);                             // Minutes
  _data_lorawan[49] = uint8_t (_data[27]);                             // Seconds
  
  // Se imprimen los datos de LoRaWAN en hexadecimal 
  Serial.println("LORAWAN HEX DATA: ");
  
  for(int a = 0; a < sizeof(_data_lorawan); a++)
  {
    Serial.print("0x");
    Serial.print(_data_lorawan[a], HEX);
    Serial.print(" ");
  }

  Serial.println();

  // Se envían los datos a través de LoRaWAN
  int err;
  modem.beginPacket();
  modem.write(_data_lorawan[0]);
  modem.write(_data_lorawan[1]);
  modem.write(_data_lorawan[2]);
  modem.write(_data_lorawan[3]);
  modem.write(_data_lorawan[4]);
  modem.write(_data_lorawan[5]);
  modem.write(_data_lorawan[6]);
  modem.write(_data_lorawan[7]);
  modem.write(_data_lorawan[8]);
  modem.write(_data_lorawan[9]);
  modem.write(_data_lorawan[10]);
  modem.write(_data_lorawan[11]);
  modem.write(_data_lorawan[12]);
  modem.write(_data_lorawan[13]);
  modem.write(_data_lorawan[14]);
  modem.write(_data_lorawan[15]);
  modem.write(_data_lorawan[16]);
  modem.write(_data_lorawan[17]);
  modem.write(_data_lorawan[18]);
  modem.write(_data_lorawan[19]);
  modem.write(_data_lorawan[20]);
  modem.write(_data_lorawan[21]);
  modem.write(_data_lorawan[22]);
  modem.write(_data_lorawan[23]);
  modem.write(_data_lorawan[24]);
  modem.write(_data_lorawan[25]);
  modem.write(_data_lorawan[26]);
  modem.write(_data_lorawan[27]);
  modem.write(_data_lorawan[28]);
  modem.write(_data_lorawan[29]);
  modem.write(_data_lorawan[30]);
  modem.write(_data_lorawan[31]);
  modem.write(_data_lorawan[32]);
  modem.write(_data_lorawan[33]);
  modem.write(_data_lorawan[34]);
  modem.write(_data_lorawan[35]);
  modem.write(_data_lorawan[36]);
  modem.write(_data_lorawan[37]);
  modem.write(_data_lorawan[38]);
  modem.write(_data_lorawan[39]);
  modem.write(_data_lorawan[40]);
  modem.write(_data_lorawan[41]);
  modem.write(_data_lorawan[42]);
  modem.write(_data_lorawan[43]);
  modem.write(_data_lorawan[44]);
  modem.write(_data_lorawan[45]);
  modem.write(_data_lorawan[46]);
  modem.write(_data_lorawan[47]);
  modem.write(_data_lorawan[48]);
  modem.write(_data_lorawan[49]);
  
  err = modem.endPacket(true);

  if (err > 0) {
    Serial.println("-- MENSAJE ENVIADO CORRECTAMENTE A TRAVÉS DE LORAWAN --");
  } else {
    Serial.println("-- ERROR ENVIANDO EL MENSAJE A TRAVÉS DE LORAWAN --");
  }

  // Se envía el sistema a dormir por la cantidad puesta en la variable sleep time
  //delay(360*1000);
  LowPower.sleep(sleep_time*60*1000); // e.g. 10 minutos * 60*segundos * 1000 milisegundos
}
