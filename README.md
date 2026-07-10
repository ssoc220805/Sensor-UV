# Centinela UV — Sistema de Monitoreo de Radiación Ultravioleta

Centinela UV es un dispositivo IoT basado en **ESP32-S3** y sensor **GY-ML8511** que mide la radiación ultravioleta, avisa el nivel de riesgo con un **LED RGB**, envía cada lectura a un **servidor web en Hostinger** donde queda guardada, y manda una **alerta por Telegram** cuando el índice UV sube a un nivel peligroso.

Proyecto desarrollado para el curso **TEI201 — Taller de Diseño en Ingeniería**, Universidad Adolfo Ibáñez.

---

## Integrantes

- Leo Castro
- Sara Orellana
- Cristian Yañez

---

## El problema

Las personas que pasan tiempo al aire libre no tienen una forma directa de saber qué tan intensa es la radiación ultravioleta a la que están expuestas, ni cuándo deberían protegerse. La información disponible suele ser general (el pronóstico de la ciudad) y no refleja las condiciones del lugar exacto donde está la persona.

Centinela UV entrega una medición local y en tiempo real de la radiación UV, junto con una recomendación clara de protección según el nivel detectado.

---

## Qué hace el sistema

El sistema cumple el ciclo completo de un dispositivo IoT:

1. **Captura.** El sensor GY-ML8511 entrega un voltaje proporcional a la radiación UV. El ESP32-S3 lo lee con su conversor analógico-digital y lo transforma en un índice UV.
2. **Almacena.** Cada 3 segundos el ESP32 envía la lectura por WiFi a un script `guardar.php` alojado en Hostinger, que la guarda de forma permanente. Los datos siguen ahí aunque el dispositivo se apague.
3. **Visualiza.** El dashboard web muestra el índice UV actual, la recomendación de protección y el resumen del día (máximo, mínimo y promedio) con un gráfico.
   → https://lightskyblue-zebra-707371.hostingersite.com/
4. **Decide.** El LED RGB cambia de color según el nivel de riesgo, y si el índice supera el umbral el sistema **manda una alerta a Telegram** para que el usuario se entere aunque no esté mirando ni el dispositivo ni la página.

---

## Arquitectura

```
Sensor GY-ML8511  →  ESP32-S3  →  WiFi  →  Hostinger (guardar.php)  →  Dashboard web
                          │
                          ├──→  LED RGB (aviso local inmediato)
                          └──→  API de Telegram (alerta al celular)
```

El ESP32 se conecta a una red WiFi existente (durante las pruebas usamos el hotspot de un celular). No sirve la página web por sí mismo: la página está alojada en Hostinger, y por eso se puede abrir desde cualquier lugar con internet, no solo estando al lado del dispositivo.

---

## Niveles de riesgo

El firmware clasifica el índice UV en tres niveles:

| Índice UV | Nivel | Color del LED | Alerta Telegram | Recomendación |
|---|---|---|---|---|
| 0.0 – 2.9 | BAJO | Verde | No | No se requiere protección especial |
| 3.0 – 5.9 | MODERADO | Amarillo | Aviso | Usar bloqueador y lentes de sol |
| 6.0 o más | ALTO | Rojo | Alerta | Buscar sombra, usar bloqueador, sombrero y lentes |

Los cortes en 3 y 6 vienen de la tabla oficial del Índice UV de la OMS. Agrupamos las categorías "alto", "muy alto" y "extremo" en un solo nivel ALTO porque la acción recomendada para el usuario es la misma: protegerse.

Para no llenar de mensajes el celular, el sistema solo envía una alerta **cuando el nivel cambia** (por ejemplo, de BAJO a MODERADO) y respeta un tiempo de espera entre alertas del mismo tipo.

---

## Componentes

| Componente | Cantidad | Especificación | Función |
|---|---|---|---|
| ESP32-S3 DevKitC-1 | 1 | WiFi 2.4 GHz, doble núcleo | Lee el sensor, controla el LED y envía los datos |
| Sensor UV GY-ML8511 | 1 | Salida analógica, 280–390 nm | Medición de radiación ultravioleta |
| LED RGB | 1 | Cátodo común, 5 mm | Indicación visual del nivel de riesgo |
| Resistencias | 3 | 220 Ω, 1/4 W | Limitan la corriente de cada canal del LED |
| Batería LiPo | 1 | 3.7 V, ~1200 mAh | Alimentación autónoma |
| Controlador Li-Po Rider Pro | 1 | Carga solar/USB, salida 5 V | Gestiona la carga y alimenta el ESP32 |
| Panel solar | 1 | 5 V, ~1 W | Recarga la batería |
| Mini protoboard | 1 | — | Montaje sin soldar |
| Cables Dupont | 1 set | 20 cm | Conexiones |

El detalle con costos está en [`hardware/Documentacion_Hardware.md`](hardware/Documentacion_Hardware.md).

---

## Conexiones

