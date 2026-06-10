#include "root.hpp"

#include <string>

Root::Root()
{
    _root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = &Root::handler,
        .user_ctx = this};

    _uris = {&_root_uri};
}

const std::vector<httpd_uri_t *> &Root::uris() const
{
    return _uris;
}

esp_err_t Root::handler(httpd_req_t *req)
{
    const std::string html = R"rawliteral(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Stride</title>
<style>
:root {
  --sw: 220px;
  --sb: #0f172a;
  --sb-hover: #1e293b;
  --primary: #2563eb;
  --primary-h: #1d4ed8;
  --bg: #f8fafc;
  --card: #ffffff;
  --border: #e2e8f0;
  --text: #1e293b;
  --muted: #64748b;
  --r: 8px;
  --shadow: 0 1px 3px rgba(0,0,0,.07), 0 1px 2px rgba(0,0,0,.05);
}
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
  font-size: 14px; color: var(--text);
  background: var(--bg);
  display: flex; height: 100vh; overflow: hidden;
}
/* ── SIDEBAR ── */
.sidebar {
  width: var(--sw); background: var(--sb); color: #94a3b8;
  display: flex; flex-direction: column; flex-shrink: 0;
  padding: 1rem 0.75rem; gap: 2px; z-index: 100;
  transition: transform 0.25s ease;
}
.sb-brand {
  padding: 0.5rem 0.75rem 1.5rem;
  font-size: 1.05rem; font-weight: 700; color: #fff; letter-spacing: 0.02em;
}
.nav-item {
  display: flex; align-items: center; gap: 0.6rem;
  padding: 0.6rem 0.75rem; border-radius: 7px;
  cursor: pointer; font-size: 0.875rem;
  transition: background 0.15s, color 0.15s; user-select: none;
}
.nav-item:hover, .nav-item.active { background: var(--sb-hover); color: #fff; }
/* ── TOPBAR ── */
.topbar {
  display: none; align-items: center; gap: 0.75rem;
  background: var(--sb); color: #fff;
  padding: 0.75rem 1rem; flex-shrink: 0;
}
.menu-btn {
  background: none; border: none; color: #fff;
  font-size: 1.3rem; cursor: pointer; line-height: 1;
  padding: 0; display: flex;
}
/* ── OVERLAY ── */
.overlay {
  display: none; position: fixed; inset: 0;
  background: rgba(0,0,0,.45); z-index: 99;
}
.overlay.open { display: block; }
/* ── MAIN ── */
.main { flex: 1; display: flex; flex-direction: column; min-width: 0; overflow: hidden; }
.content { flex: 1; padding: 1.5rem; overflow-y: auto; -webkit-overflow-scrolling: touch; }
/* ── CARD ── */
.card {
  background: var(--card); border-radius: var(--r);
  padding: 1.5rem; box-shadow: var(--shadow); border: 1px solid var(--border);
}
/* ── BUTTONS ── */
.btn {
  display: inline-flex; align-items: center; gap: 0.35rem;
  padding: 0.45rem 0.9rem; border-radius: 6px;
  font-size: 0.82rem; font-weight: 500; cursor: pointer;
  border: 1px solid var(--border); background: var(--card); color: var(--text);
  transition: background 0.15s, border-color 0.15s; text-decoration: none;
}
.btn:hover { background: var(--bg); }
.btn-primary { background: var(--primary); color: #fff; border-color: var(--primary); }
.btn-primary:hover { background: var(--primary-h); border-color: var(--primary-h); }
.btn-icon {
  padding: 0.35rem 0.4rem; border-radius: 5px; font-size: 0.95rem;
  background: none; border: none; cursor: pointer;
  color: var(--muted); transition: color 0.15s, background 0.15s;
}
.btn-icon:hover { background: var(--bg); color: var(--text); }
/* ── TABLE ── */
.ftable { width: 100%; border-collapse: collapse; margin-top: 1rem; }
.ftable th {
  text-align: left; font-size: 0.72rem; font-weight: 600;
  color: var(--muted); text-transform: uppercase; letter-spacing: 0.05em;
  padding: 0.5rem 0.75rem; border-bottom: 1px solid var(--border);
}
.ftable td { padding: 0.7rem 0.75rem; border-bottom: 1px solid var(--border); vertical-align: middle; }
.ftable tr:last-child td { border-bottom: none; }
.ftable tbody tr:hover td { background: var(--bg); }
.ftable .actions { display: flex; gap: 0.2rem; justify-content: flex-end; }
.fname { display: flex; align-items: center; gap: 0.5rem; font-size: 0.875rem; }
/* ── EDITOR ── */
.editor-area {
  width: 100%; min-height: 380px;
  font-family: 'SF Mono', 'Fira Code', 'Consolas', monospace;
  font-size: 0.83rem; line-height: 1.65;
  padding: 0.9rem 1rem; border: 1px solid var(--border);
  border-radius: var(--r); resize: vertical; outline: none;
  color: var(--text); background: #f8fafc;
  transition: border-color 0.15s, background 0.15s;
}
.editor-area:focus { border-color: var(--primary); background: #fff; }
/* ── VIEWER ── */
.log-view {
  background: #0f172a; color: #7dd3fc;
  padding: 1rem; border-radius: var(--r);
  font-family: 'SF Mono', 'Fira Code', 'Consolas', monospace;
  font-size: 0.78rem; line-height: 1.7;
  overflow-x: auto; white-space: pre-wrap; word-break: break-all;
  max-height: 58vh; overflow-y: auto;
}
.pagination { display: flex; gap: 0.5rem; margin-top: 1rem; align-items: center; }
.page-info { font-size: 0.78rem; color: var(--muted); margin-left: auto; }
/* ── DOC ── */
.doc { max-width: 720px; }
.doc h2 { border-bottom: 1px solid var(--border); padding-bottom: 0.4rem; margin-top: 1.75rem; font-size: 1rem; }
.doc h3 { color: var(--muted); margin-top: 1.2rem; font-size: 0.78rem; text-transform: uppercase; letter-spacing: 0.05em; }
.doc pre { background: #0f172a; color: #e2e8f0; padding: 1rem; border-radius: var(--r); overflow-x: auto; font-size: 0.8rem; line-height: 1.6; margin-top: .5rem; }
.doc table { width: 100%; border-collapse: collapse; margin-top: 0.75rem; }
.doc td, .doc th { padding: 0.55rem 0.75rem; border-bottom: 1px solid var(--border); text-align: left; font-size: 0.85rem; }
.doc-sub { color: var(--muted); font-size: 0.9rem; margin-top: 0.4rem; }
/* ── FORM ── */
.form-group { margin-bottom: 1rem; }
.form-label { display: block; font-size: 0.82rem; font-weight: 500; margin-bottom: 0.4rem; color: var(--muted); }
.form-input {
  width: 100%; padding: 0.55rem 0.75rem;
  border: 1px solid var(--border); border-radius: 6px;
  font-size: 0.875rem; outline: none; background: var(--bg);
  transition: border-color 0.15s;
}
.form-input:focus { border-color: var(--primary); background: #fff; }
/* ── EMPTY ── */
.empty { text-align: center; padding: 3rem 1rem; color: var(--muted); }
.empty-icon { font-size: 2rem; margin-bottom: 0.75rem; }
/* ── RUN BANNER ── */
.run-banner {
  display: flex; align-items: center; gap: 0.55rem;
  background: #ecfdf5; color: #065f46;
  border: 1px solid #a7f3d0; border-radius: var(--r);
  padding: 0.55rem 0.85rem; margin-bottom: 1rem;
  font-size: 0.82rem;
}
.run-dot {
  width: 9px; height: 9px; border-radius: 50%;
  background: #16a34a; box-shadow: 0 0 0 0 rgba(22,163,74,0.55);
  animation: run-pulse 1.2s ease-out infinite;
  flex-shrink: 0;
}
@keyframes run-pulse {
  0%   { box-shadow: 0 0 0 0 rgba(22,163,74,0.55); }
  70%  { box-shadow: 0 0 0 8px rgba(22,163,74,0); }
  100% { box-shadow: 0 0 0 0 rgba(22,163,74,0); }
}
/* ── TOAST ── */
.toast {
  position: fixed; bottom: 1.25rem; right: 1.25rem;
  background: #1e293b; color: #fff;
  padding: 0.65rem 1.1rem; border-radius: 7px;
  font-size: 0.82rem; box-shadow: 0 4px 14px rgba(0,0,0,.18);
  opacity: 0; transform: translateY(6px);
  transition: opacity 0.2s, transform 0.2s; z-index: 300; pointer-events: none;
}
.toast.show { opacity: 1; transform: translateY(0); }
/* ── PAGE HEADER ── */
.page-hdr {
  display: flex; align-items: center;
  justify-content: space-between; margin-bottom: 1.25rem;
}
.page-hdr h2 { font-size: 1rem; font-weight: 600; }
.page-hdr-left { display: flex; align-items: center; gap: 0.75rem; }
/* ── RESPONSIVE ── */
@media (max-width: 767px) {
  body { flex-direction: column; }
  .topbar { display: flex; }
  .sidebar {
    position: fixed; top: 0; left: 0; bottom: 0;
    transform: translateX(-100%);
  }
  .sidebar.open { transform: translateX(0); }
  .content { padding: 1rem; }
}
</style>
</head>
<body>

<div class="topbar">
  <button class="menu-btn" onclick="toggleSidebar()" aria-label="Menu">&#9776;</button>
  <span style="font-weight:600;font-size:1rem;">Stride</span>
</div>

<div class="overlay" id="overlay" onclick="closeSidebar()"></div>

<div class="sidebar" id="sidebar">
  <div class="sb-brand">Stride</div>
  <div class="nav-item active" id="nav-browser" onclick="nav('browser')">
    <span>&#128193;</span> Browser
  </div>
  <div class="nav-item" id="nav-librarie" onclick="nav('librarie')">
    <span>&#128218;</span> Referencia
  </div>
  <div class="nav-item" id="nav-settings" onclick="nav('settings')">
    <span>&#9881;</span> Ajustes
  </div>
</div>

<div class="main">
  <div class="content" id="content">
    <div class="card">
      <p style="color:var(--muted)">Cargando...</p>
    </div>
  </div>
</div>

<div class="toast" id="toast"></div>

<script>
function toggleSidebar() {
  document.getElementById('sidebar').classList.toggle('open');
  document.getElementById('overlay').classList.toggle('open');
}
function closeSidebar() {
  document.getElementById('sidebar').classList.remove('open');
  document.getElementById('overlay').classList.remove('open');
}
function nav(page) {
  document.querySelectorAll('.nav-item').forEach(el => el.classList.remove('active'));
  var el = document.getElementById('nav-' + page);
  if (el) el.classList.add('active');
  loadPage('/' + page);
  closeSidebar();
}
function toast(msg, err) {
  var t = document.getElementById('toast');
  t.textContent = msg;
  t.style.background = err ? '#dc2626' : '#1e293b';
  t.classList.add('show');
  setTimeout(function(){ t.classList.remove('show'); }, 2500);
}
function loadPage(url) {
  fetch(url)
    .then(function(r){ if(!r.ok) throw new Error(); return r.text(); })
    .then(function(h){ document.getElementById('content').innerHTML = h; })
    .catch(function(){
      document.getElementById('content').innerHTML =
        "<div class='card'><p style='color:#dc2626'>Error cargando la p&aacute;gina</p></div>";
    });
}
function createFile() {
  var name = prompt('Nombre del archivo (ej: programa.str)');
  if (!name) return;
  fetch('/create?file=' + encodeURIComponent(name), {method:'POST'})
    .then(function(r){ if(!r.ok) throw new Error(); return r.text(); })
    .then(function(){ toast('Archivo creado'); loadPage('/browser'); })
    .catch(function(){ toast('Error creando archivo', true); });
}
function editFile(file) { loadPage('/editor?file=' + encodeURIComponent(file)); }
function viewFile(file) { loadPage('/view?file=' + encodeURIComponent(file)); }
function saveFile(file) {
  var content = document.getElementById('editor').value;
  fetch('/save?file=' + encodeURIComponent(file), {
    method: 'POST', body: content,
    headers: {'Content-Type': 'text/plain'}
  })
    .then(function(r){ if(!r.ok) throw new Error(); return r.text(); })
    .then(function(){ toast('Guardado'); })
    .catch(function(){ toast('Error al guardar', true); });
}
function deleteFile(file) {
  if (!confirm('Eliminar ' + file + '?')) return;
  fetch('/delete?file=' + encodeURIComponent(file), {method:'POST'})
    .then(function(r){ if(!r.ok) throw new Error(); return r.text(); })
    .then(function(){ toast('Eliminado'); loadPage('/browser'); })
    .catch(function(){ toast('Error eliminando', true); });
}
function runFile(file) {
  fetch('/run?file=' + encodeURIComponent(file), {method:'POST'})
    .then(function(r){ if(!r.ok) throw new Error(); return r.text(); })
    .then(function(){ toast('Ejecutando ' + file); refreshRunStatus(); })
    .catch(function(){ toast('Error al ejecutar', true); });
}
var runStatusInFlight = false;
function refreshRunStatus() {
  if (runStatusInFlight) return;
  var banner = document.getElementById('runStatus');
  if (!banner) return;
  runStatusInFlight = true;
  fetch('/running', {cache: 'no-store'})
    .then(function(r){ if(!r.ok) throw new Error(); return r.text(); })
    .then(function(name){
      var b = document.getElementById('runStatus');
      var label = document.getElementById('runName');
      if (!b || !label) return;
      if (name && name.length > 0) {
        label.textContent = name;
        b.style.display = 'flex';
      } else {
        b.style.display = 'none';
      }
    })
    .catch(function(){})
    .finally(function(){ runStatusInFlight = false; });
}
var runStatusTimer = null;
function startRunStatusPolling() {
  if (runStatusTimer) { clearInterval(runStatusTimer); runStatusTimer = null; }
  refreshRunStatus();
  runStatusTimer = setInterval(function(){
    if (!document.getElementById('runStatus')) {
      clearInterval(runStatusTimer); runStatusTimer = null; return;
    }
    refreshRunStatus();
  }, 6000);
}
window.onload = function(){ nav('browser'); };
</script>
</body>
</html>)rawliteral";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
    return ESP_OK;
}
