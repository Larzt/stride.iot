# 📚 Lenguaje Stride (DSL)

Referencia del DSL embebido que interpreta el dispositivo desde archivos `.str` almacenados en la tarjeta SD. El mismo contenido se sirve en runtime en el endpoint `/librarie` del propio dispositivo.

## 1. Cómo funciona

- Cada archivo `.str` es un programa.
- **Una sentencia por línea.** Las líneas vacías se ignoran.
- Las **palabras clave son insensibles a mayúsculas** (`LOOP` = `loop` = `Loop`).
- Los **números** pueden ser decimales (`17`) o hexadecimales (`0xFA`, `0X1B`).
- Las **cadenas** van entre comillas dobles: `"hola"`.
- Los programas se ejecutan secuencialmente desde la primera línea.

## 2. Inicio rápido

```text
device = led name = myled pin = 17

loop 3
  write = myled on
  wait 1
  write = myled off
  wait 1
dloop
```

Declara un LED en el pin 17 y lo parpadea tres veces.

## 3. Dispositivos

### Declaración

Sintaxis: `device = <tipo> name = <identificador> pin = <número>`

```text
device = led    name = myled  pin = 17
device = buzzer name = horn   pin = 26
device = button name = btn    pin = 32
```

Tipos soportados:

| Tipo     | Uso                                                                       |
|----------|---------------------------------------------------------------------------|
| `led`    | Salida digital. Compatible con `write` y lectura del estado.              |
| `buzzer` | Salida digital. Compatible con `write`.                                   |
| `button` | Entrada digital. Su valor en condiciones es `1` (pulsado) o `0`.          |

### Escritura de salida

Sintaxis: `write = <nombre> on|off`

```text
write = myled on
write = horn  off
```

> [!NOTE]
> Los botones no se escriben — su valor se lee implícitamente al evaluar condiciones (ver §6).

## 4. Variables

### Asignación con expresión

Sintaxis: `<identificador> = <expresión>`

```text
counter = 5
total   = counter + 3
mask    = 0xFF & data
shifted = value << 2
result  = (a + b) * 2
```

### Asignación con flecha

Solo para asignar un **valor literal** (número decimal o hex). Sintaxis: `<valor> -> <identificador>`

```text
0    -> counter
0x76 -> sensor_addr
```

### Operadores soportados en expresiones

| Categoría       | Operadores              |
|-----------------|-------------------------|
| Aritméticos     | `+` `-` `*` `/` `%`     |
| Desplazamiento  | `<<` `>>`               |
| Bit a bit       | `&` `|`                 |
| Agrupación      | `(` `)`                 |

> [!TIP]
> La evaluación es de izquierda a derecha; usa paréntesis para forzar precedencia.

## 5. Tiempo

Sintaxis: `wait <segundos>` (admite decimales).

```text
wait 1
wait 0.25
```

## 6. Control de flujo

### Condicionales

Sintaxis: `if <var> <op> <valor>` … `[ else … ]` `endif`

```text
if counter > 5
  write = myled off
else
  write = myled on
endif
```

Operadores de comparación: `==` `!=` `<` `<=` `>` `>=`

El lado izquierdo puede ser una **variable**, un **LED** (devuelve su estado, `0`/`1`) o un **botón** (`1` si está pulsado).

### Bucles

Bloque `loop` … `dloop`. Admite tres formas:

```text
loop 5            # repite 5 veces
  ...
dloop

loop -1           # bucle infinito
  ...
dloop

loop counter < 10 # bucle condicional
  counter = counter + 1
dloop
```

> [!NOTE]
> El intérprete inserta una pequeña pausa al final de cada iteración para no bloquear al sistema.

## 7. Salida y log

### Archivo de log

Sintaxis: `file "<nombre>"`. Cambia el archivo donde se acumulan los mensajes de `print`. Si no existe, se crea en la raíz de la SD.

```text
file "sesion.log"
```

### Impresión

Sintaxis: `print <args…>`. Concatena cadenas y valores de variables, añade *timestamp* y escribe la línea al archivo de log activo.

```text
print "Lectura:" sensor "ºC"
print "Estado=" counter
```

## 8. I2C

El bus se inicializa una sola vez y luego se accede a cada dispositivo por su dirección de 7 bits.

### Inicialización

```text
i2c init
```

### Escritura

Sintaxis: `i2c write <addr> <data> [<reg>]`

```text
i2c write 0x76 0xF4          # un byte
i2c write 0x76 0xF4 0x25     # registro + byte
```

### Lectura big-endian

Sintaxis: `i2c read <addr> [<reg>] <bytes> -> <var>` (1–32 bytes; combina los 4 primeros).

```text
i2c read 0x76 0xFA 3 -> temp_raw
```

### Lectura little-endian

Sintaxis: `i2c readle <addr> <reg> <bytes> -> <var>` (1–4 bytes).

```text
i2c readle 0x76 0x88 2 -> cal_T1
```

> [!NOTE]
> En lecturas de 2 bytes, si el resultado supera 32767 se convierte automáticamente a entero con signo.

## 9. Utilidades

### SIGN16

Reinterpreta una variable como entero con signo de 16 bits (> 32767 → resta 65536).

```text
i2c read 0x76 0xFA 2 -> raw
sign16 raw
```

## 10. Ejemplo completo

```text
file "blink.log"

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
dloop
```

## 11. Referencia rápida

| Categoría    | Palabras clave                                                         |
|--------------|------------------------------------------------------------------------|
| Dispositivos | `device` `led` `button` `buzzer` `name` `pin` `write`                  |
| Valores      | `on` `off`                                                             |
| Datos        | `=` `->` `print` `file`                                                |
| Tiempo       | `wait`                                                                 |
| Control      | `if` `else` `endif` `loop` `dloop`                                     |
| Comparación  | `==` `!=` `<` `<=` `>` `>=`                                            |
| Aritmética   | `+` `-` `*` `/` `%`                                                    |
| Bits         | `&` `|` `<<` `>>`                                                      |
| I2C          | `i2c` `init` `write` `read` `readle`                                   |
| Utilidades   | `sign16`                                                               |
