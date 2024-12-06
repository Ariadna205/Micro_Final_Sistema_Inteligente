// Librerías necesarias
#include <Wire.h>
#include <LiquidCrystal_I2C.h> // Librería para el LCD I2C
#include <DHT.h> // Librería del sensor DHT
#include <AsyncTaskLib.h>  // Librería para tareas asíncronas
#include <WiFi.h>
#include "time.h"
// Configuración del DHT22
#define DHTPIN 4       // Pin del sensor DHT22
#define DHTTYPE DHT22  // Tipo de sensor DHT22
DHT dht(DHTPIN, DHTTYPE);
// Configuración del PIR
#define PIR_PIN 2 // Pin del PIR
// Configuración del IR
#define IRPIN 5 // Pin del sensor IR
#define LED 13  // Pin del LED
#define FAN 25
#define SOUND_SENSOR_PIN 34
#define BUZZER 26 // Pin asignado para el buzzer
// Variables para estados
bool ledState = LOW;        // Estado actual del LED
bool lastIRState = LOW;     // Último estado leído del sensor IR
bool movimientoDetectado = false; // Estado del PIR
// Variables para el sensor de sonido
int clapCount = 0;           // Contador de aplausos
unsigned long lastClapTime = 0; // Tiempo del último aplauso
const int clapInterval = 1000;  // Intervalo máximo entre aplausos (en ms)
//Variables para WIFI
//const char* ssid = "FAMILIA_TORRES";     // Reemplaza con tu red WiFi
//const char* password = "1058935905"; // Reemplaza con tu contraseña
const char* ssid = "TP-LINK_DE16";
const char* password = "67743667";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000; // Ajusta según tu zona horaria (GMT-5)
const int daylightOffset_sec = 3600; // Ajuste para horario de verano
// Variable para controlar si la alerta ya se mostró
bool alertaMostrada = false;
// Configuración del LCD (dirección 0x27, 20 columnas, 4 filas)
LiquidCrystal_I2C lcd(0x27, 20, 4);

void temperaturaHumedadRead() {
  float temperatura = dht.readTemperature(); // °C
  float humedad = dht.readHumidity();       // %
  // Verificar errores en las lecturas
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error al leer el DHT22");
    lcd.setCursor(0, 0);
    lcd.print("Error en sensor DHT");
    return;
  }
  //Serial.println("Temperatura: ");
  //Serial.println(temperatura);
  if (temperatura > 27) {
    if (!alertaMostrada) {  // Solo muestra la alerta si aún no se ha mostrado
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ALERTA: Mucho calor");
      lcd.setCursor(0, 1);
      lcd.print("Encendiendo aire");
      delay(2000);   
      alertaMostrada = true;  // Marca la alerta como mostrada
    }
    digitalWrite(FAN, HIGH); // Enciende el ventilador
    Serial.println("Fan encendido");
  } else {
    digitalWrite(FAN, LOW); // Apaga el ventilador
    // Si la temperatura baja de nuevo, restablece la bandera para permitir la alerta en el futuro
    if (alertaMostrada) {
      alertaMostrada = false;  // Restablece la bandera para que la alerta pueda mostrarse nuevamente
    }
  }
  // Mostrar la información en el LCD
  lcd.setCursor(0, 0);
  lcd.print("Temperatura: ");
  lcd.print(temperatura);
  lcd.print((char)223);            // Símbolo de grado
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Humedad: ");
  lcd.print(humedad);
  lcd.print(" %");
}

// Declaración de tareas asincrónicas
AsyncTask temperaturaHumedadTask(2000, true, temperaturaHumedadRead);

