# 📚 Lenguaje Stride (DSL)

Referencia del lenguaje que interpreta el dispositivo desde archivos `.str` almacenados en la tarjeta SD. El mismo contenido se sirve en runtime en el endpoint `/librarie` del propio dispositivo (si cambias este documento, actualiza también `core/src/handler/librarie.cc`).

El lenguaje está pensado para personas **sin experiencia en programación**: se lee casi como inglés, una orden por línea, y todos los bloques se cierran con `end`.

## 1. Cómo funciona

- Cada archivo `.str` es un programa.
- **Una sentencia por línea.** Las líneas vacías se ignoran.
- Los **comentarios** empiezan con `#` y llegan hasta el final de la línea.
- Las **palabras clave son insensibles a mayúsculas** (`REPEAT` = `repeat`). Los nombres que tú inventas (variables y dispositivos) sí distinguen mayúsculas.
- Los **números** pueden ser decimales (`17`) o hexadecimales (`0xFA`).
- Las **cadenas** van entre comillas dobles: `"hola"`.
- Todos los bloques (`if`, `repeat`, `when`, `every`) terminan con **`end`** y **pueden anidarse** (hasta 16 niveles).
- Si el programa tiene errores de escritura, **no se ejecuta**: el editor web y el log muestran cada error con su número de línea.

## 2. Inicio rápido

```text
led light on pin 17

repeat 3 times
  turn light on
  wait 1 s
  turn light off
  wait 1 s
end
```

Declara un LED en el pin 17 y lo parpadea tres veces.

## 3. Dispositivos

### Declaración

Sintaxis: `led|button|buzzer <nombre> on pin <número>`

```text
led    light on pin 17
buzzer horn  on pin 26
button btn   on pin 32
```

| Tipo     | Uso                                                                |
|----------|--------------------------------------------------------------------|
| `led`    | Salida digital. Se controla con `turn` / `toggle`.                 |
| `buzzer` | Salida digital. Se controla con `turn` / `toggle`.                 |
| `button` | Entrada digital. Se lee en condiciones (`btn is pressed`) y eventos (`when btn pressed`). |

### Pines del expansor (PCF8574)

El módulo expansor añade 8 pines por I2C (dirección fija `0x27`). Se declaran con `pin … on expander` y después **se usan exactamente igual que un LED**:

```text
pin exLed on expander 0
turn exLed on
toggle exLed
```

> El pin `/INT` del expansor es una salida física del chip cableada a un GPIO del ESP32. Si la usas, declárala como un botón normal: `button intExp on pin <gpio>`.

### Acciones

```text
turn light on      # enciende
turn light off     # apaga
toggle light       # invierte el estado
```

## 4. Variables y expresiones

### Asignación

Dos formas equivalentes; usa la que te resulte más natural:

```text
set count to 0
count = count + 1
```

Las variables son números enteros. `on` vale `1` y `off` vale `0`.

### Operadores

Precedencia estándar (de menor a mayor): `or` → `and` → `not` → comparaciones → `|` → `&` → `<< >>` → `+ -` → `* / %` → `-` unario. Usa paréntesis cuando quieras dejarlo explícito.

| Categoría       | Operadores                                  |
|-----------------|---------------------------------------------|
| Lógicos         | `and` `or` `not`                            |
| Comparación     | `==` `!=` `<` `<=` `>` `>=` `is` `is not`   |
| Aritméticos     | `+` `-` `*` `/` `%`                         |
| Desplazamiento  | `<<` `>>`                                   |
| Bit a bit       | `&` `\|`                                    |
| Agrupación      | `(` `)`                                     |
| Funciones       | `signed16(x)`                               |

`is` se lee como `==`: `if count is 5`. Para dispositivos hay formas especiales que se leen solas:

```text
if btn is pressed          # el botón está pulsado
if btn is released         # el botón está suelto
if light is on             # el LED está encendido
if temp > 30 and humid < 50
if not (btn is pressed)
```

`signed16(x)` reinterpreta un valor como entero con signo de 16 bits (útil tras lecturas I2C de 2 bytes):

```text
i2c read 0x76 register 0xFA size 2 into raw
set temp to signed16(raw)
```

## 5. Tiempo

Sintaxis: `wait <cantidad> <unidad>` con unidad obligatoria: `ms`, `s` o `min`.

```text
wait 500 ms
wait 1.5 s
wait 2 min
wait delay ms      # la cantidad puede ser una variable
```

Los decimales (`1.5`) solo se permiten en tiempos.

## 6. Decisiones: `if`

```text
if count > 5
  turn light off
else if count > 2
  toggle light
else
  turn light on
end
```

La condición puede ser cualquier expresión (ver §4). Los bloques pueden anidarse libremente.

## 7. Repeticiones: `repeat`

```text
repeat 5 times          # un número fijo de veces (la palabra times es opcional)
  ...
end

repeat forever          # hasta que se pare el programa
  ...
end

repeat while count < 10 # mientras se cumpla la condición
  set count to count + 1
end

repeat until btn is pressed   # hasta que se cumpla la condición
  ...
end
```

> El intérprete inserta una pequeña pausa al final de cada iteración para no bloquear el sistema.

## 8. Eventos: `when` y `every`

Además del flujo de arriba a abajo, el programa puede **reaccionar**. Los bloques `when` y `every` se registran al leerse y empiezan a funcionar cuando el programa llega al final:

```text
button btn on pin 32
led light on pin 17

when btn pressed         # cada vez que se pulse el botón
  toggle light
end

when btn released        # cada vez que se suelte
  print "soltado"
end

every 10 s               # cada 10 segundos
  print "sigo vivo"
end
```

