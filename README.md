[README (2).md](https://github.com/user-attachments/files/29170040/README.2.md)
# Monitor UV — Sistema de Monitoreo de Radiación Ultravioleta

Sistema de monitoreo UV autónomo basado en **ESP32-S3** y sensor **GY-ML8511**. El dispositivo mide la radiación ultravioleta en tiempo real, indica el nivel de riesgo mediante un LED RGB y funciona como su propio punto de acceso WiFi, sirviendo un dashboard web local que muestra el índice UV, una recomendación de protección y un resumen gráfico del día. No requiere conexión a internet.

Proyecto desarrollado para el curso **TEI201 — Taller de Diseño en Ingeniería**, Universidad Adolfo Ibáñez.

---

## Integrantes

- Leo Castro
- Sara Orellana
- Cristian Yañez


---

## El problema

[Completar con el problema definido en el Avance 1. A modo de borrador:]

Las personas que pasan tiempo al aire libre no cuentan con una forma directa e inmediata de saber qué tan intensa es la radiación ultravioleta a la que están expuestas ni cuándo deben tomar medidas de protección. La sobreexposición al sol es un factor de riesgo importante para la salud de la piel, y la información disponible suele ser general y no refleja las condiciones del lugar exacto donde se encuentra la persona.

Este proyecto aborda ese problema entregando una medición local y en tiempo real de la radiación UV, junto con una recomendación clara de protección según el nivel detectado.

---

## ¿Qué hace el sistema?

El sistema implementa el ciclo completo de un dispositivo IoT:

1. **Captura.** El sensor GY-ML8511 mide la intensidad de la radiación ultravioleta y el ESP32-S3 la convierte en un índice UV.
2. **Información.** El índice se clasifica en una categoría de riesgo (baja, moderada, alta, muy alta, extrema) según la tabla del Índice UV.
3. **Decisión.** El dispositivo comunica el resultado de dos formas: un LED RGB que cambia de color según el riesgo y un dashboard web que muestra el valor, una recomendación de protección y un resumen del día con los valores máximo y mínimo registrados.

---

## Componentes necesarios

| Componente | Cantidad | Especificación | Función |
|---|---|---|---|
| ESP32-S3 | 1 | Microcontrolador con WiFi | Procesamiento, lectura del sensor y servidor web |
| Sensor UV GY-ML8511 | 1 | Salida analógica | Medición de radiación ultravioleta |
| LED RGB | 1 | Cátodo común | Indicación visual del nivel de riesgo |
| Resistencias | 3 | 220 Ω | Limitación de corriente del LED |
| Batería LiPo | 1 | 3.7 V recargable | Alimentación autónoma |
| Controlador de carga | 1 | Li-Po Rider Pro | Gestión de carga de la batería |
| Panel solar | 1 | 5 V de salida | Recarga de la batería |
| Cables / protoboard | — | — | Conexiones |

### Conexiones

| Señal | Pin del ESP32-S3 |
|---|---|
| Salida del sensor (OUT) | GPIO4 |
| LED Rojo | GPIO5 |
| LED Verde | GPIO6 |
| LED Azul | GPIO7 |
| VCC del sensor | 3.3 V |
| GND | GND |

> Nota: si el LED RGB es de ánodo común, cambiar la constante `ANODO_COMUN` a `true` en el firmware.

---

## Estructura del repositorio

```
monitor-uv-tei201/
├── README.md
├── FUENTES.md
├── firmware/
│   └── monitor_uv_esp32.ino     Código del ESP32 (sensor, LED y servidor web)
├── hardware/
│   ├── esquematico.pdf
│   └── BOM.xlsx
├── diseño-3d/
│   ├── encapsulado.f3d
│   ├── planos.pdf
│   └── renders/
├── testing/
│   └── protocolo_pruebas.pdf
└── docs/
    └── reporte_final.pdf
```

---

## Cómo cargar el firmware

1. Instalar el [Arduino IDE](https://www.arduino.cc/en/software) y agregar el soporte para placas ESP32 (Gestor de Tarjetas).
2. Abrir el archivo `firmware/monitor_uv_esp32.ino`.
3. En **Herramientas → Placa**, seleccionar **ESP32-S3**.
4. Conectar la placa por USB y seleccionar el puerto correspondiente.
5. Subir el código con el botón **Cargar**.

El firmware utiliza las librerías `WiFi.h` y `WebServer.h`, que vienen incluidas con el núcleo de ESP32; no es necesario instalar librerías externas.

---

## Cómo usar el dashboard

El dashboard lo sirve el propio ESP32, por lo que **no necesita estar alojado en ningún servidor ni requiere internet**.

1. Encender el dispositivo.
2. Desde un celular o computador, conectarse a la red WiFi que crea el ESP32:
   - **Red:** `MonitorUV`
   - **Clave:** `uv123456`
3. Abrir un navegador e ingresar a la dirección: **http://192.168.4.1**

La página toma una lectura cada 5 minutos, muestra el índice UV actual con su recomendación y va construyendo el gráfico del día con los valores máximo y mínimo.

### Probar el dashboard sin el dispositivo

El archivo del dashboard (`firmware/monitor_uv.html`, incluido también dentro del firmware) puede abrirse directamente en un navegador. Al pulsar el botón **"Simular día (demo)"** se genera un día completo de datos de prueba, lo que permite revisar todas las secciones sin tener el hardware conectado.

---

## Declaración de fuentes

El uso de librerías, código externo y herramientas de inteligencia artificial está documentado en el archivo [`FUENTES.md`](FUENTES.md), de acuerdo con el código de honor de la Universidad Adolfo Ibáñez.
