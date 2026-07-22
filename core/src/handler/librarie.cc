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

// Mirror of docs/dsl.md — keep both in sync when the language changes.
esp_err_t Librarie::handler(httpd_req_t *req)
{
  std::string html = R"rawliteral(
<div class="card doc">

<h1>📚 Lenguaje Stride (DSL)</h1>
<p class="doc-subtitle">
Referencia del lenguaje que interpreta el dispositivo desde archivos
<code>.str</code> almacenados en la tarjeta SD. Pensado para personas sin
experiencia en programación: se lee casi como inglés, una orden por línea, y
todos los bloques se cierran con <code>end</code>.
</p>

<section>
<h2>1. Cómo funciona</h2>
<ul>
  <li>Cada archivo <code>.str</code> es un programa.</li>
  <li><b>Una sentencia por línea</b>. Las líneas vacías se ignoran.</li>
  <li>Los <b>comentarios</b> empiezan con <code>#</code>.</li>
  <li>Las <b>palabras clave son insensibles a mayúsculas</b>
      (<code>REPEAT</code> = <code>repeat</code>). Los nombres que tú inventas
      sí distinguen mayúsculas.</li>
  <li>Los <b>números</b> pueden ser decimales (<code>17</code>) o
      hexadecimales (<code>0xFA</code>).</li>
  <li>Las <b>cadenas</b> van entre comillas dobles: <code>"hola"</code>.</li>
  <li>Todos los bloques (<code>if</code>, <code>repeat</code>,
      <code>when</code>, <code>every</code>) terminan con <code>end</code> y
      <b>pueden anidarse</b> (hasta 16 niveles).</li>
  <li>Si el programa tiene errores de escritura, <b>no se ejecuta</b>: el
      editor y el log muestran cada error con su línea.</li>
</ul>
</section>

<section>
<h2>2. Inicio rápido</h2>
<pre>led light on pin 17

repeat 3 times
  turn light on
  wait 1 s
  turn light off
  wait 1 s
end</pre>
<p>Declara un LED en el pin 17 y lo parpadea tres veces.</p>
</section>

<section>
<h2>3. Dispositivos</h2>

<h3>Declaración</h3>
<p>Sintaxis: <code>led|button|buzzer &lt;nombre&gt; on pin &lt;número&gt;</code></p>
<pre>led    light on pin 17
buzzer horn  on pin 26
button btn   on pin 32</pre>

<table>
  <tr><th>Tipo</th><th>Uso</th></tr>
  <tr><td><code>led</code></td>    <td>Salida digital. Se controla con <code>turn</code> / <code>toggle</code>.</td></tr>
  <tr><td><code>buzzer</code></td> <td>Salida digital. Se controla con <code>turn</code> / <code>toggle</code>.</td></tr>
  <tr><td><code>button</code></td> <td>Entrada digital. Se lee en condiciones (<code>btn is pressed</code>) y eventos (<code>when btn pressed</code>).</td></tr>
</table>

<h3>Pines del expansor (PCF8574)</h3>
<p>El módulo expansor añade 8 pines por I2C (dirección fija <code>0x27</code>).
Se declaran con <code>pin … on expander</code> y después <b>se usan exactamente
igual que un LED</b>:</p>
<pre>pin exLed on expander 0
turn exLed on
toggle exLed</pre>
<p class="note">El pin /INT del expansor es una salida física del chip cableada
a un GPIO del ESP32. Si la usas, declárala como un botón normal:
<code>button intExp on pin &lt;gpio&gt;</code>.</p>

<h3>Acciones</h3>
<pre>turn light on      # enciende
turn light off     # apaga
toggle light       # invierte el estado</pre>
</section>

<section>
<h2>4. Variables y expresiones</h2>

<h3>Asignación</h3>
<p>Dos formas equivalentes; usa la que te resulte más natural:</p>
<pre>set count to 0
count = count + 1</pre>
<p>Las variables son números enteros. <code>on</code> vale 1 y
<code>off</code> vale 0.</p>

<h3>Operadores</h3>
<p>Precedencia estándar (de menor a mayor): <code>or</code> →
<code>and</code> → <code>not</code> → comparaciones → <code>|</code> →
<code>&amp;</code> → <code>&lt;&lt; &gt;&gt;</code> → <code>+ -</code> →
<code>* / %</code> → <code>-</code> unario. Usa paréntesis cuando quieras
dejarlo explícito.</p>
<table>
  <tr><th>Categoría</th><th>Operadores</th></tr>
  <tr><td>Lógicos</td><td><code>and</code> <code>or</code> <code>not</code></td></tr>
  <tr><td>Comparación</td><td><code>==</code> <code>!=</code> <code>&lt;</code> <code>&lt;=</code> <code>&gt;</code> <code>&gt;=</code> <code>is</code> <code>is not</code></td></tr>
  <tr><td>Aritméticos</td><td><code>+</code> <code>-</code> <code>*</code> <code>/</code> <code>%</code></td></tr>
  <tr><td>Desplazamiento</td><td><code>&lt;&lt;</code> <code>&gt;&gt;</code></td></tr>
  <tr><td>Bit a bit</td><td><code>&amp;</code> <code>|</code></td></tr>
  <tr><td>Agrupación</td><td><code>(</code> <code>)</code></td></tr>
  <tr><td>Funciones</td><td><code>signed16(x)</code></td></tr>
</table>

<p><code>is</code> se lee como <code>==</code>: <code>if count is 5</code>.
Para dispositivos hay formas especiales que se leen solas:</p>
<pre>if btn is pressed          # el botón está pulsado
if btn is released         # el botón está suelto
if light is on             # el LED está encendido
if temp &gt; 30 and humid &lt; 50
if not (btn is pressed)</pre>

<p><code>signed16(x)</code> reinterpreta un valor como entero con signo de 16
bits (útil tras lecturas I2C de 2 bytes):</p>
<pre>i2c read 0x76 register 0xFA size 2 into raw
set temp to signed16(raw)</pre>
</section>

<section>
<h2>5. Tiempo</h2>
<p>Sintaxis: <code>wait &lt;cantidad&gt; &lt;unidad&gt;</code> con unidad
obligatoria: <code>ms</code>, <code>s</code> o <code>min</code>.</p>
<pre>wait 500 ms
wait 1.5 s
wait 2 min
wait delay ms      # la cantidad puede ser una variable</pre>
<p class="note">Los decimales (<code>1.5</code>) solo se permiten en tiempos.</p>
</section>

<section>
<h2>6. Decisiones: if</h2>
<pre>if count &gt; 5
  turn light off
else if count &gt; 2
  toggle light
else
  turn light on
end</pre>
<p>La condición puede ser cualquier expresión (ver §4). Los bloques pueden
anidarse libremente.</p>
</section>

<section>
<h2>7. Repeticiones: repeat</h2>
<pre>repeat 5 times          # un número fijo de veces (times es opcional)
  ...
end

repeat forever          # hasta que se pare el programa
  ...
end

repeat while count &lt; 10 # mientras se cumpla la condición
  set count to count + 1
end

repeat until btn is pressed   # hasta que se cumpla la condición
  ...
end</pre>
<p class="note">El intérprete inserta una pequeña pausa al final de cada
iteración para no bloquear al sistema.</p>
</section>

<section>
<h2>8. Eventos: when y every</h2>
<p>Además del flujo de arriba a abajo, el programa puede <b>reaccionar</b>.
Los bloques <code>when</code> y <code>every</code> se registran al leerse y
empiezan a funcionar cuando el programa llega al final:</p>
<pre>button btn on pin 32
led light on pin 17

when btn pressed         # cada vez que se pulse el botón
  toggle light
end

when btn released        # cada vez que se suelte
  print "soltado"
end

every 10 s               # cada 10 segundos
  print "sigo vivo"
end</pre>
<p>Mientras haya algún <code>when</code> o <code>every</code>, el programa
queda esperando eventos hasta que se pare (desde la web, con el botón físico o
con <code>stop</code>).</p>
<p class="note">No mezcles <code>repeat forever</code> con eventos: mientras un
<code>repeat</code> se ejecuta, los eventos no se atienden. Para tareas
periódicas usa <code>every</code>.</p>

<h3>Parar el programa: stop</h3>
<pre>when btn pressed
  show "adios"
  stop
end</pre>
</section>

<section>
<h2>9. Salida</h2>

<h3>print — escribir al log</h3>
<p>Concatena cadenas y valores, añade <i>timestamp</i> y escribe al archivo de
log activo (visible en /view y en la pantalla):</p>
<pre>print "Lectura:" temp "C"
print "count =" count + 1</pre>

<h3>log to — elegir el archivo de log</h3>
<pre>log to "sesion.log"</pre>
<p>Si no existe, se crea en la raíz de la SD. Por defecto se usa
<code>prints.log</code>.</p>

<h3>show — escribir en la pantalla</h3>
<p>Muestra el texto en la pantalla TFT mientras el programa se ejecuta (se
conservan las últimas 4 líneas):</p>
<pre>show "Temperatura:" temp</pre>
</section>

<section>
<h2>10. I2C</h2>
<p>El bus se inicializa solo la primera vez que se usa. Cada dispositivo se
identifica por su dirección de 7 bits.</p>

<h3>Escritura</h3>
<p>Sintaxis: <code>i2c write &lt;addr&gt; [register &lt;reg&gt;] value &lt;dato&gt;</code></p>
<pre>i2c write 0x76 value 0xF4                # un byte
i2c write 0x76 register 0xF4 value 0x25  # registro + byte
i2c write addr register reg value mode   # también con variables</pre>

<h3>Lectura</h3>
<p>Sintaxis: <code>i2c read &lt;addr&gt; [register &lt;reg&gt;] size &lt;bytes&gt;
into &lt;variable&gt; [little [endian]]</code></p>
<pre>i2c read 0x76 register 0xFA size 3 into temp_raw       # big-endian (1-32 bytes)
i2c read 0x76 register 0x88 size 2 into cal_T1 little  # little-endian (1-4 bytes)</pre>
<p class="note">Se combinan los 4 primeros bytes en un entero. Si el valor debe
interpretarse con signo, usa <code>signed16(...)</code> (ya no se aplica
automáticamente).</p>
</section>

<section>
<h2>11. Errores</h2>
<ul>
  <li><b>Errores de escritura</b> (sintaxis): el programa no se ejecuta. Al
      guardar desde el editor se muestran todos los errores con su línea;
      también quedan en el log.</li>
  <li><b>Errores durante la ejecución</b> (dispositivo no declarado, división
      entre cero…): la línea se salta, el programa continúa y el aviso queda en
      el log con su número de línea.</li>
</ul>
</section>

<section>
<h2>12. Ejemplo completo</h2>
<pre># contador de pulsaciones con limite
log to "contador.log"

led    light on pin 17
button btn   on pin 32

set count to 0

when btn pressed
  set count to count + 1
  turn light on
  show "Pulsaciones:" count
  print "pulsado n" count
  wait 200 ms
  turn light off

  if count &gt;= 10
    show "Limite alcanzado!"
    stop
  end
end

every 5 s
  print "esperando... van" count
end</pre>
</section>

<section>
<h2>13. Referencia rápida</h2>
<table>
  <tr><th>Categoría</th><th>Palabras clave</th></tr>
  <tr><td>Dispositivos</td>  <td><code>led</code> <code>button</code> <code>buzzer</code> <code>pin … on expander</code> <code>turn</code> <code>toggle</code></td></tr>
  <tr><td>Valores</td>       <td><code>on</code> <code>off</code> <code>pressed</code> <code>released</code></td></tr>
  <tr><td>Variables</td>     <td><code>set … to</code> <code>=</code></td></tr>
  <tr><td>Tiempo</td>        <td><code>wait</code> + <code>ms</code> <code>s</code> <code>min</code></td></tr>
  <tr><td>Decisiones</td>    <td><code>if</code> <code>else if</code> <code>else</code> <code>end</code></td></tr>
  <tr><td>Repetición</td>    <td><code>repeat … times</code> <code>repeat forever</code> <code>repeat while</code> <code>repeat until</code> <code>end</code></td></tr>
  <tr><td>Eventos</td>       <td><code>when … pressed/released</code> <code>every</code> <code>end</code> <code>stop</code></td></tr>
  <tr><td>Lógica</td>        <td><code>and</code> <code>or</code> <code>not</code> <code>is</code> <code>is not</code></td></tr>
  <tr><td>Comparación</td>   <td><code>==</code> <code>!=</code> <code>&lt;</code> <code>&lt;=</code> <code>&gt;</code> <code>&gt;=</code></td></tr>
  <tr><td>Aritmética</td>    <td><code>+</code> <code>-</code> <code>*</code> <code>/</code> <code>%</code> <code>&amp;</code> <code>|</code> <code>&lt;&lt;</code> <code>&gt;&gt;</code></td></tr>
  <tr><td>Salida</td>        <td><code>print</code> <code>show</code> <code>log to</code></td></tr>
  <tr><td>I2C</td>           <td><code>i2c write … value</code> <code>i2c read … size … into</code> <code>register</code> <code>little endian</code></td></tr>
  <tr><td>Utilidades</td>    <td><code>signed16(x)</code> <code>#</code> (comentarios)</td></tr>
</table>
</section>

<section>
<h2>14. Migración desde la sintaxis antigua</h2>
<table>
  <tr><th>Antes</th><th>Ahora</th></tr>
  <tr><td><code>device = led name = myled pin = 17</code></td><td><code>led myled on pin 17</code></td></tr>
  <tr><td><code>write = myled on</code></td><td><code>turn myled on</code></td></tr>
  <tr><td><code>0 -&gt; counter</code></td><td><code>set counter to 0</code> (o <code>counter = 0</code>)</td></tr>
  <tr><td><code>wait 1</code> / <code>wait 0.5</code></td><td><code>wait 1 s</code> / <code>wait 500 ms</code> (unidad obligatoria)</td></tr>
  <tr><td><code>if btn == 1 … endif</code></td><td><code>if btn is pressed … end</code></td></tr>
  <tr><td><code>loop 3 … dloop</code></td><td><code>repeat 3 times … end</code></td></tr>
  <tr><td><code>loop -1 … dloop</code></td><td><code>repeat forever … end</code> (o eventos <code>when</code>/<code>every</code>)</td></tr>
  <tr><td><code>loop counter &lt; 10 … dloop</code></td><td><code>repeat while counter &lt; 10 … end</code></td></tr>
  <tr><td><code>file "sesion.log"</code></td><td><code>log to "sesion.log"</code></td></tr>
  <tr><td><code>i2c init</code></td><td>(ya no hace falta: se inicializa solo)</td></tr>
  <tr><td><code>i2c write 0x76 0xF4 0x25</code></td><td><code>i2c write 0x76 register 0xF4 value 0x25</code></td></tr>
  <tr><td><code>i2c read 0x76 0xFA 3 -&gt; raw</code></td><td><code>i2c read 0x76 register 0xFA size 3 into raw</code></td></tr>
  <tr><td><code>i2c readle 0x76 0x88 2 -&gt; cal</code></td><td><code>i2c read 0x76 register 0x88 size 2 into cal little</code></td></tr>
  <tr><td><code>expin myLed = 0</code></td><td><code>pin myLed on expander 0</code></td></tr>
  <tr><td><code>i2c write pin=myLed HIGH</code></td><td><code>turn myLed on</code></td></tr>
  <tr><td><code>i2c read pin=myBut -&gt; estado</code></td><td><code>estado = myBut</code> (se lee como un botón)</td></tr>
  <tr><td><code>sign16 raw</code></td><td><code>raw = signed16(raw)</code></td></tr>
</table>
</section>

</div>
)rawliteral";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html.c_str(), html.length());

  return ESP_OK;
}