Mientras haya algún `when` o `every`, el programa queda esperando eventos hasta que se pare (desde la web, con el botón físico o con `stop`).

> No mezcles `repeat forever` con eventos: mientras un `repeat` se ejecuta, los eventos no se atienden. Para tareas periódicas usa `every`.

### Parar el programa: `stop`

```text
when btn pressed
  show "adios"
  stop
end
```

## 9. Salida

### `print` — escribir al log

Concatena cadenas y valores, añade *timestamp* y escribe al archivo de log activo (visible en `/view` y en la pantalla):

```text
print "Lectura:" temp "C"
print "count =" count + 1
```

### `log to` — elegir el archivo de log

```text
log to "sesion.log"
```

Si no existe, se crea en la raíz de la SD. Por defecto se usa `prints.log`.

### `show` — escribir en la pantalla

Muestra el texto en la pantalla TFT del dispositivo mientras el programa se ejecuta (se conservan las últimas 4 líneas):

```text
show "Temperatura:" temp
```

## 10. I2C

El bus se inicializa solo la primera vez que se usa. Cada dispositivo se identifica por su dirección de 7 bits.

### Escritura

Sintaxis: `i2c write <addr> [register <reg>] value <dato>`

```text
i2c write 0x76 value 0xF4                # un byte
i2c write 0x76 register 0xF4 value 0x25  # registro + byte
i2c write addr register reg value mode   # también con variables
```

### Lectura

Sintaxis: `i2c read <addr> [register <reg>] size <bytes> into <variable> [little [endian]]`

```text
i2c read 0x76 register 0xFA size 3 into temp_raw          # big-endian (1-32 bytes)
i2c read 0x76 register 0x88 size 2 into cal_T1 little     # little-endian (1-4 bytes)
```

Se combinan los 4 primeros bytes en un entero. Si el valor debe interpretarse con signo, usa `signed16(...)` (ya no se aplica automáticamente).

## 11. Errores

- **Errores de escritura** (sintaxis): el programa no se ejecuta. Al guardar desde el editor web se muestran todos los errores con su línea; también quedan en el log.
- **Errores durante la ejecución** (por ejemplo, usar un dispositivo no declarado o dividir entre cero): la línea se salta, el programa continúa y el aviso queda en el log con su número de línea.

## 12. Ejemplo completo

```text
# contador de pulsaciones con limite
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

  if count >= 10
    show "Limite alcanzado!"
    stop
  end
end

every 5 s
  print "esperando... van" count
end
```

## 13. Referencia rápida

| Categoría    | Palabras clave                                                          |
|--------------|--------------------------------------------------------------------------|
| Dispositivos | `led` `button` `buzzer` `pin … on expander` `turn` `toggle`             |
| Valores      | `on` `off` `pressed` `released`                                          |
| Variables    | `set … to` `=`                                                           |
| Tiempo       | `wait` + `ms` `s` `min`                                                  |
| Decisiones   | `if` `else if` `else` `end`                                              |
| Repetición   | `repeat … times` `repeat forever` `repeat while` `repeat until` `end`   |
| Eventos      | `when … pressed/released` `every` `end` `stop`                           |
| Lógica       | `and` `or` `not` `is` `is not`                                           |
| Comparación  | `==` `!=` `<` `<=` `>` `>=`                                              |
| Aritmética   | `+` `-` `*` `/` `%` `&` `\|` `<<` `>>`                                   |
| Salida       | `print` `show` `log to`                                                  |
| I2C          | `i2c write … value` `i2c read … size … into` `register` `little endian` |
| Utilidades   | `signed16(x)` `#` (comentarios)                                          |

## 14. Migración desde la sintaxis antigua

| Antes                                  | Ahora                                      |
|----------------------------------------|--------------------------------------------|
| `device = led name = myled pin = 17`   | `led myled on pin 17`                      |
| `write = myled on`                     | `turn myled on`                            |
| `0 -> counter`                         | `set counter to 0` (o `counter = 0`)       |
| `wait 1` / `wait 0.5`                  | `wait 1 s` / `wait 500 ms` (unidad obligatoria) |
| `if btn == 1 … endif`                  | `if btn is pressed … end`                  |
| `loop 3 … dloop`                       | `repeat 3 times … end`                     |
| `loop -1 … dloop`                      | `repeat forever … end` (o eventos `when`/`every`) |
| `loop counter < 10 … dloop`            | `repeat while counter < 10 … end`          |
| `file "sesion.log"`                    | `log to "sesion.log"`                      |
| `i2c init`                             | (ya no hace falta: se inicializa solo)     |
| `i2c write 0x76 0xF4 0x25`             | `i2c write 0x76 register 0xF4 value 0x25`  |
| `i2c read 0x76 0xFA 3 -> raw`          | `i2c read 0x76 register 0xFA size 3 into raw` |
| `i2c readle 0x76 0x88 2 -> cal`        | `i2c read 0x76 register 0x88 size 2 into cal little` |
| `expin myLed = 0`                      | `pin myLed on expander 0`                  |
| `i2c write pin=myLed HIGH`             | `turn myLed on`                            |
| `i2c read pin=myBut -> estado`         | `estado = myBut` (se lee como un botón)    |
| `sign16 raw`                           | `raw = signed16(raw)`                      |
| (sin comentarios)                      | `# comentario`                             |
| (sin anidamiento)                      | bloques anidados con `end`                 |
