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

esp_err_t Librarie::handler(httpd_req_t *req)
{
  std::string html = R"rawliteral(
<div class="card doc">

<h1>📚 Lenguaje Stride (DSL)</h1>
<p class="doc-subtitle">
Referencia del DSL embebido que interpreta el dispositivo desde archivos
<code>.str</code> almacenados en la tarjeta SD.
</p>

<section>
<h2>1. Cómo funciona</h2>
<ul>
  <li>Cada archivo <code>.str</code> es un programa.</li>
  <li><b>Una sentencia por línea</b>. Las líneas vacías se ignoran.</li>
  <li>Las <b>palabras clave son insensibles a mayúsculas</b>
      (<code>LOOP</code> = <code>loop</code> = <code>Loop</code>).
      Los <b>identificadores conservan la caja</b>.</li>
  <li>Los <b>números</b> pueden ser decimales (<code>17</code>) o
      hexadecimales (<code>0xFA</code>, <code>0X1B</code>).</li>
  <li>Las <b>cadenas</b> van entre comillas dobles: <code>"hola"</code>.</li>
  <li>Los programas se ejecutan secuencialmente desde la primera línea.</li>
</ul>
</section>

<section>
<h2>2. Inicio rápido</h2>
<pre>device = led name = myled pin = 17

loop 3
  write = myled on
  wait 1
  write = myled off
  wait 1
dloop</pre>
<p>Declara un LED en el pin 17 y lo parpadea tres veces.</p>
</section>

<section>
<h2>3. Dispositivos</h2>

<h3>Declaración</h3>
<p>Sintaxis: <code>device = &lt;tipo&gt; name = &lt;identificador&gt; pin = &lt;número&gt;</code></p>
<pre>device = led    name = myled  pin = 17
device = buzzer name = horn   pin = 26
device = button name = btn    pin = 32</pre>

<p>Tipos soportados:</p>
<table>
  <tr><th>Tipo</th><th>Uso</th></tr>
  <tr><td><code>led</code></td>    <td>Salida digital. Compatible con <code>write</code> y lectura del estado.</td></tr>
  <tr><td><code>buzzer</code></td> <td>Salida digital. Compatible con <code>write</code>.</td></tr>
  <tr><td><code>button</code></td> <td>Entrada digital. Su valor en condiciones es 1 (pulsado) o 0.</td></tr>
</table>

<h3>Escritura de salida</h3>
<p>Sintaxis: <code>write = &lt;nombre&gt; on|off</code></p>
<pre>write = myled on
write = horn  off</pre>

<p class="note">Los botones no se escriben — su valor se lee implícitamente al evaluar condiciones (ver §6).</p>
</section>

<section>
<h2>4. Variables</h2>

<h3>Asignación con expresión</h3>
<p>Sintaxis: <code>&lt;identificador&gt; = &lt;expresión&gt;</code></p>
<pre>counter = 5
total   = counter + 3
mask    = 0xFF &amp; data
shifted = value &lt;&lt; 2
result  = (a + b) * 2</pre>

<h3>Asignación con flecha</h3>
<p>Solo para asignar un valor literal (número decimal o hex). Sintaxis: <code>&lt;valor&gt; -&gt; &lt;identificador&gt;</code></p>
<pre>0    -> counter
0x76 -> sensor_addr</pre>

<h3>Operadores soportados en expresiones</h3>
<table>
  <tr><th>Categoría</th><th>Operadores</th></tr>
  <tr><td>Aritméticos</td><td><code>+</code> <code>-</code> <code>*</code> <code>/</code> <code>%</code></td></tr>
  <tr><td>Desplazamiento</td><td><code>&lt;&lt;</code> <code>&gt;&gt;</code></td></tr>
  <tr><td>Bit a bit</td><td><code>&amp;</code> <code>|</code></td></tr>
  <tr><td>Agrupación</td><td><code>(</code> <code>)</code></td></tr>
</table>
<p class="note">La evaluación se realiza de izquierda a derecha; usa paréntesis para forzar precedencia.</p>
</section>

<section>
<h2>5. Tiempo</h2>
<p>Sintaxis: <code>wait &lt;segundos&gt;</code> (admite decimales).</p>
<pre>wait 1
wait 0.25</pre>
</section>

<section>
<h2>6. Control de flujo</h2>

<h3>Condicionales</h3>
<p>Sintaxis: <code>if &lt;var&gt; &lt;op&gt; &lt;valor&gt;</code> … <code>[ else … ]</code> <code>endif</code></p>
<pre>if counter &gt; 5
  write = myled off
else
  write = myled on
endif</pre>

<p>Operadores de comparación: <code>==</code> <code>!=</code> <code>&lt;</code> <code>&lt;=</code> <code>&gt;</code> <code>&gt;=</code></p>
<p>El lado izquierdo puede ser una variable, un LED (devuelve su estado, 0/1) o un botón (1 si está pulsado).</p>

<h3>Bucles</h3>
<p>Bloque <code>loop</code> … <code>dloop</code>. Admite tres formas:</p>
<pre>loop 5            # repite 5 veces
  ...
dloop

loop -1           # bucle infinito
  ...
dloop

loop counter &lt; 10 # bucle condicional
  counter = counter + 1
dloop</pre>
<p class="note">El intérprete inserta una pequeña pausa al final de cada iteración para no bloquear al sistema.</p>
</section>

<section>
<h2>7. Salida y log</h2>

<h3>Archivo de log</h3>
<p>Sintaxis: <code>file "&lt;nombre&gt;"</code>. Cambia el archivo donde se acumulan los mensajes de <code>print</code>. Si no existe, se crea en la raíz de la SD.</p>
<pre>file "sesion.log"</pre>

<h3>Impresión</h3>
<p>Sintaxis: <code>print &lt;args…&gt;</code>. Concatena cadenas y valores de variables, añade <i>timestamp</i> y escribe la línea al archivo de log activo.</p>
<pre>print "Lectura:" sensor "ºC"
print "Estado=" counter</pre>
</section>

<section>
<h2>8. I2C</h2>

<p>El bus se inicializa una sola vez y luego se accede a cada dispositivo por su dirección de 7 bits.</p>

<h3>Inicialización</h3>
<pre>i2c init</pre>

<h3>Escritura</h3>
<p>Sintaxis: <code>i2c write &lt;addr&gt; &lt;data&gt; [&lt;reg&gt;]</code></p>
<pre>i2c write 0x76 0xF4          # un byte
i2c write 0x76 0xF4 0x25     # registro + byte</pre>

<h3>Lectura big-endian</h3>
<p>Sintaxis: <code>i2c read &lt;addr&gt; [&lt;reg&gt;] &lt;bytes&gt; -&gt; &lt;var&gt;</code> (1–32 bytes; combina los 4 primeros).</p>
<pre>i2c read 0x76 0xFA 3 -&gt; temp_raw</pre>

<h3>Lectura little-endian</h3>
<p>Sintaxis: <code>i2c readle &lt;addr&gt; &lt;reg&gt; &lt;bytes&gt; -&gt; &lt;var&gt;</code> (1–4 bytes).</p>
<pre>i2c readle 0x76 0x88 2 -&gt; cal_T1</pre>

<p class="note">En lecturas de 2 bytes, si el resultado supera 32767 se convierte automáticamente a entero con signo.</p>
</section>

<section>
<h2>9. Utilidades</h2>

<h3>SIGN16</h3>
<p>Reinterpreta una variable como entero con signo de 16 bits (<code>&gt; 32767</code> → resta 65536).</p>
<pre>i2c read 0x76 0xFA 2 -&gt; raw
sign16 raw</pre>
</section>

<section>
<h2>10. Ejemplo completo</h2>
<pre>file "blink.log"

device = led    name = led1 pin = 17
device = button name = btn  pin = 32

count = 0

loop -1
  if btn == 1
    write = led1 on
    count = count + 1
    print "Pulsado nº" count
    wait 0.5
  else
    write = led1 off
  endif
  wait 0.05
dloop</pre>
</section>

<section>
<h2>11. Referencia rápida</h2>
<table>
  <tr><th>Categoría</th><th>Palabras clave</th></tr>
  <tr><td>Dispositivos</td>  <td><code>device</code> <code>led</code> <code>button</code> <code>buzzer</code> <code>name</code> <code>pin</code> <code>write</code></td></tr>
  <tr><td>Valores</td>       <td><code>on</code> <code>off</code></td></tr>
  <tr><td>Datos</td>         <td><code>=</code> <code>-&gt;</code> <code>print</code> <code>file</code></td></tr>
  <tr><td>Tiempo</td>        <td><code>wait</code></td></tr>
  <tr><td>Control</td>       <td><code>if</code> <code>else</code> <code>endif</code> <code>loop</code> <code>dloop</code></td></tr>
  <tr><td>Comparación</td>   <td><code>==</code> <code>!=</code> <code>&lt;</code> <code>&lt;=</code> <code>&gt;</code> <code>&gt;=</code></td></tr>
  <tr><td>Aritmética</td>    <td><code>+</code> <code>-</code> <code>*</code> <code>/</code> <code>%</code></td></tr>
  <tr><td>Bits</td>          <td><code>&amp;</code> <code>|</code> <code>&lt;&lt;</code> <code>&gt;&gt;</code></td></tr>
  <tr><td>I2C</td>           <td><code>i2c</code> <code>init</code> <code>write</code> <code>read</code> <code>readle</code></td></tr>
  <tr><td>Utilidades</td>    <td><code>sign16</code></td></tr>
</table>
</section>

</div>
)rawliteral";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html.c_str(), html.length());

  return ESP_OK;
}
