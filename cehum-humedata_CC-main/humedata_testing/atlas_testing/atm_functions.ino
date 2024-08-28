void get_atm_values(){
  bmp.begin(0x76);
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
  Adafruit_BMP280::SAMPLING_X2,                     /* Temp. oversampling */
  Adafruit_BMP280::SAMPLING_X16,                    /* Pressure oversampling */
  Adafruit_BMP280::FILTER_X16,                      /* Filtering. */
  Adafruit_BMP280::STANDBY_MS_500);                 /* Standby time. */
  atm_pressure = bmp.readPressure()/1000;
  atm_temperature = bmp.readTemperature();
  
//  Serial.print("-- EXTERNAL PRESSURE: ");
  _data[8] = atm_pressure;
//  Serial.print(atm_pressure);
//  Serial.println(" KPA --");
  
  
//  Serial.print("-- EXTERNAL TEMPERATURE: ");
  _data[9] = atm_temperature;
//  Serial.print(atm_temperature);
//  Serial.println(" °C --");

}

void get_gps_data(){
  while(Serial1.available() > 0){
    if(gps.encode(Serial1.read())){
      gps_latitude = gps.location.lat();
      gps_longitude = gps.location.lng();
    }
  }
//    Serial.println(gps_longitude);
  _data[11] = gps_longitude;
//    Serial.print("LON: ");
  _data[10] = gps_latitude;

}
