/*
  =====================================================================
  Sistema de Monitoreo UV Autonomo Local  -  Grupo 5
  Placa:   ESP32-S3
  Sensor:  GY-ML8511 (UV, salida analogica)
  Alerta:  LED RGB (verde / amarillo / rojo)
  Web:     Punto de acceso WiFi -> http://192.168.4.1
  =====================================================================
*/

#include <WiFi.h>
#include <WebServer.h>

// ------------------- PINES (ajustables) -------------------
const int PIN_UV = 4;   // OUT del sensor -> pin con ADC (entrada analogica)
const int PIN_R  = 5;   // LED Rojo
const int PIN_G  = 6;   // LED Verde
const int PIN_B  = 7;   // LED Azul (no se usa en este proyecto, queda libre)

// Si tu LED RGB es de ANODO COMUN, cambia esta linea a true.
// (Si los colores salen "al reves", cambialo y vuelve a subir el codigo.)
const bool ANODO_COMUN = false;

// ------------------- WIFI (Access Point) -------------------
const char* AP_SSID = "MonitorUV";   // nombre de la red que crea la ESP32
const char* AP_PASS = "uv123456";    // clave (minimo 8 caracteres)

WebServer server(80);

float uvIntensity = 0.0;   // intensidad UV en mW/cm2
String nivel = "BAJO";

// ------------------- FUNCIONES AUXILIARES -------------------
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Enciende el LED con el color indicado (1 = encendido, 0 = apagado)
void setColor(bool r, bool g, bool b) {
  if (ANODO_COMUN) { r = !r; g = !g; b = !b; }
  digitalWrite(PIN_R, r);
  digitalWrite(PIN_G, g);
  digitalWrite(PIN_B, b);
}

// Lee el sensor varias veces y promedia para una lectura mas estable
float leerUV() {
  long suma = 0;
  for (int i = 0; i < 16; i++) { suma += analogRead(PIN_UV); delay(2); }
  float promedio = suma / 16.0;
  float voltaje  = promedio * (3.3 / 4095.0);     // ADC 12 bits, ref 3.3V
  float uv = mapFloat(voltaje, 0.99, 2.8, 0.0, 15.0); // curva tipica ML8511
  if (uv < 0) uv = 0;
  return uv;
}

