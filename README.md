![tests](https://github.com/Larzt/tfg-stride/actions/workflows/tests.yml/badge.svg)
![ESP-IDF Build](https://github.com/Larzt/tfg-stride/actions/workflows/esp-idf-build.yml/badge.svg)

# 📡 STRIDE — Dispositivo IoT Modular para ESP32

STRIDE es el firmware base de un **dispositivo IoT modular** desarrollado como **Trabajo de Fin de Grado (TFG)** sobre **ESP-IDF** y **ESP32**. Su objetivo es ofrecer una alternativa **versátil y reconfigurable** a los dispositivos IoT comerciales: el núcleo central integra **WiFi (AP+STA), un servidor HTTP, pantalla TFT, lector de tarjeta SD y un bus I2C de expansión**, y delega la lógica de usuario a **scripts `.str`** escritos en un **DSL propio** que se interpretan en el dispositivo.

La pieza clave de la arquitectura es que añadir hardware, endpoints, vistas o comandos del DSL **no requiere tocar el núcleo**: cada extensión vive en un módulo aislado y se conecta al sistema a través de un puñado de patrones bien definidos.

---

## 🗂️ Estructura del repositorio

```
.
├── CMakeLists.txt           # Proyecto ESP-IDF (declara los EXTRA_COMPONENT_DIRS)
├── partitions.csv           # Tabla de particiones de la flash
├── sdkconfig*               # Configuración de ESP-IDF
└── core/
    ├── src/                 # Componente principal: lógica del dispositivo
    │   ├── main.cc          # app_main(): bootstrap y creación de tareas FreeRTOS
    │   ├── blackboard.hpp   # Estado global compartido (config + observables)
    │   ├── brain/           # Subsistemas centrales (network, server, display, …)
    │   ├── handler/         # Endpoints HTTP (uno por archivo)
    │   ├── tasks/           # Funciones de tarea FreeRTOS
    │   ├── types/           # Enums y tipos compartidos
    │   └── utils/           # Helpers (parse, timer, sink, hexer)
    ├── plugin/              # Componentes reutilizables (cada uno es un IDF component)
    │   ├── io/              # Drivers de periféricos: LED, botón, buzzer (StrideBase)
    │   ├── lexer/           # Lexer del DSL .str
    │   ├── logger/          # Logger por subsistema
    │   ├── observer/        # StrideObservable / StrideSubscription
    │   └── locator/         # Service locator tipado
    ├── external/            # Dependencias vendored (LovyanGFX)
    └── tests/               # Tests unitarios sobre PC
```

`core/src/CMakeLists.txt` registra `core/src` como **componente principal** y declara como dependencias (`REQUIRES`) los plugins (`logger`, `io`, `observer`, `lexer`, `locator`). El `CMakeLists.txt` raíz expone los tres directorios de componentes (`core/src`, `core/external`, `core/plugin`) vía `EXTRA_COMPONENT_DIRS`.

---

## 🧠 Arquitectura del código

El sistema se organiza en **capas**, cada una con una responsabilidad clara y con dependencias **solo hacia abajo**:

```
                  ┌──────────────────────────────────────────────┐
                  │  main.cc  (app_main, bootstrap, FreeRTOS)    │
                  └──────────────────────────────────────────────┘
                                       │
        ┌──────────────────────────────┼──────────────────────────────┐
        ▼                              ▼                              ▼
 ┌──────────────┐             ┌──────────────┐              ┌──────────────────┐
 │   tasks/     │             │   brain/     │              │   handler/       │
 │ FreeRTOS     │ ──────────► │ Subsistemas  │ ◄──────────  │ Endpoints HTTP   │
 │ (loops)      │             │ (Network,    │              │ (Ping, Root,     │
 │              │             │  Server,     │              │  Editor, Wifi…)  │
 │              │             │  Display,    │              │                  │
 │              │             │  Interpreter)│              │                  │
 └──────────────┘             └──────────────┘              └──────────────────┘
        │                            │                              │
        └─────────────┬──────────────┴──────────────┬───────────────┘
                      ▼                             ▼
              ┌──────────────────┐         ┌────────────────────┐
              │   blackboard.hpp │         │    core/plugin/    │
              │ (estado global + │         │  io · observer ·   │
              │   observables)   │         │  logger · locator  │
              └──────────────────┘         │       · lexer      │
                                           └────────────────────┘
```

### Patrones transversales

Toda la arquitectura se apoya en **cinco patrones** que conviene entender antes de tocar código:

| Patrón | Dónde vive | Para qué sirve |
|---|---|---|
| **Blackboard** | [core/src/blackboard.hpp](core/src/blackboard.hpp) | Estructura `static` con la configuración y el estado global del dispositivo (pines, puertos, IPs, modo actual…). Cualquier capa puede leerla. |
| **Observable** | [stride_observer.hpp](core/plugin/observer/include/stride_observer.hpp) | `StrideObservable<T>` envuelve un valor y notifica a sus subscriptores cuando cambia. Es la forma estándar de comunicar cambios de estado entre tareas (p. ej. `Blackboard::CurrentServerMode`). |
| **Service Locator** | [stride_locator.hpp](core/plugin/locator/include/stride_locator.hpp) | Registro tipado de servicios (`StrideLocator::Register<T>` / `Get<T>`). Evita pasar punteros a mano entre subsistemas. |
| **Handler (HTTP)** | [handler.hpp](core/src/handler/include/handler.hpp) | Interfaz común para endpoints HTTP. Cada `Handler` expone uno o varios `httpd_uri_t*` y el `Server` los registra automáticamente. |
| **Logger por subsistema** | [stride_logger.hpp](core/plugin/logger/include/stride_logger.hpp) | `StrideLogger::Log(StrideSubsystem::X, …)` produce logs con un tag estable por área (Network, Server, Interpreter…). |

### Subsistemas (`core/src/brain/`)

- **[network/](core/src/brain/network/)** — Inicializa WiFi (modo AP + STA), persiste credenciales en NVS, controla el LED de red.
- **[server/](core/src/brain/server/)** — Servidor HTTP. Mantiene la lista de `Handler`s y los registra en `httpd`. La función `load_handlers()` decide qué endpoints están activos según `Blackboard::CurrentServerMode` (Developer vs Production).
- **[display/](core/src/brain/display/)** — Pantalla TFT (LovyanGFX) gestionada con una **máquina de estados** (`DisplayBaseState`: `Startup`, `Main`, `View`, `Running`). El singleton `Display::Instance()` posee el TFT y el estado activo; las transiciones se hacen con `Display::transition_to(...)`.
- **[interpreter/](core/src/brain/interpreter/)** — Intérprete del DSL `.str`. Singleton que recorre los `Token`s del `lexer` y ejecuta comandos sobre el hardware (LEDs, botones, buzzer, I2C).
- **[expander/](core/src/brain/expander/)** — Bus I2C maestro para los módulos expansores.
- **[bus/](core/src/brain/bus/)** — Constantes y `spi_sd_init()` para SD y TFT.
- **[apps/](core/src/brain/apps/)** — `AppManager` escanea la SD y mantiene la lista de programas disponibles.

### Tareas FreeRTOS (`core/src/tasks/`)

`app_main` ([core/src/main.cc:14](core/src/main.cc#L14)) crea las tareas y las **fija a un core concreto**:

- **Core 0** — sistema, WiFi, HTTP, monitorización: `hear_server_mode_button_task`, `open_card_task`.
- **Core 1** — UI y cómputo: `hear_program_selected_file_button_task`, `read_card_task` (intérprete), `display_task` (dueño del bus SPI2 del TFT).

> [!IMPORTANT]
> Cualquier tarea que dibuje en el TFT o llame a `Display::transition_to(...)` **debe correr en el Core 1**, porque comparte la propiedad del bus SPI con `display_task`.

### Plugins (`core/plugin/`)

Cada plugin es un **componente IDF independiente** con su propio `CMakeLists.txt`. Esto los hace reusables y testeables aislados:

- **io/** — Drivers de periféricos heredados de `StrideBase` (`StrideLed`, `StrideButton`, `StrideBuzzer`). Encapsulan la API de GPIO de ESP-IDF.
- **observer/** — Plantillas `StrideObservable<T>` y `StrideSubscription` (RAII).
- **logger/** — Logger con enumerado `StrideSubsystem`.
- **locator/** — Mapa `type_index → void*` para inyección de servicios.
- **lexer/** — Tokenizador del DSL (`Token`, `tokenize()`).

### DSL `.str` y archivos en SD

El usuario final no programa C++: escribe scripts `.str` que se almacenan en la SD y se editan vía la web (`/editor`, `/view`, `/browser`). El flujo es: `AppManager` escanea la SD → la pantalla muestra los programas → al seleccionar uno, `read_card_task` lo lee, lo tokeniza con `lexer` y lo ejecuta con `Interpreter`.

---

## ➕ Cómo añadir código nuevo

A continuación, las recetas más habituales. Todas siguen un mismo principio: **localiza la capa adecuada y respeta sus patrones** en lugar de añadir lógica al núcleo.

### 1. Añadir un endpoint HTTP

Crea un nuevo `Handler` siguiendo el patrón de [ping.hpp](core/src/handler/include/ping.hpp) / [ping.cc](core/src/handler/ping.cc).

1. **`core/src/handler/include/<nombre>.hpp`** — hereda de `Handler`, expón `uris()` y declara un handler estático `static esp_err_t handler(httpd_req_t *req)`.
2. **`core/src/handler/<nombre>.cc`** — implementa el constructor (rellena el `httpd_uri_t` con `.uri`, `.method`, `.handler` y `.user_ctx = this`) y la función estática (recupera `this` desde `req->user_ctx`).
3. **Regístralo** en `Server::load_handlers()` ([server.cc:77](core/src/brain/server/server.cc#L77)) con `this->add_handler(new MiHandler());`. Si el endpoint solo debe existir en modo desarrollador, mételo dentro del `if (Blackboard::CurrentServerMode.get() == ServerMode::Developer)`.
4. **Aumenta `Blackboard::MaxHandlers`** si te quedas sin slots (`config.max_uri_handlers`).

No hace falta tocar el `CMakeLists.txt`: el `file(GLOB_RECURSE ...)` de [core/src/CMakeLists.txt](core/src/CMakeLists.txt) recoge los `.cc` nuevos automáticamente.

### 2. Añadir una tarea FreeRTOS

1. **`core/src/tasks/include/mi_task.hpp`** — declara la firma `void mi_task(void *pvParameters);`.
2. **`core/src/tasks/mi_task.cc`** — implementa el bucle. Usa `vTaskDelay(pdMS_TO_TICKS(...))` para ceder CPU y `StrideLogger::Log(...)` para trazas.
3. **Lánzala desde `app_main`** ([core/src/main.cc](core/src/main.cc)) con `xTaskCreatePinnedToCore(...)`, eligiendo el core según la regla anterior (UI/TFT → core 1, resto → core 0). Dimensiona el stack en base a las tareas existentes (4–8 KiB es lo habitual).
4. Si la tarea reacciona a un cambio de estado, **suscríbete a un observable del Blackboard** en vez de hacer polling: `Blackboard::CurrentServerMode.subscribe([](auto& m){ ... });`.

### 3. Añadir un nuevo periférico (LED, sensor, etc.)

Los drivers viven en [core/plugin/io/](core/plugin/io/) y todos derivan de [StrideBase](core/plugin/io/include/stride_base.hpp).

1. Crea `stride_<dispositivo>.hpp` en `core/plugin/io/include/` heredando de `StrideBase`, implementa `start()` y la API pública del periférico (`on/off/toggle/set/get` o equivalente).
2. Crea `stride_<dispositivo>.cc` en `core/plugin/io/`.
3. El `file(GLOB ...)` de [core/plugin/io/CMakeLists.txt](core/plugin/io/CMakeLists.txt) lo recoge solo.
4. Úsalo desde cualquier subsistema o tarea: `StrideLed led(GPIO_NUM_26);`. Si el pin debe ser configurable, **declaralo en el Blackboard** (sección "Global buttons" o similar) en vez de hardcodearlo.

Si el periférico es complejo o tiene su propio stack (p. ej. otro bus), considera crear un **plugin nuevo** (carpeta hermana de `io/`, con su `CMakeLists.txt` y su `idf_component_register`) y añadirlo al `REQUIRES` de [core/src/CMakeLists.txt:30](core/src/CMakeLists.txt#L30).

### 4. Añadir una pantalla/estado al display

La pantalla es una **máquina de estados**. Cada estado implementa [DisplayBaseState](core/src/brain/display/include/display.hpp).

1. Añade el valor al enum `StateType` en [core/src/types/enums.hpp](core/src/types/enums.hpp).
2. Crea `core/src/brain/display/include/<mi>_state.hpp` con una clase que herede de `DisplayBaseState` (`on_enter`, `on_update`, `on_exit`, `get_type`, y opcionalmente `on_input`).
3. Implementa el `.cc` correspondiente; usa `ctx.getTFT()` para dibujar.
4. Para transicionar: `Display::Instance().transition_to(std::make_unique<MiState>());` desde una tarea del **Core 1**.

### 5. Añadir un comando al DSL (`.str`)

1. **Lexer** — añade el `TokenType` en [core/plugin/lexer/include/lexer.hpp](core/plugin/lexer/include/lexer.hpp), su rama en `get_type()`, y la regla de tokenización en [lexer.cc](core/plugin/lexer/lexer.cc).
2. **Intérprete** — declara `execute_mi_comando(const std::vector<Token>&)` en [interpreter.hpp](core/src/brain/interpreter/interpreter.hpp) e impleméntala en [interpreter.cc](core/src/brain/interpreter/interpreter.cc). Engánchala en el dispatch principal de `execute_simple_block_command` / `execute_range`.
3. Si el comando habla con I2C, sigue el patrón de los helpers `executeI2C*` ya existentes.

### 6. Añadir configuración global

Si la nueva funcionalidad expone un parámetro (un pin, un timeout, un puerto…):

1. Añádelo como `static inline` en la sección correspondiente del [Blackboard](core/src/blackboard.hpp).
2. Si su cambio debe **propagarse en caliente**, declaralo como `StrideObservable<T>` y suscríbete donde haga falta.
3. Si debe persistir entre reinicios, guárdalo en **NVS** siguiendo el patrón de `Network::save_net_credentials`.

### 7. Exponer un servicio compartido entre subsistemas

Cuando dos subsistemas necesitan compartir una instancia sin acoplamiento directo, usa el **Locator** en vez de pasar punteros por constructor:

```cpp
StrideLocator::Register<MiServicio>(&instancia);   // dueño lo publica
auto* svc = StrideLocator::Get<MiServicio>();      // consumidor lo recupera
```

---

## 🌐 Configuración de conectividad

El dispositivo arranca con un **fallback estático** (sección `ESPAP`/`LOCAL` para desarrollo) y, en cuanto el usuario guarda credenciales por la web, las **persiste en NVS** y se conecta en automático en los siguientes reinicios.

```json
{
  "ESPAP": { "SSID": "STRIDE_Dev_AP", "PASS": "stride1234" },
  "LOCAL": { "SSID": "Mi_Red_Pruebas", "PASS": "password_pruebas" }
}
```

Flujo del usuario final:

1. **SoftAP** — el ESP32 levanta su propia red WiFi al no encontrar credenciales válidas.
2. **Web de configuración** — el usuario se conecta a esa red e introduce SSID/PASS de su red local.
3. **NVS** — las credenciales se persisten en flash.
4. **Auto-conexión** — los siguientes arranques saltan directamente a modo STA.

### Gestión de archivos (Web UI)

- **`/browser`** — listado de los `.str` en la raíz de la SD.
- **`/editor?file=...`** — editor en navegador para modificar scripts en caliente.
- **`/view`** — visor solo lectura para depuración rápida.

> [!TIP]
> **Optimización de memoria:** los endpoints de lectura (`/view`, `/editor`) usan _chunked transfer_, así que pueden servir archivos grandes sin saturar la heap del ESP32.
