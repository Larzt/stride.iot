![tests](https://github.com/Larzt/tfg-stride/actions/workflows/tests.yml/badge.svg)
![ESP-IDF Build](https://github.com/Larzt/tfg-stride/actions/workflows/esp-idf-build.yml/badge.svg)

# 📡 Servidor HTTP Modular para ESP32

Este repositorio contiene la implementación base del firmware para el **Dispositivo Modular STRIDE**, un **PROYECTO DE TRABAJO DE FIN DE GRADO (TFG)**.

El objetivo principal es establecer una **arquitectura de software flexible** utilizando **ESP-IDF** para el microcontrolador ESP32, resolviendo la **falta de versatilidad** de los dispositivos IoT comerciales. Este núcleo central actúa como el "cerebro" del sistema, integrando conectividad **WiFi, una API y una pantalla**, permitiendo la **fácil reconfiguración** mediante módulos intercambiables los cuales pueden ser programados con facilidad gracias a su propio DSL desarrollado.

## ⚙️ Características Técnicas y Funcionalidades

El proyecto implementa las siguientes funcionalidades iniciales:

- **Arquitectura:** Estructura de código pensada para ser **escalable y fácilmente mantenible**, con capas lógicas separadas (WiFi, Servidor, Controladores de Hardware).
- **Conectividad Híbrida:** Inicialización de la interfaz WiFi en **modo AP (Access Point) + STA (Station)**.
- **Servidor HTTP Ligero:** Servidor implementado para la interacción remota y la obtención de datos.
- **Control de Hardware:** Controladores separados para el hardware.

## 🌐 Configuración de Conectividad

El dispositivo STRIDE implementa un sistema de gestión de red dual, diferenciando entre la configuración estática de desarrollo y la configuración dinámica para el usuario final.

#### Configuración de Desarrollo

The project **hello_world** contains one source file in C language [hello_world_main.c](src/hello_world_main.c). The file is located in folder [main](src).

```json
{
  "ESPAP": {
    "SSID": "STRIDE_Dev_AP",
    "PASS": "stride1234"
  },
  "LOCAL": {
    "SSID": "Mi_Red_Pruebas",
    "PASS": "password_pruebas"
  }
}
```

> [!NOTE]
> Este archivo solo se utiliza como fallback (variables iniciales) o durante el ciclo de depuracion en el laboratio.

### Configuraion para el Usuario Final

Pensando en la versatilidad del dispositivo, el suuario no necesita modificar el codigo fuente. El flujo de conexion es el siguiente:

1. Modo SoftAP: Al encenderse, el ESP32 crea su propia red Wi-Fi (Punto de Acceso).
2. Interfaz de Configuración: El usuario se conecta a dicha red mediante un smartphone o PC.
3. Persistencia en NVS: A través del servidor HTTP embebido, el usuario introduce las credenciales de su red local. Estas se guardan permanentemente en la memoria NVS (Non-Volatile Storage).
4. Auto-Conexión: En los siguientes reinicios, el dispositivo priorizará los datos de la NVS, conectándose automáticamente a la red del usuario sin intervención manual.

### Gestión de Archivos (Web UI)

El sistema expone una interfaz web para la gestión de módulos y scripts:

- **Browser (`browser`)**: Permite navegar por la raíz de la tarjeta SD, listando archivos detectados.
- **Editor (`/editor?file=...`)**: Interfaz web con un área de texto (textarea) que permite modificar en tiempo real el código fuente de los programas `.str` almacenados.
- **Visualizador (`/view`)**: Renderiza el contenido del archivo en modo solo lectura para depuración rápida.

> [!TIP]
> **Optimización de Memoria:** Los endpoints de lectura (`/view` y `/editor`) utilizan transferencia de datos por fragmentos (_chunked transfer_). Esto permite servir archivos de gran tamaño almacenados en la SD sin exceder el límite de memoria heap del microcontrolador.
