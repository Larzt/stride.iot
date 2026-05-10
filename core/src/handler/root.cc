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
    const std::string kENDL = "\n";

    std::string html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="es">
    <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Stride Dashboard</title>

    <style>
    :root {
        --sidebar: #0f172a;
        --sidebar-hover: #1e293b;
        --primary: #2563eb;
        --bg: #f1f5f9;
        --card: #ffffff;
    }

    body {
        margin: 0;
        font-family: system-ui;
        display: flex;
        height: 100vh;
    }

    /* SIDEBAR */
    .sidebar {
        width: 220px;
        background: var(--sidebar);
        color: white;
        display: flex;
        flex-direction: column;
        padding: 1rem;
    }

    .sidebar h2 {
        margin-bottom: 2rem;
    }

    .nav-item {
        padding: 12px;
        border-radius: 8px;
        cursor: pointer;
    }

    .nav-item:hover {
        background: var(--sidebar-hover);
    }

    /* CONTENT */
    .content {
        flex: 1;
        background: var(--bg);
        padding: 2rem;
        overflow-y: auto;
    }

    .card {
        background: var(--card);
        padding: 2rem;
        border-radius: 12px;
    }
    /* LIBRARIE */
    .doc {
        max-width: 850px;
        margin: auto;
    }

    .doc-subtitle {
        color: #64748b;
        margin-bottom: 2rem;
    }

    section {
        margin-bottom: 2.5rem;
    }

    h2 {
        margin-top: 2rem;
        padding-bottom: 6px;
        border-bottom: 1px solid #e2e8f0;
    }

    h3 {
        margin-top: 1.2rem;
        color: #334155;
    }

    pre {
        background: #0f172a;
        color: #e2e8f0;
        padding: 1rem;
        border-radius: 10px;
        overflow-x: auto;
        font-size: 0.9rem;
    }

    table {
        width: 100%;
        border-collapse: collapse;
        margin-top: 1rem;
    }

    td, th {
        padding: 10px;
        border-bottom: 1px solid #e2e8f0;
        text-align: left;
    }
    </style>
    </head>

    <body>

    <div class="sidebar">
        <h2>Stride</h2>
        <div class="nav-item" onclick="loadPage('/browser')">📁 Browser</div>
        <div class="nav-item" onclick="loadPage('/librarie')">📚 Librarie</div>
        <div class="nav-item" onclick="loadPage('/settings')">⚙ Settings</div>
    </div>

    <div class="content" id="content">
        <div class="card">
            <h1>Bienvenido</h1>
            <p>Selecciona una opción del menú.</p>
        </div>
    </div>

    <script>
    function loadPage(url) {
        fetch(url)
        .then(res => res.text())
        .then(html => {
            document.getElementById("content").innerHTML = html;
        });
    }
    window.onload = () => {
      loadPage('/browser');
    }

    function createFile() {
        fetch('/create')
            .then(() => loadPage('/browser'));
    }

    function editFile(file) {
        loadPage('/editor?file=' + encodeURIComponent(file));
    }

    function viewFile(file) {
        loadPage('/view?file=' + encodeURIComponent(file));
    }

    function loadPage(url) {
        fetch(url)
        .then(res => {
            if (!res.ok) throw new Error("Error");
            return res.text();
        })
        .then(html => {
            document.getElementById("content").innerHTML = html;
        })
        .catch(() => {
            document.getElementById("content").innerHTML =
                "<div class='card'><h2>Error cargando la página</h2></div>";
        });
    }
    function saveFile(file) {
        const content = document.getElementById("editor").value;

        fetch('/save?file=' + encodeURIComponent(file), {
            method: 'POST',
            body: content,
            headers: {
                "Content-Type": "text/plain"
            }
        })
        .then(res => {
            console.log("STATUS:", res.status);
            return res.text();
        })
        .then(text => {
            console.log("RESPONSE:", text);
            alert("Guardado OK");
        })
        .catch(err => {
            console.error(err);
            alert("Error al guardar");
        });
    }
    </script>

    </body>
    </html>
    )rawliteral";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());

    return ESP_OK;
}
