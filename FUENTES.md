# Declaración de Fuentes e Inteligencia Artificial

Este documento declara las librerías, los servicios externos, el código externo y las herramientas de inteligencia artificial que usamos para desarrollar **Centinela UV**. La idea es ser transparentes sobre qué partes del trabajo se apoyaron en recursos externos y dejar claro que entendemos todo el código que forma parte del sistema, de acuerdo con el código de honor de la Universidad Adolfo Ibáñez.

Los números de línea corresponden al archivo `firmware/centinela_uv2.ino`.

---

## 1. Librerías utilizadas

| Librería | Versión | Para qué la usamos | Fuente |
|---|---|---|---|
| `WiFi.h` | core ESP32 `<completar>` | Conectar el ESP32-S3 a la red WiFi en modo estación (STA) | https://github.com/espressif/arduino-esp32 |
| `WiFiClientSecure.h` | core ESP32 `<completar>` | Abrir la conexión HTTPS hacia Hostinger y hacia la API de Telegram | https://github.com/espressif/arduino-esp32 |
| `HTTPClient.h` | core ESP32 `<completar>` | Hacer las peticiones POST y GET al servidor y al bot | https://github.com/espressif/arduino-esp32 |

Las tres librerías vienen incluidas con el núcleo de ESP32 para Arduino, así que no fue necesario instalar nada aparte. La versión es la del núcleo instalado y se revisa en **Arduino IDE → Herramientas → Placa → Gestor de Tarjetas → esp32**.

> Nota para el equipo: completar los tres campos de versión antes de entregar.

---

## 2. Servicios externos utilizados

| Servicio | Para qué lo usamos |
|---|---|
| Hostinger (hosting PHP) | Aloja `guardar.php`, que recibe las lecturas y las almacena, y el dashboard web |
| API de Telegram Bot | Envía las alertas al celular cuando el índice UV sube de nivel |

---

## 3. Código externo adaptado

No copiamos código completo desde tutoriales, foros ni repositorios de otras personas. Usamos como referencia conceptual los ejemplos oficiales del núcleo de ESP32 para conexión WiFi y para peticiones HTTP, y la documentación de la API de Telegram para el formato de la URL de `sendMessage`:

- Ejemplos WiFi y HTTPClient: https://github.com/espressif/arduino-esp32/tree/master/libraries
- API de Telegram: https://core.telegram.org/bots/api#sendmessage

Las partes del código que no escribimos desde cero las desarrollamos con ayuda de inteligencia artificial, y están declaradas en la sección siguiente.

---

## 4. Uso de Inteligencia Artificial

### 4.1 Lectura del sensor: `leerUV()` (líneas 71–89)

- **Herramienta:** Claude (junio 2026).
- **Prompt utilizado:** "Escribe una función en C++ para Arduino que lea un sensor analógico ML8511 con el ADC del ESP32-S3 y convierta el voltaje en índice UV."
- **Cómo lo adaptamos:** cambiamos la lectura única por el promedio de 5 muestras seguidas con 1 ms de separación, para reducir el ruido del ADC. Ajustamos la conversión a la relación del GY-ML8511 (0.99 V ≈ índice 0, 2.8 V ≈ índice 15) y agregamos una saturación entre 0 y 20 para descartar valores imposibles.
- **Comprensión:** entendemos que `analogRead()` devuelve un número entre 0 y 4095 porque configuramos 12 bits de resolución; ese número se multiplica por `3.3/4095` para obtener el voltaje real del pin; y `mapFloat()` hace una interpolación lineal entre los dos puntos de referencia del sensor. Verificamos el comportamiento en el Monitor Serial tapando y destapando el sensor.

### 4.2 Control del LED RGB: `setColor()` y clasificación en el `loop()` (líneas 59–69 y 227–260)

- **Herramienta:** Claude y Gemini (junio 2026).
- **Para qué:** apoyo para escribir la función que enciende los tres canales del LED y la lógica que decide el color según el índice UV.
- **Cómo lo adaptamos:** definimos nosotros los cortes en 3.0 y 6.0 a partir de la tabla del Índice UV de la OMS, agrupando "alto", "muy alto" y "extremo" en un solo nivel ALTO. Agregamos la constante `ANODO_COMUN` para que la función sirva con LED de ánodo o de cátodo común (usamos cátodo común, `false`).
- **Comprensión:** entendemos que el LED se controla con tres salidas digitales independientes y que el color se forma combinándolas (verde = solo G, amarillo = R + G, rojo = solo R). Si el LED fuera de ánodo común, la lógica se invierte, y por eso la función niega los tres valores.

### 4.3 Envío de datos al servidor: `enviarDato()` (líneas 119–152)