void sensorIRRead(void) {
  bool currentIRState = digitalRead(IRPIN);
  movimientoDetectado = digitalRead(PIR_PIN);  // Leer estado del PIR

  //lcd.setCursor(0, 2);
  //lcd.print("Mov: ");
  //lcd.print(movimientoDetectado ? "Detectado   " : "No detectado");

  // Detecta flanco de subida en el sensor IR y verifica el PIR
  if (currentIRState == HIGH && lastIRState == LOW && movimientoDetectado) {              
    // Obtener la hora actual desde time.h
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {  // Obtiene la hora actual
      int currentHour = timeinfo.tm_hour;  // Hora actual
      int currentMinute = timeinfo.tm_min;  // Minuto actual
      if (currentHour >= 6 && currentHour < 12) {
          // Buenos días
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("BUENOS DIAS :)");
          lcd.setCursor(0, 1);
          lcd.print("Evita encender luz");
          delay(4000);
          // Activar buzzer por 500 ms
          digitalWrite(BUZZER, HIGH);
          Serial.println("Buzzer encendido");
          delay(500);
          digitalWrite(BUZZER, LOW);
          Serial.println("Buzzer apagado");
          lcd.clear();
      } else if (currentHour >= 12 && currentHour < 16) {
          // Buenas tardes
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("BUENAS TARDES :)");
          lcd.setCursor(0, 1);
          lcd.print("Evita encender luz");
          delay(4000);
          // Activar buzzer por 500 ms
          digitalWrite(BUZZER, HIGH);
          Serial.println("Buzzer encendido");
          delay(500);
          digitalWrite(BUZZER, LOW);
          Serial.println("Buzzer apagado");
          lcd.clear();
      } else {
          // Buenas noches
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("BUENAS NOCHES :)");
          lcd.setCursor(0, 1);
          lcd.print("Puedes usar la luz");
          delay(4000);
          lcd.clear();
      }

      // Enciende el LED independientemente del horario
      ledState = HIGH; 
      digitalWrite(LED, ledState);
      Serial.println("LED ENCENDIDO con IR");
    }
  }    
  lastIRState = currentIRState; // Actualiza el estado previo del sensor IR
  //lcd.setCursor(0, 3);
  //lcd.print("LED: ");
  //lcd.print(ledState ? "Encendido   " : "Apagado     ");
}
AsyncTask sensorIRTask(20, true, sensorIRRead);
  int getFilteredSoundLevel() {
    static int samples[10];
    static int index = 0;
    static unsigned long lastSampleTime = 0;
    
    if (millis() - lastSampleTime >= 10) {  // Muestrea cada 10ms sin bloquear
        samples[index] = analogRead(SOUND_SENSOR_PIN);
        index = (index + 1) % 10;
        lastSampleTime = millis();
    }
    
    // Calcular promedio
    int total = 0;
    for (int i = 0; i < 10; i++) {
        total += samples[i];
    }
    return total / 10;
  }

void sensorSonidoRead(void) {

  int soundDetected = getFilteredSoundLevel();

  if (soundDetected > 400) {
    unsigned long currentTime = millis();
    if (currentTime - lastClapTime < clapInterval) {
      clapCount++;
    } else {
      clapCount = 1;
    }
    lastClapTime = currentTime;

    Serial.print("Aplausos detectados: ");
    Serial.println(clapCount);

    if (clapCount == 2) { // Dos aplausos detectados
      ledState = LOW;
      digitalWrite(LED, ledState);
      Serial.println("LED APAGADO con sensor de sonido");
      clapCount = 0; // Reiniciar contador
    }
  }
}

AsyncTask sensorSonidoTask(50, true, sensorSonidoRead);

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" Conectado a WiFi");
}

void printNTPTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Error obteniendo la hora NTP");
    return;
  }

  // Muestra la hora en el LCD
  lcd.setCursor(0, 2);
  lcd.printf("Hora: %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  lcd.setCursor(0, 3);
  lcd.printf("Fecha: %02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
}

void updateNTPTime() {
  printNTPTime();
}
AsyncTask ntpTimeTask(1000, true, updateNTPTime);


void setup() {
  // Consola de depuración
  Serial.begin(9600);
  //Configuración del WIFI
  setupWiFi();
  // Sincroniza la hora con un servidor NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // Configuración de pines
  pinMode(LED, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(IRPIN, INPUT);
  pinMode(SOUND_SENSOR_PIN, INPUT);
  pinMode(FAN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  digitalWrite(FAN, LOW); // Apaga el buzzer inicialmente
  digitalWrite(LED, LOW); // Apaga el LED inicialmente
  // Inicia el sensor DHT
  dht.begin();
  // Inicia el LCD
  lcd.init();
  lcd.backlight();
  // Mensaje de inicio en el LCD
  lcd.setCursor(0, 0);
  lcd.print("Sistema iniciado...");
  delay(2000); // Espera 2 segundos para mostrar el mensaje inicial
  lcd.clear();
  Serial.println("Sistema iniciado...");
  // Inicializar tareas asincrónicas
  temperaturaHumedadTask.Start();
  sensorIRTask.Start();
  sensorSonidoTask.Start();
  ntpTimeTask.Start();
}

void loop() {
  // Ejecutar tareas asincrónicas
  temperaturaHumedadTask.Update();
  sensorIRTask.Update();
  sensorSonidoTask.Update();
  ntpTimeTask.Update();
  // Aquí, las lecturas de sensores se manejarán de forma asíncrona, evitando bloqueos
}






