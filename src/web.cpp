#include "web.h"
#include "config.h"
#include "schedule.h"
#include "reminder.h"

#include <WiFi.h>
#include <WebServer.h>
#include <time.h>

static WebServer server(WEB_PORT);

// ===========================================================================
//  HTML — dashboard page
// ===========================================================================

static const char DASHBOARD_HTML[] PROGMEM = R"HTMLPAGE(<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0b1220">
<title>FITALIS — Pengingat Obat</title>
<style>
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0;padding:0;overscroll-behavior:none}
body{
  font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;
  background:#0b1220;color:#e6edf7;min-height:100dvh;
  padding:16px 14px calc(24px + env(safe-area-inset-bottom));
  -webkit-user-select:none;user-select:none;
}
h1{font-size:19px;margin:0;letter-spacing:.5px}
h1 span{color:#4ade80}
.sub{font-size:11px;color:#64748b;margin-top:2px}
.card{background:#131c2e;border:1px solid #1e2b45;border-radius:14px;padding:14px;margin-bottom:12px}
.card h2{font-size:13px;margin:0 0 10px;color:#94a3b8;font-weight:600;text-transform:uppercase;letter-spacing:.6px}
.clock{font-size:38px;font-weight:700;font-variant-numeric:tabular-nums;line-height:1.1}
.clock.small{font-size:20px}
.meta{font-size:11px;color:#64748b;margin-top:4px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.stat{background:#0f1729;border-radius:10px;padding:10px}
.stat .k{font-size:10px;color:#64748b;text-transform:uppercase;letter-spacing:.4px}
.stat .v{font-size:15px;font-weight:600;margin-top:3px}
.pill{display:inline-block;font-size:10px;padding:3px 8px;border-radius:99px;font-weight:600}
.pill.ok{background:#052e1a;color:#4ade80}
.pill.warn{background:#3b2a05;color:#fbbf24}
.pill.bad{background:#3b0d0d;color:#f87171}
.sched{background:#0f1729;border-radius:11px;padding:11px;margin-bottom:8px;border-left:3px solid #2563eb}
.sched.taken{border-left-color:#4ade80;opacity:.72}
.sched.off{border-left-color:#475569;opacity:.5}
.sched .row1{display:flex;justify-content:space-between;align-items:center;gap:8px}
.sched .nm{font-weight:600;font-size:14px}
.sched .tm{font-size:19px;font-weight:700;font-variant-numeric:tabular-nums}
.sched .dz{font-size:11px;color:#94a3b8;margin-top:3px}
.days{display:flex;gap:3px;margin-top:7px;flex-wrap:wrap}
.day{font-size:9px;padding:3px 6px;border-radius:5px;background:#1e2b45;color:#64748b}
.day.on{background:#1d4ed8;color:#fff}
button{font-family:inherit;font-size:13px;border:0;border-radius:10px;padding:11px 15px;min-height:44px;font-weight:600;cursor:pointer;color:#fff;background:#2563eb}
button:active{opacity:.75}
button.ghost{background:#1e2b45}
button.danger{background:#7f1d1d}
button.ok{background:#15803d}
button.tiny{padding:7px 11px;min-height:34px;font-size:11px;border-radius:8px}
input,select{font-family:inherit;font-size:15px;width:100%;padding:11px;border-radius:10px;border:1px solid #24314d;background:#0b1220;color:#e6edf7;margin-top:4px;min-height:44px}
label{font-size:11px;color:#94a3b8;display:block;margin-top:10px}
.frm{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.frm .full{grid-column:1/-1}
.daysel{display:flex;gap:5px;flex-wrap:wrap;margin-top:6px}
.daysel label{margin:0}
.daysel input{display:none}
.daysel span{display:inline-block;font-size:11px;padding:9px 11px;border-radius:8px;background:#1e2b45;color:#94a3b8;cursor:pointer;min-width:38px;text-align:center}
.daysel input:checked+span{background:#2563eb;color:#fff}
.hidden{display:none}
.alarm{background:#450a0a;border:1px solid #7f1d1d;border-radius:14px;padding:14px;margin-bottom:12px;text-align:center;animation:pulse 1s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.6}}
.alarm .t{font-size:11px;color:#fca5a5;text-transform:uppercase;letter-spacing:1px}
.alarm .n{font-size:22px;font-weight:700;margin:7px 0}
.btnrow{display:flex;gap:8px;flex-wrap:wrap;margin-top:10px}
.btnrow button{flex:1;min-width:96px}
.bar{display:flex;gap:6px;margin-bottom:12px}
.bar button{flex:1;background:#1e2b45;font-size:11px;padding:9px;min-height:40px}
.bar button.on{background:#2563eb}
#toast{position:fixed;left:50%;transform:translateX(-50%);bottom:24px;background:#1e2b45;padding:11px 18px;border-radius:10px;font-size:13px;opacity:0;transition:.25s;pointer-events:none;z-index:99;max-width:90vw;text-align:center}
#toast.show{opacity:1}
.empty{font-size:12px;color:#64748b;text-align:center;padding:18px}
</style>
</head>
<body>

<h1>FITALIS <span>&#9679;</span></h1>
<div class="sub" id="devid">memuat...</div>

<div id="alarmBox" class="alarm hidden">
  <div class="t">Waktunya minum obat</div>
  <div class="n" id="alarmName">-</div>
  <div class="sub" id="alarmDose">-</div>
  <div class="btnrow">
    <button class="ok" onclick="ack()">Sudah Minum</button>
    <button class="ghost" onclick="snooze()">Tunda 5 mnt</button>
  </div>
</div>

<div class="bar">
  <button id="tabLive" class="on" onclick="showTab('live')">Beranda</button>
  <button id="tabSched" onclick="showTab('sched')">Jadwal</button>
  <button id="tabInfo" onclick="showTab('info')">Info</button>
</div>

<!-- ============ TAB: LIVE ============ -->
<div id="tab-live">
  <div class="card">
    <h2>Waktu Sekarang</h2>
    <div class="clock" id="clock">--:--:--</div>
    <div class="meta" id="date">-</div>
    <div class="meta" id="syncstate">&nbsp;</div>
  </div>

  <div class="card">
    <h2>Ringkasan</h2>
    <div class="grid">
      <div class="stat"><div class="k">Jadwal Aktif</div><div class="v" id="statActive">0</div></div>
      <div class="stat"><div class="k">Sudah Diminum</div><div class="v" id="statTaken">0</div></div>
      <div class="stat"><div class="k">Jadwal Berikutnya</div><div class="v" id="statNext">-</div></div>
      <div class="stat"><div class="k">Status Alarm</div><div class="v" id="statAlarm">-</div></div>
    </div>
  </div>

  <div class="card">
    <h2>Jadwal Hari Ini</h2>
    <div id="todayList"><div class="empty">Belum ada jadwal</div></div>
  </div>
</div>

<!-- ============ TAB: SCHEDULE ============ -->
<div id="tab-sched" class="hidden">
  <div class="card">
    <h2>Daftar Jadwal</h2>
    <div id="schedList"><div class="empty">Belum ada jadwal</div></div>
  </div>

  <div class="card">
    <h2 id="formTitle">Tambah Jadwal</h2>
    <div class="frm">
      <div class="full">
        <label>Nama Obat</label>
        <input id="fName" placeholder="Contoh: Amlodipine" maxlength="31">
      </div>
      <div class="full">
        <label>Dosis</label>
        <input id="fDose" placeholder="Contoh: 1 tablet" maxlength="15">
      </div>
      <div>
        <label>Jam</label>
        <input id="fHour" type="number" min="0" max="23" value="8">
      </div>
      <div>
        <label>Menit</label>
        <input id="fMin" type="number" min="0" max="59" value="0">
      </div>
      <div class="full">
        <label>Hari</label>
        <div class="daysel" id="daySel"></div>
        <button type="button" class="ghost tiny" style="margin-top:8px" onclick="setDays('all')">Setiap Hari</button>
        <button type="button" class="ghost tiny" style="margin-top:8px" onclick="setDays('none')">Kosongkan</button>
      </div>
    </div>
    <div class="btnrow">
      <button id="btnSave" onclick="saveSched()">Simpan Jadwal</button>
      <button id="btnCancel" class="ghost hidden" onclick="resetForm()">Batal</button>
    </div>
    <input type="hidden" id="fIndex" value="-1">
  </div>
</div>

<!-- ============ TAB: INFO ============ -->
<div id="tab-info" class="hidden">
  <div class="card">
    <h2>Perangkat</h2>
    <div class="grid">
      <div class="stat"><div class="k">Device ID</div><div class="v" id="iDev">-</div></div>
      <div class="stat"><div class="k">Versi</div><div class="v" id="iVer">-</div></div>
      <div class="stat"><div class="k">IP</div><div class="v" id="iIp">-</div></div>
      <div class="stat"><div class="k">WiFi</div><div class="v" id="iRssi">-</div></div>
      <div class="stat"><div class="k">Uptime</div><div class="v" id="iUp">-</div></div>
      <div class="stat"><div class="k">RAM Bebas</div><div class="v" id="iHeap">-</div></div>
    </div>
  </div>

  <div class="card">
    <h2>Set Jam Manual</h2>
    <div class="meta" style="margin-bottom:8px">Kalau ESP32 tidak dapat internet (mode AP), set jamnya dari sini. Jam akan dipakai untuk memicu alarm.</div>
    <input id="iManualTime" type="datetime-local">
    <div class="btnrow">
      <button onclick="setTime()">Set Jam</button>
    </div>
  </div>

  <div class="card">
    <h2>Uji Coba</h2>
    <div class="btnrow">
      <button class="ghost" onclick="testAlarm()">Tes Alarm 5 Detik</button>
      <button class="ghost" onclick="resetTaken()">Reset Penanda Harian</button>
    </div>
  </div>
</div>

<div id="toast"></div>

<script>
const DAYS = ['Min','Sen','Sel','Rab','Kam','Jum','Sab'];
let editingIndex = -1;

function toast(m){const t=document.getElementById('toast');t.textContent=m;t.classList.add('show');setTimeout(()=>t.classList.remove('show'),2200);}

async function api(path, opts){
  const r = await fetch(path, opts);
  if(!r.ok) throw new Error('HTTP '+r.status);
  return r.json();
}

function showTab(t){
  ['live','sched','info'].forEach(x=>{
    document.getElementById('tab-'+x).classList.toggle('hidden', x!==t);
    document.getElementById('tab'+x.charAt(0).toUpperCase()+x.slice(1)).classList.toggle('on', x===t);
  });
  if(t==='sched') loadSchedules();
  if(t==='info') loadInfo();
}

// ---- clock + status polling ----
let lastState = 'idle';

async function refresh(){
  try{
    const st = await api('/api/status');
    document.getElementById('clock').textContent = st.time;
    document.getElementById('date').textContent = st.date;
    document.getElementById('syncstate').innerHTML = st.synced
      ? '<span class="pill ok">Jam sinkron (NTP)</span>'
      : '<span class="pill warn">Jam belum diset — buka tab Info</span>';
    document.getElementById('devid').textContent = 'Device: ' + (st.deviceId||'-');
    document.getElementById('statAlarm').textContent = st.state==='ringing' ? 'BERBUNYI' : 'Siaga';

    const box = document.getElementById('alarmBox');
    if(st.state === 'ringing'){
      box.classList.remove('hidden');
      document.getElementById('alarmName').textContent = st.activeName || '-';
      document.getElementById('alarmDose').textContent = st.activeDose || '';
      if(lastState !== 'ringing'){
        toast('Waktunya minum obat!');
        if(navigator.vibrate) navigator.vibrate([400,200,400,200,400]);
      }
    } else {
      box.classList.add('hidden');
    }
    lastState = st.state;
  }catch(e){ /* backend busy, try again next tick */ }
}

async function loadToday(){
  try{
    const d = await api('/api/schedules');
    renderSchedules(d, 'todayList', true);
    const active = d.schedules.filter(s=>s.enabled).length;
    const taken  = d.schedules.filter(s=>s.taken).length;
    document.getElementById('statActive').textContent = active;
    document.getElementById('statTaken').textContent = taken;

    const now = new Date();
    const nowMin = now.getHours()*60 + now.getMinutes();
    let next = null;
    d.schedules.filter(s=>s.enabled && !s.taken).forEach(s=>{
      const m = s.hour*60 + s.minute;
      if(m >= nowMin && (!next || m < next.hour*60+next.minute)) next = s;
    });
    document.getElementById('statNext').textContent = next
      ? String(next.hour).padStart(2,'0')+':'+String(next.minute).padStart(2,'0')
      : '-';
  }catch(e){}
}

function renderSchedules(d, targetId, compactOnly){
  const el = document.getElementById(targetId);
  if(!d.schedules.length){
    el.innerHTML = '<div class="empty">Belum ada jadwal. Buka tab Jadwal untuk menambah.</div>';
    return;
  }
  el.innerHTML = d.schedules.map(s=>{
    let cls = 'sched';
    if(s.taken) cls += ' taken';
    else if(!s.enabled) cls += ' off';

    const dayHtml = DAYS.map((dn,i)=>
      `<span class="day ${(s.days & (1<<i)) ? 'on':''}">${dn}</span>`).join('');

    const timeStr = String(s.hour).padStart(2,'0')+':'+String(s.minute).padStart(2,'0');

    if(compactOnly){
      return `<div class="${cls}">
        <div class="row1"><div class="nm">${esc(s.name)}</div><div class="tm">${timeStr}</div></div>
        <div class="dz">${esc(s.dose)} ${s.taken?'&middot; <span style="color:#4ade80">sudah diminum</span>':''}</div>
      </div>`;
    }

    return `<div class="${cls}">
      <div class="row1"><div class="nm">${esc(s.name)}</div><div class="tm">${timeStr}</div></div>
      <div class="dz">${esc(s.dose)} &middot; ${s.enabled?'aktif':'nonaktif'}${s.taken?' &middot; sudah diminum':''}</div>
      <div class="days">${dayHtml}</div>
      <div class="btnrow">
        <button class="ghost tiny" onclick="editSched(${s.index})">Ubah</button>
        <button class="ghost tiny" onclick="toggleSched(${s.index},${s.enabled?0:1})">${s.enabled?'Matikan':'Nyalakan'}</button>
        <button class="danger tiny" onclick="delSched(${s.index})">Hapus</button>
      </div>
    </div>`;
  }).join('');
}

function esc(s){return String(s==null?'':s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}

// ---- day selector ----
function buildDaySel(){
  document.getElementById('daySel').innerHTML = DAYS.map((dn,i)=>
    `<label><input type="checkbox" class="daycb" value="${i}"><span>${dn}</span></label>`).join('');
}
function setDays(mode){
  document.querySelectorAll('.daycb').forEach(cb=>cb.checked = (mode==='all'));
}
function getDays(){
  let m = 0;
  document.querySelectorAll('.daycb').forEach(cb=>{ if(cb.checked) m |= (1<<parseInt(cb.value)); });
  return m;
}
function applyDays(mask){
  document.querySelectorAll('.daycb').forEach(cb=>{ cb.checked = !!(mask & (1<<parseInt(cb.value))); });
}

// ---- schedule CRUD ----
async function loadSchedules(){
  try{
    const d = await api('/api/schedules');
    renderSchedules(d, 'schedList', false);
  }catch(e){ toast('Gagal memuat jadwal'); }
}

function editSched(i){
  api('/api/schedules').then(d=>{
    const s = d.schedules.find(x=>x.index===i);
    if(!s) return;
    document.getElementById('fName').value = s.name;
    document.getElementById('fDose').value = s.dose;
    document.getElementById('fHour').value = s.hour;
    document.getElementById('fMin').value  = s.minute;
    applyDays(s.days);
    document.getElementById('fIndex').value = i;
    editingIndex = i;
    document.getElementById('formTitle').textContent = 'Ubah Jadwal';
    document.getElementById('btnCancel').classList.remove('hidden');
    window.scrollTo({top:document.body.scrollHeight, behavior:'smooth'});
  });
}

function resetForm(){
  document.getElementById('fName').value = '';
  document.getElementById('fDose').value = '';
  document.getElementById('fHour').value = 8;
  document.getElementById('fMin').value = 0;
  applyDays(0x7F);
  document.getElementById('fIndex').value = -1;
  editingIndex = -1;
  document.getElementById('formTitle').textContent = 'Tambah Jadwal';
  document.getElementById('btnCancel').classList.add('hidden');
}

async function saveSched(){
  const name = document.getElementById('fName').value.trim();
  const dose = document.getElementById('fDose').value.trim();
  const hour = parseInt(document.getElementById('fHour').value);
  const min  = parseInt(document.getElementById('fMin').value);
  const days = getDays();
  const idx  = parseInt(document.getElementById('fIndex').value);

  if(!name){ toast('Nama obat wajib diisi'); return; }
  if(isNaN(hour)||hour<0||hour>23){ toast('Jam harus 0-23'); return; }
  if(isNaN(min)||min<0||min>59){ toast('Menit harus 0-59'); return; }
  if(days===0){ toast('Pilih minimal satu hari'); return; }

  const body = {name, dose: dose||'-', hour, minute: min, days};
  try{
    if(idx >= 0){
      await api('/api/schedules/'+idx, {method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
      toast('Jadwal diperbarui');
    } else {
      await api('/api/schedules', {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
      toast('Jadwal ditambahkan');
    }
    resetForm();
    loadSchedules();
    loadToday();
  }catch(e){ toast('Gagal menyimpan: '+e.message); }
}

async function toggleSched(i, en){
  const d = await api('/api/schedules');
  const s = d.schedules.find(x=>x.index===i);
  if(!s) return;
  await api('/api/schedules/'+i, {method:'PUT',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({name:s.name,dose:s.dose,hour:s.hour,minute:s.minute,days:s.days,enabled:!!en})});
  loadSchedules(); loadToday();
}

async function delSched(i){
  if(!confirm('Hapus jadwal ini?')) return;
  await api('/api/schedules/'+i, {method:'DELETE'});
  toast('Jadwal dihapus');
  loadSchedules(); loadToday();
}

// ---- alarm actions ----
async function ack(){ await api('/api/alarm/ack',{method:'POST'}); toast('Tercatat, terima kasih'); loadToday(); }
async function snooze(){ await api('/api/alarm/snooze',{method:'POST'}); toast('Ditunda 5 menit'); }
async function testAlarm(){
  await api('/api/test/alarm',{method:'POST'});
  toast('Alarm tes berbunyi 5 detik');
}
async function resetTaken(){ await api('/api/schedules/reset-taken',{method:'POST'}); toast('Penanda harian direset'); loadToday(); }

// ---- info ----
async function loadInfo(){
  try{
    const i = await api('/api/status');
    document.getElementById('iDev').textContent  = i.deviceId;
    document.getElementById('iVer').textContent  = i.version;
    document.getElementById('iIp').textContent   = i.ip;
    document.getElementById('iRssi').textContent = i.rssi ? i.rssi+' dBm' : 'AP mode';
    const up = i.uptime;
    document.getElementById('iUp').textContent   = Math.floor(up/3600)+'j '+Math.floor((up%3600)/60)+'m';
    document.getElementById('iHeap').textContent = Math.round(i.heap/1024)+' KB';
  }catch(e){}
}

async function setTime(){
  const v = document.getElementById('iManualTime').value;
  if(!v){ toast('Pilih tanggal dan jam dulu'); return; }
  try{
    await api('/api/time', {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({datetime:v})});
    toast('Jam diatur');
    refresh();
  }catch(e){ toast('Gagal set jam'); }
}

// ---- boot ----
buildDaySel();
applyDays(0x7F);
resetForm();
refresh();
loadToday();
setInterval(refresh, 2000);
setInterval(loadToday, 15000);
setInterval(()=>{ if(!document.getElementById('tab-sched').classList.contains('hidden')) loadSchedules(); }, 20000);
</script>
</body>
</html>)HTMLPAGE";

// ===========================================================================
//  Helpers
// ===========================================================================

static void sendJson(int code, const String& body) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

static void sendOk(const char* msg) {
  String b = "{\"ok\":true,\"message\":\"";
  b += msg;
  b += "\"}";
  sendJson(200, b);
}

static void sendErr(int code, const char* msg) {
  String b = "{\"ok\":false,\"error\":\"";
  b += msg;
  b += "\"}";
  sendJson(code, b);
}

// Pull a string field out of a flat JSON object body.
// Deliberately minimal: avoids a JSON library for 4 fields.
static bool jsonField(const String& body, const char* key, String& out) {
  String needle = String("\"") + key + "\"";
  int k = body.indexOf(needle);
  if (k < 0) return false;
  int colon = body.indexOf(':', k + needle.length());
  if (colon < 0) return false;

  int i = colon + 1;
  while (i < (int)body.length() && isspace((unsigned char)body[i])) i++;
  if (i >= (int)body.length()) return false;

  if (body[i] == '"') {
    i++;
    String val;
    while (i < (int)body.length() && body[i] != '"') {
      if (body[i] == '\\' && i + 1 < (int)body.length()) i++;
      val += body[i++];
    }
    out = val;
    return true;
  }

  // number / bool / null
  int start = i;
  while (i < (int)body.length() && body[i] != ',' && body[i] != '}') i++;
  out = body.substring(start, i);
  out.trim();
  return true;
}

static long jsonInt(const String& body, const char* key, long fallback) {
  String v;
  if (!jsonField(body, key, v) || v.length() == 0) return fallback;
  if (v == "true")  return 1;
  if (v == "false") return 0;
  return v.toInt();
}

// ===========================================================================
//  Route handlers
// ===========================================================================

static void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html; charset=utf-8", DASHBOARD_HTML);
}

static void handleApiStatus() {
  String out;
  out.reserve(400);
  out += "{\"deviceId\":\"";  out += FITALIS_DEVICE_ID; out += "\"";
  out += ",\"version\":\"";   out += FITALIS_VERSION;   out += "\"";
  out += ",\"ip\":\"";        out += webLocalIp();      out += "\"";

  int32_t rssi = (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
  out += ",\"rssi\":";  out += rssi;

  // Merge reminder status (it is already a JSON object).
  String rem = reminderStatusJson();
  if (rem.startsWith("{")) {
    out += ",";
    out += rem.substring(1, rem.length() - 1);   // strip outer braces
  }
  out += "}";
  sendJson(200, out);
}

static void handleGetSchedules() {
  sendJson(200, scheduleToJson());
}

static void handleAddSchedule() {
  String body = server.arg("plain");
  String name, dose;
  jsonField(body, "name", name);
  jsonField(body, "dose", dose);

  long hour = jsonInt(body, "hour", -1);
  long min  = jsonInt(body, "minute", -1);
  long days = jsonInt(body, "days", 0);

  if (name.length() == 0)          { sendErr(400, "name required"); return; }
  if (hour < 0 || hour > 23)       { sendErr(400, "hour out of range"); return; }
  if (min < 0 || min > 59)         { sendErr(400, "minute out of range"); return; }
  if (days <= 0)                   { sendErr(400, "days required"); return; }

  int idx = scheduleAdd(name.c_str(), dose.c_str(), (uint8_t)hour, (uint8_t)min, (uint8_t)days);
  if (idx < 0) { sendErr(409, "schedule table full"); return; }

  String out = "{\"ok\":true,\"index\":" + String(idx) + "}";
  sendJson(201, out);
}

// Extract a numeric index from paths like /api/schedules/2
// Written to work on every ESP32 core version (UriBraces is only in newer ones).
static int pathIndex() {
  String uri = server.uri();          // e.g. "/api/schedules/2"
  int slash = uri.lastIndexOf('/');
  if (slash < 0 || slash >= (int)uri.length() - 1) return -1;
  String tail = uri.substring(slash + 1);
  if (tail.length() == 0) return -1;
  for (size_t i = 0; i < tail.length(); i++) {
    if (!isdigit((unsigned char)tail[i])) return -1;
  }
  return tail.toInt();
}

static bool isSchedulePath() {
  return server.uri().startsWith("/api/schedules/");
}

static void handleUpdateSchedule() {
  int idx = pathIndex();
  String body = server.arg("plain");

  if (idx < 0 || idx >= scheduleCount()) { sendErr(404, "schedule not found"); return; }

  Schedule* cur = scheduleGet(idx);

  String name, dose;
  if (!jsonField(body, "name", name)) name = cur->name;
  if (!jsonField(body, "dose", dose)) dose = cur->dose;

  long hour = jsonInt(body, "hour",   cur->hour);
  long min  = jsonInt(body, "minute", cur->minute);
  long days = jsonInt(body, "days",   cur->days);
  long en   = jsonInt(body, "enabled", cur->enabled ? 1 : 0);

  if (hour < 0 || hour > 23 || min < 0 || min > 59 || days <= 0) {
    sendErr(400, "invalid time or days");
    return;
  }

  scheduleUpdate(idx, name.c_str(), dose.c_str(),
                 (uint8_t)hour, (uint8_t)min, (uint8_t)days, en != 0);
  sendOk("schedule updated");
}

static void handleDeleteSchedule() {
  int idx = pathIndex();
  if (!scheduleDelete(idx)) { sendErr(404, "schedule not found"); return; }
  sendOk("schedule deleted");
}

static void handleAlarmAck() {
  if (reminderAcknowledge()) sendOk("acknowledged");
  else                       sendErr(409, "no active alarm");
}

static void handleAlarmDismiss() {
  if (reminderDismiss()) sendOk("dismissed");
  else                   sendErr(409, "no active alarm");
}

static void handleAlarmSnooze() {
  if (reminderSnooze()) sendOk("snoozed");
  else                  sendErr(409, "no active alarm");
}

static void handleTestAlarm() {
  // Ring for 5 seconds regardless of schedule state, so the user can
  // verify wiring before relying on it.
  digitalWrite(PIN_BUZZER, BUZZER_ACTIVE_HIGH ? HIGH : LOW);
  delay(5000);
  digitalWrite(PIN_BUZZER, BUZZER_ACTIVE_HIGH ? LOW : HIGH);
  LOGLN("[web] test alarm fired");
  sendOk("test alarm fired");
}

static void handleResetTaken() {
  scheduleClearTakenFlags();
  sendOk("taken flags cleared");
}

static void handleSetTime() {
  String body = server.arg("plain");
  String dt;
  if (!jsonField(body, "datetime", dt)) { sendErr(400, "datetime required"); return; }
  if (!timeSetFromClient(dt))           { sendErr(400, "invalid datetime format"); return; }
  sendOk("time set");
}

static void handleNotFound() {
  sendErr(404, "not found");
}

// ===========================================================================
//  WiFi / NTP
// ===========================================================================

bool webIsConnected() {
#if WIFI_MODE == MODE_STATION
  return WiFi.status() == WL_CONNECTED;
#else
  return true;   // AP is always "up"
#endif
}

String webLocalIp() {
#if WIFI_MODE == MODE_STATION
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  return String("0.0.0.0");
#else
  return WiFi.softAPIP().toString();
#endif
}

static void startAccessPoint() {
  WiFi.mode(WIFI_AP);   // WIFI_AP comes from the core's WiFiType.h
  bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD[0] ? AP_PASSWORD : nullptr);
  LOGF("[wifi] AP %s -> %s\n", ok ? "started" : "FAILED",
       WiFi.softAPIP().toString().c_str());
}

static bool connectStation() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(STA_SSID, STA_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(400);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    LOGF("[wifi] connected, IP %s\n", WiFi.localIP().toString().c_str());
    return true;
  }
  LOGLN("[wifi] station connect failed");
  return false;
}

bool webConnectAndSyncTime() {
#if WIFI_MODE == MODE_AP
  startAccessPoint();
  // No internet in AP mode — the dashboard sets the clock instead.
  return false;
#else
  if (!connectStation()) {
    // Fall back to AP so the dashboard is still reachable.
    LOGLN("[wifi] falling back to AP mode");
    startAccessPoint();
    return false;
  }

  configTime(TZ_OFFSET_HOURS * 3600, 0, NTP_SERVER_1, NTP_SERVER_2);

  LOGLN("[ntp] waiting for time sync...");
  uint32_t start = millis();
  while (!timeIsSynced() && millis() - start < 15000) {
    delay(300);
  }

  if (timeIsSynced()) {
    LOGF("[ntp] synced: %s %s\n", dateNowString().c_str(), timeNowString().c_str());
    return true;
  }
  LOGLN("[ntp] sync timed out — set the clock from the dashboard");
  return false;
#endif
}

// ===========================================================================
//  Init / loop
// ===========================================================================

void webInit() {
  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/status",    HTTP_GET, handleApiStatus);
  server.on("/api/schedules", HTTP_GET, handleGetSchedules);
  server.on("/api/schedules", HTTP_POST, handleAddSchedule);

  // Indexed routes are matched manually so they work on any core version.
  server.on("/api/schedules/reset-taken", HTTP_POST, handleResetTaken);

  server.on("/api/alarm/ack",     HTTP_POST, handleAlarmAck);
  server.on("/api/alarm/dismiss", HTTP_POST, handleAlarmDismiss);
  server.on("/api/alarm/snooze",  HTTP_POST, handleAlarmSnooze);

  server.on("/api/test/alarm", HTTP_POST, handleTestAlarm);
  server.on("/api/time",       HTTP_POST, handleSetTime);

  server.onNotFound([]() {
    // Fall through: indexed schedule routes and everything else land here.
    if (isSchedulePath()) {
      if (server.method() == HTTP_PUT)    { handleUpdateSchedule(); return; }
      if (server.method() == HTTP_DELETE) { handleDeleteSchedule(); return; }
      sendErr(405, "method not allowed");
      return;
    }
    handleNotFound();
  });

  server.begin();
  LOGF("[web] dashboard on http://%s/\n", webLocalIp().c_str());
}

void webLoop() {
  server.handleClient();

#if WIFI_MODE == MODE_STATION
  // Reconnect in the background if the link drops. The reminder keeps
  // running from flash with no network at all.
  static uint32_t lastAttempt = 0;
  if (WiFi.status() != WL_CONNECTED && millis() - lastAttempt > 30000) {
    lastAttempt = millis();
    LOGLN("[wifi] reconnecting...");
    WiFi.disconnect();
    WiFi.begin(STA_SSID, STA_PASSWORD);
  }
#endif
}