// ------------------- PAGINA WEB -------------------
String htmlPage() {
  String html = R"=====(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
<title>Monitor UV</title>
<style>
  /* ============================================================
     Tokens. Paleta de categorías = misma de la tabla IUV.
     Página pensada para leerse al aire libre (fondo claro).
     ============================================================ */
  :root{
    --paper:#F4F6F8;
    --card:#FFFFFF;
    --ink:#16202A;
    --muted:#69757F;
    --line:#E3E8ED;

    /* colores IUV (borde, chips, gráfico) */
    --c-baja:#5BB94A;
    --c-moderada:#F2DD00;
    --c-alta:#F2A100;
    --c-muyalta:#E5202E;
    --c-extrema:#BE5C97;

    --radius:18px;
    --shadow:0 1px 2px rgba(22,32,42,.06), 0 8px 24px rgba(22,32,42,.06);
  }

  *{box-sizing:border-box;}
  html,body{margin:0;}
  body{
    background:var(--paper);
    color:var(--ink);
    font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
    line-height:1.45;
    -webkit-font-smoothing:antialiased;
    padding:18px 16px calc(28px + env(safe-area-inset-bottom));
  }
  .wrap{max-width:520px;margin:0 auto;}

  /* ---------- Encabezado ---------- */
  header{display:flex;align-items:baseline;justify-content:space-between;gap:12px;margin:4px 2px 18px;}
  h1{font-size:19px;font-weight:800;letter-spacing:-.01em;margin:0;}
  .estado{font-size:12px;color:var(--muted);display:flex;align-items:center;gap:6px;white-space:nowrap;}
  .dot{width:8px;height:8px;border-radius:50%;background:var(--muted);flex:none;}
  .dot.on{background:var(--c-baja);}
  .dot.off{background:var(--c-muyalta);}
  .dot.demo{background:var(--c-alta);}

  .card{background:var(--card);border-radius:var(--radius);box-shadow:var(--shadow);padding:20px;margin-bottom:14px;}
  .eyebrow{font-size:11px;font-weight:700;letter-spacing:.09em;text-transform:uppercase;color:var(--muted);margin:0 0 10px;}

  /* ---------- 1. Lectura actual (borde de color = firma) ---------- */
  .lectura{
    border:5px solid var(--cat,var(--line));
    box-shadow:var(--shadow), 0 0 0 1px rgba(22,32,42,.02), 0 10px 30px -8px var(--catGlow,transparent);
    transition:border-color .4s ease, box-shadow .4s ease;
  }
  .valorWrap{display:flex;align-items:flex-end;gap:14px;flex-wrap:wrap;}
  .valor{font-size:84px;line-height:.92;font-weight:800;letter-spacing:-.03em;font-variant-numeric:tabular-nums;}
  .valorMeta{padding-bottom:8px;}
  .chip{
    display:inline-block;font-size:13px;font-weight:800;letter-spacing:.01em;
    padding:6px 13px;border-radius:999px;
    background:var(--cat,var(--line));color:var(--catOn,var(--ink));
  }
  .indiceLabel{display:block;font-size:12px;color:var(--muted);margin-top:8px;}
  .hora{margin:16px 0 0;font-size:14px;color:var(--muted);}
  .hora b{color:var(--ink);font-weight:700;font-variant-numeric:tabular-nums;}

  /* ---------- 2. Recomendación ---------- */
  .recoRow{display:flex;gap:14px;align-items:flex-start;}
  .recoIcon{flex:none;margin-top:2px;}
  .recoText{font-size:16px;}
  .recoText strong{display:block;font-size:13px;color:var(--muted);font-weight:700;margin-bottom:3px;letter-spacing:.01em;}

  /* ---------- 3. Resumen del día ---------- */
  .chartBox{width:100%;}
  .chartBox svg{width:100%;height:auto;display:block;}
  .chartEmpty{display:flex;align-items:center;justify-content:center;height:150px;color:var(--muted);font-size:14px;text-align:center;padding:0 20px;}
  .stats{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;margin-top:16px;}
  .stat{background:var(--paper);border-radius:12px;padding:12px;}
  .stat .k{font-size:11px;color:var(--muted);font-weight:700;letter-spacing:.04em;text-transform:uppercase;display:flex;align-items:center;gap:5px;}
  .stat .sw{width:9px;height:9px;border-radius:50%;flex:none;}
  .stat .v{font-size:26px;font-weight:800;font-variant-numeric:tabular-nums;line-height:1.1;margin-top:4px;}
  .stat .t{font-size:12px;color:var(--muted);font-variant-numeric:tabular-nums;}
  .nota{font-size:12px;color:var(--muted);margin:14px 2px 0;}

  /* ---------- Controles ---------- */
  .controls{display:flex;gap:10px;margin-top:6px;}
  button{
    flex:1;font:inherit;font-size:14px;font-weight:700;cursor:pointer;
    padding:13px 14px;border-radius:12px;border:1px solid var(--line);
    background:var(--card);color:var(--ink);transition:background .15s,border-color .15s;
  }
  button:hover{border-color:#C9D2DA;}
  button.primary{background:var(--ink);color:#fff;border-color:var(--ink);}
  button.primary:hover{background:#0c141c;}
  button:focus-visible{outline:3px solid #9CC6FF;outline-offset:2px;}

  @media (prefers-reduced-motion:reduce){*{transition:none!important;}}
</style>
</head>
<body>
<div class="wrap">

  <header>
    <h1>Monitor UV</h1>
    <span class="estado"><span class="dot" id="dotEstado"></span><span id="txtEstado">Sin conexión</span></span>
  </header>

  <!-- 1 ─ LECTURA ACTUAL -->
  <section class="card lectura" id="cardLectura" aria-live="polite">
    <p class="eyebrow">Lectura actual</p>
    <div class="valorWrap">
      <div class="valor" id="valor">--</div>
      <div class="valorMeta">
        <span class="chip" id="chip">Sin datos</span>
        <span class="indiceLabel">Índice UV</span>
      </div>
    </div>
    <p class="hora">Tomada a las <b id="hora">--:--</b></p>
  </section>

  <!-- 2 ─ RECOMENDACIÓN -->
  <section class="card">
    <p class="eyebrow">Recomendación</p>
    <div class="recoRow">
      <span class="recoIcon" id="recoIcon" aria-hidden="true"></span>
      <p class="recoText"><strong id="recoTitulo">Cuídate del sol</strong><span id="recoTexto">Esperando la primera lectura del sensor…</span></p>
    </div>
  </section>

  <!-- 3 ─ RESUMEN DEL DÍA -->
  <section class="card">
    <p class="eyebrow">Resumen del día</p>
    <div class="chartBox" id="chartBox">
      <div class="chartEmpty" id="chartEmpty">Aún no hay lecturas de hoy. El gráfico se completa a lo largo del día.</div>
    </div>
    <div class="stats">
      <div class="stat">
        <div class="k"><span class="sw" id="swMax" style="background:var(--line)"></span>Máximo</div>
        <div class="v" id="vMax">--</div>
        <div class="t" id="tMax">--:--</div>
      </div>
      <div class="stat">
        <div class="k"><span class="sw" id="swMin" style="background:var(--line)"></span>Mínimo</div>
        <div class="v" id="vMin">--</div>
        <div class="t" id="tMin">--:--</div>
      </div>
      <div class="stat">
        <div class="k">Promedio</div>
        <div class="v" id="vProm">--</div>
        <div class="t" id="tProm">0 lecturas</div>
      </div>
    </div>
    <p class="nota" id="nota"></p>
  </section>

  <div class="controls">
    <button class="primary" id="btnConectar">Conectar al sensor</button>
    <button id="btnDemo">Simular día (demo)</button>
  </div>

</div>

<script>
/* ================================================================
   CONFIGURACIÓN
   ----------------------------------------------------------------
   ENDPOINT: ruta del ESP32 que devuelve el índice UV actual.
   Como la página la sirve el propio ESP32 (192.168.4.1), va relativa.
   Debe responder algo como:  6.3    o    {"uv":6.3}
   INTERVALO_MIN: cada cuántos minutos se toma y guarda una lectura.
   ================================================================ */
const ENDPOINT     = "/uv";
const INTERVALO_MIN = 5;

/* ---------- Categorías IUV (mismos rangos y colores de la tabla) ----------
   0–2 Baja · 3–5 Moderada · 6–7 Alta · 8–10 Muy alta · 11+ Extrema      */
const CATS = [
  { nombre:"Baja",                color:"#5BB94A", on:"#11340B", rango:[0,2],
    titulo:"Riesgo bajo",
    reco:"Puedes estar al aire libre con seguridad. Si hay nieve o reflejo del agua, usa lentes de sol." },
  { nombre:"Moderada",            color:"#F2DD00", on:"#4A4400", rango:[3,5],
    titulo:"Protégete un poco",
    reco:"Usa bloqueador SPF 30+, sombrero y lentes de sol. Busca sombra cerca del mediodía." },
  { nombre:"Alta",                color:"#F2A100", on:"#472F00", rango:[6,7],
    titulo:"Cuídate del sol",
    reco:"Bloqueador SPF 30+, sombrero, lentes y ropa que cubra. Evita el sol directo entre las 11 y las 16 h." },
  { nombre:"Muy alta",            color:"#E5202E", on:"#FFFFFF", rango:[8,10],
    titulo:"Riesgo alto",
    reco:"Reduce la exposición entre las 11 y las 16 h. Bloqueador SPF 50+, ropa, sombrero y mantente a la sombra." },
  { nombre:"Extremadamente alta", color:"#BE5C97", on:"#FFFFFF", rango:[11,99],
    titulo:"Peligro: evita el sol",
    reco:"Sin protección la piel se quema en minutos. Evita el sol del mediodía y permanece a la sombra." },
];

/* Devuelve la categoría según el valor UV (decimales mapeados a la tabla) */
function categoria(uv){
  if (uv < 2.5)  return CATS[0];
  if (uv < 5.5)  return CATS[1];
  if (uv < 7.5)  return CATS[2];
  if (uv < 10.5) return CATS[3];
  return CATS[4];
}

/* ---------- Estado ---------- */
let lecturas = [];          // del día: [{uv, t (epoch ms)}]
let timer = null;
let modo = "idle";          // idle | live | demo
const hoy = new Date().toLocaleDateString("es-CL");
const KEY = "uv_dia_" + hoy;

/* ---------- Persistencia local (sobrevive si recargas la página) ---------- */
function guardar(){ try{ localStorage.setItem(KEY, JSON.stringify(lecturas)); }catch(e){} }
function cargar(){
  try{
    const raw = localStorage.getItem(KEY);
    if (raw) lecturas = JSON.parse(raw);
    // limpia días anteriores
    for (let i=0;i<localStorage.length;i++){
      const k = localStorage.key(i);
      if (k && k.startsWith("uv_dia_") && k !== KEY) localStorage.removeItem(k);
    }
  }catch(e){ lecturas = []; }
}

/* ---------- Utilidades ---------- */
const fmtHora = ms => new Date(ms).toLocaleTimeString("es-CL",{hour:"2-digit",minute:"2-digit"});
function setEstado(clase, txt){
  document.getElementById("dotEstado").className = "dot " + clase;
  document.getElementById("txtEstado").textContent = txt;
}
function iconoSol(color){
  return `<svg width="26" height="26" viewBox="0 0 24 24" fill="none" stroke="${color}" stroke-width="2" stroke-linecap="round">
    <circle cx="12" cy="12" r="4.2" fill="${color}" stroke="none"/>
    <g><line x1="12" y1="2" x2="12" y2="4.5"/><line x1="12" y1="19.5" x2="12" y2="22"/>
    <line x1="2" y1="12" x2="4.5" y2="12"/><line x1="19.5" y1="12" x2="22" y2="12"/>
    <line x1="4.9" y1="4.9" x2="6.7" y2="6.7"/><line x1="17.3" y1="17.3" x2="19.1" y2="19.1"/>
    <line x1="4.9" y1="19.1" x2="6.7" y2="17.3"/><line x1="17.3" y1="6.7" x2="19.1" y2="4.9"/></g></svg>`;
}

/* ---------- Agrega una lectura y refresca todo ---------- */
function registrar(uv, t){
  uv = Math.max(0, Number(uv));
  t  = t || Date.now();
  lecturas.push({uv, t});
  guardar();
  pintarActual(uv, t);
  pintarResumen();
}

/* ---------- 1 + 2: lectura actual y recomendación ---------- */
function pintarActual(uv, t){
  const c = categoria(uv);
  const card = document.getElementById("cardLectura");
  card.style.setProperty("--cat", c.color);
  card.style.setProperty("--catOn", c.on);
  card.style.setProperty("--catGlow", c.color + "55");

  document.getElementById("valor").textContent = uv.toFixed(1);
  const chip = document.getElementById("chip");
  chip.textContent = c.nombre;
  chip.style.setProperty("--cat", c.color);
  chip.style.setProperty("--catOn", c.on);
  document.getElementById("hora").textContent = fmtHora(t);

  document.getElementById("recoIcon").innerHTML = iconoSol(c.color);
  document.getElementById("recoTitulo").textContent = c.titulo;
  document.getElementById("recoTexto").textContent = c.reco;
}

/* ---------- 3: gráfico del día + máx / mín / promedio ---------- */
function pintarResumen(){
  const box = document.getElementById("chartBox");
  if (!lecturas.length){
    box.innerHTML = '<div class="chartEmpty">Aún no hay lecturas de hoy. El gráfico se completa a lo largo del día.</div>';
    return;
  }
  // estadísticas
  let max = lecturas[0], min = lecturas[0], suma = 0;
  for (const l of lecturas){
    if (l.uv > max.uv) max = l;
    if (l.uv < min.uv) min = l;
    suma += l.uv;
  }
  const prom = suma / lecturas.length;
  const cMax = categoria(max.uv), cMin = categoria(min.uv);

  document.getElementById("vMax").textContent = max.uv.toFixed(1);
  document.getElementById("tMax").textContent = fmtHora(max.t);
  document.getElementById("swMax").style.background = cMax.color;
  document.getElementById("vMin").textContent = min.uv.toFixed(1);
  document.getElementById("tMin").textContent = fmtHora(min.t);
  document.getElementById("swMin").style.background = cMin.color;
  document.getElementById("vProm").textContent = prom.toFixed(1);
  document.getElementById("tProm").textContent = lecturas.length + (lecturas.length===1?" lectura":" lecturas");

  document.getElementById("nota").textContent =
    "El peak del día fue " + max.uv.toFixed(1) + " (" + categoria(max.uv).nombre.toLowerCase() + ") a las " + fmtHora(max.t) + ".";

  box.innerHTML = dibujarGrafico(lecturas, max, min);
}

/* SVG hecho a mano (sin librerías, funciona offline en el ESP32).
   Bandas de fondo = colores IUV, para leer el riesgo de un vistazo. */
function dibujarGrafico(data, max, min){
  const W=520, H=210, pad={t:14,r:14,b:26,l:30};
  const x0=pad.l, x1=W-pad.r, y0=H-pad.b, y1=pad.t;
  const yMax = Math.max(12, Math.ceil(max.uv+1));
  // eje X: 6:00 a 20:00 (horas de sol)
  const HMIN=6*60, HMAX=20*60;
  const minutos = ms => { const d=new Date(ms); return d.getHours()*60 + d.getMinutes(); };
  const sx = m => x0 + (Math.min(Math.max(m,HMIN),HMAX)-HMIN)/(HMAX-HMIN)*(x1-x0);
  const sy = v => y0 - (Math.min(v,yMax)/yMax)*(y0-y1);

  // bandas de categoría
  const bandas=[[0,2,"#5BB94A"],[3,5,"#F2DD00"],[6,7,"#F2A100"],[8,10,"#E5202E"],[11,yMax,"#BE5C97"]];
  let bg="";
  for (const [a,b,col] of bandas){
    const yA=sy(Math.min(b+ .999,yMax)), yB=sy(a);
    bg+=`<rect x="${x0}" y="${yA}" width="${x1-x0}" height="${yB-yA}" fill="${col}" opacity="0.12"/>`;
  }
  // líneas de hora
  let grid="";
  for (const h of [8,12,16,20]){
    const x=sx(h*60);
    grid+=`<line x1="${x}" y1="${y1}" x2="${x}" y2="${y0}" stroke="#E3E8ED"/>`;
    grid+=`<text x="${x}" y="${H-8}" text-anchor="middle" font-size="11" fill="#69757F">${h}h</text>`;
  }
  // marcas eje Y
  let yl="";
  for (const v of [0,4,8,12]){ if(v>yMax)continue; const y=sy(v);
    yl+=`<text x="${x0-7}" y="${y+4}" text-anchor="end" font-size="11" fill="#69757F">${v}</text>`;
  }
  // línea de datos (ordenada por tiempo)
  const pts=[...data].sort((a,b)=>a.t-b.t).map(l=>[sx(minutos(l.t)),sy(l.uv)]);
  const path=pts.map((p,i)=>(i?"L":"M")+p[0].toFixed(1)+" "+p[1].toFixed(1)).join(" ");
  const area=`M${pts[0][0].toFixed(1)} ${y0} `+pts.map(p=>"L"+p[0].toFixed(1)+" "+p[1].toFixed(1)).join(" ")+` L${pts[pts.length-1][0].toFixed(1)} ${y0} Z`;
  const dots=pts.map(p=>`<circle cx="${p[0].toFixed(1)}" cy="${p[1].toFixed(1)}" r="2" fill="#16202A"/>`).join("");
  // peak destacado
  const px=sx(minutos(max.t)), py=sy(max.uv), cMax=categoria(max.uv);
  const peak=`<circle cx="${px}" cy="${py}" r="5.5" fill="${cMax.color}" stroke="#fff" stroke-width="2"/>`;

  return `<svg viewBox="0 0 ${W} ${H}" role="img" aria-label="Gráfico de índice UV durante el día">
    ${bg}${grid}${yl}
    <path d="${area}" fill="#16202A" opacity="0.05"/>
    <path d="${path}" fill="none" stroke="#16202A" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>
    ${dots}${peak}
  </svg>`;
}

/* ================================================================
   MODO LIVE: pide el UV al ESP32 cada INTERVALO_MIN minutos
   ================================================================ */
async function leerSensor(){
  try{
    const r = await fetch(ENDPOINT, {cache:"no-store"});
    if (!r.ok) throw 0;
    const txt = (await r.text()).trim();
    let uv;
    try{ uv = JSON.parse(txt).uv; }catch(e){ uv = parseFloat(txt); }
    if (isNaN(uv)) throw 0;
    registrar(uv);
    setEstado("on", "Conectado · cada " + INTERVALO_MIN + " min");
  }catch(e){
    setEstado("off", "Sin respuesta del sensor");
  }
}
function iniciarLive(){
  detener();
  modo="live";
  setEstado("on","Conectando…");
  leerSensor();
  timer = setInterval(leerSensor, INTERVALO_MIN*60*1000);
}
function detener(){ if(timer){clearInterval(timer);timer=null;} }

/* ================================================================
   MODO DEMO: genera un día completo (para probar sin el ESP32
   y para mostrar el gráfico en la presentación)
   ================================================================ */
function simularDia(){
  detener();
  modo="demo";
  lecturas=[];
  const base = new Date(); base.setHours(0,0,0,0);
  const peakHora = 13.5;                 // sol más fuerte ~13:30
  const peakUV   = 8 + Math.random()*3;  // entre 8 y 11
  for (let h=6; h<=20; h+= INTERVALO_MIN/60){
    const campana = Math.exp(-Math.pow(h-peakHora,2)/(2*2.4*2.4));
    let uv = peakUV*campana + (Math.random()-.5)*0.6;
    uv = Math.max(0, uv);
    const t = base.getTime() + h*3600*1000;
    lecturas.push({uv:+uv.toFixed(1), t});
  }
  guardar();
  // la "lectura actual" mostrada = la más cercana a la hora real
  const ahora = new Date().getHours()*60 + new Date().getMinutes();
  let cerca = lecturas[0];
  for (const l of lecturas){
    const m = new Date(l.t).getHours()*60 + new Date(l.t).getMinutes();
    if (Math.abs(m-ahora) < Math.abs(new Date(cerca.t).getHours()*60+new Date(cerca.t).getMinutes()-ahora)) cerca=l;
  }
  pintarActual(cerca.uv, cerca.t);
  pintarResumen();
  setEstado("demo","Modo demo · día simulado");
}

/* ---------- Arranque ---------- */
cargar();
if (lecturas.length){
  const u = lecturas[lecturas.length-1];
  pintarActual(u.uv, u.t);
  pintarResumen();
}
document.getElementById("btnConectar").addEventListener("click", iniciarLive);
document.getElementById("btnDemo").addEventListener("click", simularDia);
</script>
</body>
</html>
)=====";
  return html;
}

void handleRoot() { server.send(200, "text/html", htmlPage()); }
void handleData() { server.send(200, "text/plain", String(uvIntensity, 2) + "|" + nivel); }

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(115200);

  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  analogReadResolution(12);          // ADC de 0 a 4095

  setColor(0, 0, 0);                 // LED apagado al inicio

  // Crea la red WiFi propia (Access Point)
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("Red creada: ");  Serial.println(AP_SSID);
  Serial.print("Abre en el navegador: http://");
  Serial.println(WiFi.softAPIP());   // deberia mostrar 192.168.4.1

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/uv", []() { server.send(200, "text/plain", String(uvIntensity, 1)); }); // <-- NUEVO: lo que pide la pagina
  server.begin();
}

// ------------------- LOOP -------------------
void loop() {
  server.handleClient();

  static unsigned long t = 0;
  if (millis() - t > 1000) {         // mide una vez por segundo
    t = millis();
    uvIntensity = leerUV();

    if (uvIntensity < 3.0) {         // nivel bajo
      nivel = "BAJO";     setColor(0, 1, 0);   // VERDE
    } else if (uvIntensity < 6.0) {  // nivel moderado
      nivel = "MODERADO"; setColor(1, 1, 0);   // AMARILLO (rojo + verde)
    } else {                         // nivel alto
      nivel = "ALTO";     setColor(1, 0, 0);   // ROJO
    }

    Serial.print("UV: "); Serial.print(uvIntensity);
    Serial.print(" mW/cm2  ->  "); Serial.println(nivel);
  }
}
