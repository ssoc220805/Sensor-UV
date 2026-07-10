/*
  =====================================================================
  Monitoreo UV - ESP32-S3 + Hostinger + Telegram
  Proyecto: Centinela UV
  =====================================================================
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ------------------- WIFI -------------------
const char* WIFI_SSID = "iPhone de Leo";
const char* WIFI_PASS = "123456789";

// ------------------- SERVIDOR HOSTINGER -------------------
const char* SERVIDOR  = "https://lightskyblue-zebra-707371.hostingersite.com/guardar.php";
const char* TEST_URL  = "https://lightskyblue-zebra-707371.hostingersite.com/test.php";
const char* CLAVE_API = "uv-grupo5-2026";

// Envía a Hostinger cada 3 segundos
const unsigned long INTERVALO_ENVIO = 3000;

// ------------------- TELEGRAM -------------------
const char* TG_TOKEN  = "8861657158:AAEg2c3pC8nNyDoterQ6hZ82UYEzgJWsybA";
const char* TG_CHATID = "8648240921";

const unsigned long COOLDOWN_TG = 30UL * 1000UL;   // 30 s (modo demo; normal: 5UL * 60UL * 1000UL)

// ------------------- PINES -------------------
const int PIN_UV = 4;   // OUT del sensor (GPIO4, ADC)

// Ojo: dejo estos pines como los tenías en tu código.
// Si tu LED está conectado a GPIO5, GPIO6 y GPIO7, cámbialos aquí.
const int PIN_R  = 40;
const int PIN_G  = 39;
const int PIN_B  = 38;

const bool ANODO_COMUN = false;

// ------------------- VARIABLES -------------------
float uvIntensity = 0.0;
float temperatura = 0.0;
float humedad = 0.0;

String nivel = "BAJO";
String nivelAnterior = "BAJO";

unsigned long ultimoEnvio = 0;
unsigned long ultimoTgMod = 0;
unsigned long ultimoTgAlto = 0;
unsigned long pruebaHasta = 0;

// ------------------- FUNCIONES -------------------
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setColor(bool r, bool g, bool b) {
  if (ANODO_COMUN) {
    r = !r;
    g = !g;
    b = !b;
  }

  digitalWrite(PIN_R, r);
  digitalWrite(PIN_G, g);
  digitalWrite(PIN_B, b);
}

float leerUV() {
  long suma = 0;

  for (int i = 0; i < 5; i++) {
    suma += analogRead(PIN_UV);
    delay(1);
  }

  float promedio = suma / 5.0;
  float voltaje  = promedio * (3.3 / 4095.0);

  // Conversión aproximada del GY-ML8511
  float uv = mapFloat(voltaje, 0.99, 2.8, 0.0, 15.0);

  if (uv < 0) uv = 0;
  if (uv > 20) uv = 20;

  return uv;
}

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Conectando a WiFi: ");
  Serial.println(WIFI_SSID);

  unsigned long inicio = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 20000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("No se pudo conectar al WiFi.");
    Serial.println("Revisa hotspot, clave y compatibilidad 2.4 GHz.");
  }
}

// Envía la lectura al dashboard usando POST
void enviarDato() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado. Reintentando...");
    conectarWiFi();
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String cuerpo = "key=" + String(CLAVE_API) +
                  "&uv=" + String(uvIntensity, 1) +
                  "&temp=" + String(temperatura, 1) +
                  "&hum=" + String(humedad, 0);

  Serial.print("Enviando a Hostinger: ");
  Serial.println(cuerpo);

  http.begin(client, SERVIDOR);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int codigo = http.POST(cuerpo);
  String respuesta = http.getString();

  Serial.print("Dashboard HTTP: ");
  Serial.println(codigo);

  Serial.print("Respuesta servidor: ");
  Serial.println(respuesta);

  http.end();
}

void enviarTelegram(String mensaje) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  mensaje.replace(" ", "%20");
  mensaje.replace("\n", "%0A");

  String url = "https://api.telegram.org/bot" + String(TG_TOKEN) +
               "/sendMessage?chat_id=" + String(TG_CHATID) +
               "&text=" + mensaje;

  http.begin(client, url);
  int codigo = http.GET();

  Serial.print("Telegram HTTP: ");
  Serial.println(codigo);

  http.end();
}

void revisarPrueba() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, TEST_URL);

  int codigo = http.GET();

  if (codigo == 200) {
    String resp = http.getString();
    resp.trim();

    if (resp == "1") {
      pruebaHasta = millis() + 8000;
      enviarTelegram("PRUEBA: alerta de UV ALTO.");
    }
  }

  http.end();
}

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Iniciando Centinela UV...");

  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_UV, ADC_11db);

  setColor(0, 0, 0);

  conectarWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    enviarTelegram("Sistema Monitor UV encendido.");
  }
}

// ------------------- LOOP -------------------
void loop() {
  static unsigned long tiempoMedicion = 0;

  if (millis() - tiempoMedicion > 120) {
    tiempoMedicion = millis();

    uvIntensity = leerUV();

    if (uvIntensity < 3.0) {
      nivel = "BAJO";
      setColor(0, 1, 0);   // verde
    } else if (uvIntensity < 6.0) {
      nivel = "MODERADO";
      setColor(1, 1, 0);   // amarillo
    } else {
      nivel = "ALTO";
      setColor(1, 0, 0);   // rojo
    }

    if (millis() < pruebaHasta) {
      nivel = "ALTO";
      setColor(1, 0, 0);
    }

    Serial.print("UV: ");
    Serial.print(uvIntensity, 2);
    Serial.print(" -> ");
    Serial.println(nivel);

    if (nivel == "MODERADO" && nivelAnterior == "BAJO" &&
        (ultimoTgMod == 0 || millis() - ultimoTgMod > COOLDOWN_TG)) {
      enviarTelegram("Aviso: UV moderado. Indice " + String(uvIntensity, 1));
      ultimoTgMod = millis();
    }

    if (nivel == "ALTO" && nivelAnterior != "ALTO" &&
        (ultimoTgAlto == 0 || millis() - ultimoTgAlto > COOLDOWN_TG)) {
      enviarTelegram("ALERTA UV ALTO. Indice " + String(uvIntensity, 1));
      ultimoTgAlto = millis();
    }

    nivelAnterior = nivel;
  }

  if (millis() - ultimoEnvio > INTERVALO_ENVIO) {
    ultimoEnvio = millis();
    enviarDato();
  }

  static unsigned long ultimaPrueba = 0;

  if (millis() - ultimaPrueba > 6000) {
    ultimaPrueba = millis();
    revisarPrueba();
  }
}