- **Herramienta:** Claude (junio 2026).
- **Para qué:** apoyo para armar la petición HTTPS hacia el script `guardar.php` en Hostinger.
- **Cómo lo adaptamos:** definimos el formato del cuerpo (`key`, `uv`, `temp`, `hum`) para que coincidiera con lo que espera nuestro `guardar.php`, y agregamos una verificación de conexión WiFi que intenta reconectar antes de enviar.
- **Comprensión:** entendemos que se abre un cliente seguro, se envía la información como un formulario (`x-www-form-urlencoded`) mediante POST, y que el servidor responde con un código HTTP; el 200 confirma que el dato quedó guardado. Usamos `client.setInsecure()` porque no cargamos el certificado raíz en el ESP32; sabemos que eso significa que no se valida la identidad del servidor, y es una limitación aceptable para un prototipo académico.

### 4.4 Alertas de Telegram: `enviarTelegram()` (líneas 154–176) y la lógica de aviso en el `loop()` (líneas 261–277)

- **Herramienta:** Claude (junio 2026).
- **Prompt utilizado:** "Cómo enviar un mensaje desde un ESP32 a un bot de Telegram usando HTTPClient, sin librerías externas."
- **Cómo lo adaptamos:** reemplazamos los espacios y saltos de línea del mensaje por `%20` y `%0A` para que la URL sea válida. Sobre esa base, nosotros escribimos la lógica de cuándo alertar: el mensaje se manda **solo cuando el nivel cambia** (comparando `nivel` con `nivelAnterior`) y respetando un tiempo de espera (`COOLDOWN_TG`), para no llenar el chat de mensajes repetidos cuando el índice oscila alrededor de un umbral.
- **Comprensión:** entendemos que la API de Telegram recibe el token del bot y el `chat_id` dentro de la URL y devuelve un código HTTP; que `nivelAnterior` es lo que evita que se repita la alerta en cada ciclo del `loop`; y que el cooldown es una segunda protección para el caso en que el índice cruce el umbral varias veces seguidas.

### 4.5 Modo demostración: `revisarPrueba()` (líneas 178–200)

- **Herramienta:** Claude (junio 2026).
- **Para qué:** necesitábamos poder mostrar la alerta funcionando durante la presentación sin depender de que hubiera sol.
- **Cómo lo adaptamos:** el ESP32 consulta cada 6 segundos un archivo `test.php` del servidor; si devuelve `1`, guarda un tiempo límite en `pruebaHasta` y manda el mensaje `PRUEBA: alerta de UV ALTO.`. Durante los 8 segundos siguientes el `loop` fuerza el estado ALTO y el LED rojo.
- **Comprensión:** entendemos que esta función **no mide nada**: fuerza el estado. Por eso los mensajes que empiezan con `PRUEBA:` no corresponden a mediciones reales, y por eso en las capturas se ve un `ALERTA UV ALTO. Indice 0.0` justo después de una prueba: el nivel se forzó aunque el sensor estuviera leyendo 0. Lo declaramos explícitamente para no confundir simulación con medición.

### 4.6 Backend: `guardar.php`, `test.php` y dashboard `index.html`

- **Herramienta:** Claude (junio 2026).
- **Para qué:** generar el script PHP que recibe cada lectura y la guarda de forma persistente en el hosting, y la página web que muestra el valor actual, la recomendación de protección y el gráfico del día con máximo, mínimo y promedio.
- **Cómo lo adaptamos:** definimos la clave de API para que el servidor rechace peticiones que no vengan de nuestro dispositivo; mantuvimos los rangos y colores que definimos como equipo; y agregamos un modo de demostración en la página para poder mostrarla completa en la presentación.
- **Comprensión:** entendemos que el ESP32 hace un POST con los datos, el PHP los valida contra la clave y los agrega al registro, y la página lee ese registro para dibujar el gráfico y calcular los estadísticos del día.

### 4.7 Estructura del README y de este archivo

- **Herramienta:** Claude (junio 2026).
- **Para qué:** generar la estructura base de `README.md` y de este `FUENTES.md`.
- **Cómo lo adaptamos:** el contenido técnico (pines, niveles, decisiones de diseño, resultados de las pruebas) lo escribimos nosotros a partir del sistema que efectivamente construimos.

### 4.8 Orientación general del proyecto

- **Herramienta:** Claude y Gemini.
- **Para qué:** pedir orientación sobre cómo avanzar en el proyecto, la organización del repositorio y dudas puntuales durante el desarrollo.
- **Nota:** este apoyo fue de orientación al proceso. Las decisiones de diseño, la selección de componentes, el montaje físico y el contenido final del proyecto los definimos nosotros como grupo.

---

## 5. Declaración final

Declaramos que las herramientas externas y la inteligencia artificial se usaron como apoyo para acelerar el desarrollo, la depuración y la documentación. Las decisiones técnicas, la integración física, las pruebas y la adaptación al problema fueron realizadas por el equipo. Todos los integrantes podemos explicar qué hace cada función del código y por qué está donde está.
