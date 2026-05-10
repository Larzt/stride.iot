#include "librarie.hpp"
#include <string>


Librarie::Librarie()
{
  _librarie_uri = {
      .uri = "/librarie",
      .method = HTTP_GET,
      .handler = &Librarie::handler,
      .user_ctx = this};

  _uris = {&_librarie_uri};
}

const std::vector<httpd_uri_t *> &Librarie::uris() const
{
  return _uris;
}

esp_err_t Librarie::handler(httpd_req_t *req) {

std::string html = R"rawliteral(
<div class="card doc">

<h1>📚 Lenguaje Stride</h1>
<p class="doc-subtitle">
Guía práctica del lenguaje de control del sistema.
</p>

<section>
<h2>1. Inicio rápido</h2>
<p>Ejemplo mínimo:</p>

<pre>
OUTPUT=led NAME=myled pin=17

LOOP 3
write=myled on
wait 1
write=myled off
wait 1
DLOOP
</pre>
</section>

<section>
<h2>2. Dispositivos</h2>

<h3>Declaración</h3>
<pre>OUTPUT=led NAME=myled pin=17</pre>

<h3>Control</h3>
<pre>
write=myled on
write=myled off
</pre>

<p>Actualmente solo se soportan dispositivos tipo <b>led</b>.</p>
</section>

<section>
<h2>3. Variables</h2>

<h3>Asignación</h3>
<pre>
myVar=5
3 -> counter
</pre>

<p>Ambas formas son equivalentes.</p>
</section>

<section>
<h2>4. Control de flujo</h2>

<h3>Bucles</h3>
<pre>
LOOP 3
...
DLOOP
</pre>

<p>Bucle infinito:</p>
<pre>LOOP -1</pre>

<h3>Condicionales</h3>
<pre>
if myVar > 5
write=myled off
endif
</pre>
</section>

<section>
<h2>5. Tiempo</h2>
<pre>WAIT 1</pre>
<p>Pausa la ejecución en segundos.</p>
</section>

<section>
<h2>6. Salida</h2>

<h3>Archivo</h3>
<pre>FILE=myLog</pre>

<h3>Impresión</h3>
<pre>PRINT "Valor: " myVar</pre>
</section>

<section>
<h2>7. I2C</h2>
<pre>i2c read 0x76 0xFA 3 -> temp</pre>
</section>

<section>
<h2>8. Ejemplo</h2>

<pre>
OUTPUT=led NAME=myled pin=17
write=myled off

LOOP 3
write=myled on
wait 1
write=myled off
wait 1
DLOOP

myVar=5

if myVar > 3
write=myled on
endif

PRINT "Valor: " myVar
</pre>
</section>

<section>
<h2>9. Referencia rápida</h2>

<table>
<tr><th>Tipo</th><th>Comandos</th></tr>

<tr>
<td>Dispositivos</td>
<td>OUTPUT, WRITE</td>
</tr>

<tr>
<td>Control</td>
<td>IF, ENDIF, LOOP, DLOOP, WAIT</td>
</tr>

<tr>
<td>Datos</td>
<td>PRINT, FILE, =, -></td>
</tr>

<tr>
<td>I2C</td>
<td>I2C, READ</td>
</tr>

</table>

</section>

</div>
)rawliteral";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html.c_str(), html.length());

  return ESP_OK;
}