| Señal | Pin del ESP32-S3 | Nota |
|---|---|---|
| Sensor UV — VCC | 3.3 V | Alimentación del sensor |
| Sensor UV — GND | GND | Tierra común |
| Sensor UV — OUT | **GPIO4** | Entrada analógica (ADC1) |
| LED Rojo | **GPIO40** | Con resistencia de 220 Ω |
| LED Verde | **GPIO39** | Con resistencia de 220 Ω |
| LED Azul | **GPIO38** | Con resistencia de 220 Ω |
| LED — pata común | GND | LED de cátodo común |
| Batería LiPo | Li-Po Rider Pro (BAT) | Conector JST |
| Panel solar | Li-Po Rider Pro (SOLAR) | Entrada de carga solar |
| Li-Po Rider Pro — salida 5 V | VIN del ESP32-S3 | Alimenta todo el sistema |

Estos son los pines que usa el código en `firmware/centinela_uv2.ino`. La tabla, el esquemático y el código dicen lo mismo.

---

## Estructura del repositorio

```
centinela-uv-tei201/
├── README.md
├── FUENTES.md
├── .gitignore
├── firmware/
│   ├── centinela_uv2.ino          Código del ESP32
│   └── config_ejemplo.h           Plantilla de credenciales (sin claves reales)
├── backend/
│   ├── guardar.php                Recibe y guarda cada lectura
│   ├── test.php                   Activa el modo demostración
│   └── index.html                 Dashboard web
├── hardware/
│   ├── Documentacion_Hardware.md
│   ├── esquematico.png
│   └── fotos/
├── diseño-3d/
│   ├── Modelado_3D_rehechov2.f3d
│   ├── soporte__defin_.f3d
│   ├── planos.pdf
│   └── renders/
├── testing/
│   ├── Protocolo_de_pruebas.md
│   ├── Resultados.md
│   ├── datos/lecturas_23-06-2026.csv
│   └── evidencia/                 Capturas del bot de Telegram y del dashboard
└── docs/
    └── presentacion.pdf
```

---

## Cómo cargar el firmware

1. Instalar el [Arduino IDE](https://www.arduino.cc/en/software) y agregar el soporte para placas ESP32 desde el Gestor de Tarjetas.
2. Abrir `firmware/centinela_uv2.ino`.
3. En **Herramientas → Placa**, seleccionar **ESP32S3 Dev Module**.
4. Crear un archivo `config.h` a partir de `config_ejemplo.h` y completar la red WiFi, la clave de la API y las credenciales del bot de Telegram.
5. Conectar la placa por USB, seleccionar el puerto y subir el código.
6. Abrir el Monitor Serial a **115200 baudios** para ver las lecturas y confirmar que la respuesta del servidor sea `200`.

El firmware usa `WiFi.h`, `WiFiClientSecure.h` y `HTTPClient.h`, que vienen incluidas con el núcleo de ESP32. No hay que instalar librerías externas.

### Sobre las credenciales

La red WiFi, la clave de la API y el token del bot de Telegram **no se suben al repositorio**. Van en `config.h`, que está listado en `.gitignore`. Si alguien publica un token de Telegram por accidente, hay que revocarlo de inmediato con @BotFather y generar uno nuevo.

---

## Cómo usar el dashboard

El dashboard está alojado en Hostinger, así que se abre desde cualquier navegador con internet: no hace falta estar cerca del dispositivo.

1. Encender el dispositivo y verificar que se conecte al WiFi (el Monitor Serial muestra la IP).
2. Abrir https://lightskyblue-zebra-707371.hostingersite.com/
3. La página muestra la última lectura, la recomendación de protección y el gráfico del día.

Las lecturas quedan guardadas en el servidor, así que el histórico sigue disponible aunque el ESP32 se apague o se reinicie.

---

## Alertas de Telegram

El bot **Centinela UV Bot** envía tres tipos de mensaje:

- `Sistema Monitor UV encendido.` — cada vez que el dispositivo arranca y logra conectarse.
- `Aviso: UV moderado. Indice X` — cuando el índice pasa de BAJO a MODERADO.
- `ALERTA UV ALTO. Indice X` — cuando el índice llega al nivel ALTO.

Existe además un **modo demostración**: el ESP32 consulta cada 6 segundos un archivo `test.php` en el servidor, y si este devuelve `1`, el dispositivo fuerza el estado ALTO durante 8 segundos y manda el mensaje `PRUEBA: alerta de UV ALTO.`. Esto nos permite mostrar la alerta funcionando en la presentación sin depender de que haya sol. Los mensajes que empiezan con `PRUEBA:` corresponden a este modo, no a una medición real.

---

## Limitaciones conocidas

- La conversión de voltaje a índice UV usa la relación aproximada del ML8511 (0.99 V ≈ 0, 2.8 V ≈ 15). No fue calibrada contra un medidor UV de referencia, así que el valor absoluto puede tener error; lo que sí validamos es que el sensor responde correctamente a los cambios de luz.
- El firmware envía dos campos `temp` y `hum` que siempre valen 0, porque quedaron de una versión anterior del proyecto que contemplaba un sensor de temperatura. El sistema no mide temperatura ni humedad.
- El sistema necesita una red WiFi disponible. Si se cae la conexión, el dispositivo sigue midiendo y encendiendo el LED, pero no guarda ni alerta.

---

## Declaración de fuentes

El uso de librerías, código externo y herramientas de inteligencia artificial está documentado en [`FUENTES.md`](FUENTES.md), de acuerdo con el código de honor de la Universidad Adolfo Ibáñez.
