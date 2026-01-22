# Arquitectura del Proyecto - TrapcamMesh

## Índice
1. [Visión General](#visión-general)
2. [Estructura del Proyecto](#estructura-del-proyecto)
3. [Componentes Principales](#componentes-principales)
4. [Sistema de Configuración](#sistema-de-configuración)
5. [Sistema de Build](#sistema-de-build)
6. [Flujo de Ejecución](#flujo-de-ejecución)
7. [Conceptos Clave de ESP-IDF](#conceptos-clave-de-esp-idf)

---

## Visión General

Este proyecto está basado en el framework **ESP-IDF (Espressif IoT Development Framework)** y utiliza **ESP-MESH-LITE** para crear una red mallada (mesh) de dispositivos ESP32. El objetivo es establecer una red autónoma donde múltiples dispositivos ESP32 se interconectan formando una topología mesh auto-organizada y auto-sanante.

### ¿Qué es ESP-MESH-LITE?

ESP-MESH-LITE es una solución de red mallada que funciona en modo **SoftAP + Station** simultáneamente:
- **Station (STA)**: Se conecta al router o a otro nodo padre
- **SoftAP (Access Point)**: Permite que otros dispositivos se conecten a él como nodos hijos

La principal ventaja sobre ESP-MESH tradicional es que **cada nodo puede acceder a internet de forma independiente**, sin que los datos pasen por el nodo raíz, simplificando enormemente el desarrollo de aplicaciones.

---

## Estructura del Proyecto

### Directorios Principales

```
trapcam_mesh/
├── main/                      # Aplicación principal del usuario
├── components/                # Componentes locales (symlinks)
├── managed_components/        # Componentes descargados automáticamente
├── esp-mesh-lite/            # Submodulo de ESP-MESH-LITE
├── build/                     # Archivos generados por compilación
├── docs/                      # Documentación del proyecto
└── Archivos de configuración
```

---

## Componentes Principales

### 1. **main/** - Aplicación Principal

**Archivo:** `local_control.c`

**Función:** Contiene la lógica de aplicación del usuario. Es el punto de entrada del programa.

**Responsabilidades:**
- Inicialización del sistema (NVS, WiFi, event loop)
- Configuración de la red mesh
- Monitoreo y reporte del estado del sistema

**Flujo de Ejecución en `app_main()`:**

1. **Inicialización de almacenamiento NVS** (`esp_storage_init()`):
   - NVS (Non-Volatile Storage) es una memoria flash que persiste datos incluso sin alimentación
   - Almacena configuración WiFi, credenciales, etc.

2. **Inicialización de red** (`esp_netif_init()`):
   - Crea las interfaces de red (STA y AP)
   - Configura el stack TCP/IP (LwIP)

3. **Configuración WiFi** (`wifi_init()`):
   - Configura SSID y password del router (Station)
   - Configura SSID y password del SoftAP propio

4. **Inicialización de Mesh-Lite** (`esp_mesh_lite_init()`):
   - Configura los parámetros de la red mesh
   - Establece callbacks y políticas de red

5. **Inicio del sistema mesh** (`esp_mesh_lite_start()`):
   - Comienza el proceso de auto-organización
   - El dispositivo busca red mesh existente o crea una nueva

6. **Timer de monitoreo** (`xTimerCreate()`):
   - Cada 10 segundos imprime información del sistema
   - Muestra: canal, nivel en jerarquía, MAC, RSSI, nodos hijos

**Funciones Clave:**

- **`print_system_info_timercb()`**: Callback del timer que imprime estado de la red mesh cada 10 segundos
- **`app_wifi_set_softap_info()`**: Configura SSID/password del SoftAP, puede agregar MAC al nombre para identificación única

---

### 2. **components/** - Componentes Locales

Este directorio contiene **symlinks** (enlaces simbólicos) a los componentes de `esp-mesh-lite/components/`. Creados por el script `setup_components.sh`.

#### **2.1 mesh_lite**

**Función:** Implementación del protocolo ESP-MESH-LITE.

**Características:**
- Auto-organización: Los nodos encuentran automáticamente su lugar en la jerarquía
- Auto-sanación: Si un nodo padre falla, los hijos buscan nueva ruta
- Jerarquía flexible: Nodo puede ser root, padre, hijo o leaf
- Acceso independiente a internet: Cada nodo tiene su propia conexión
- Comunicación inter-nodos: Broadcast, unicast, mensajes a root

**Conceptos:**
- **Root Node (Nodo Raíz)**: Nodo conectado directamente al router
- **Parent Node (Nodo Padre)**: Nodo que tiene hijos conectados a su SoftAP
- **Child Node (Nodo Hijo)**: Nodo conectado al SoftAP de otro nodo
- **Level (Nivel)**: Profundidad en jerarquía (root=1, hijo de root=2, etc.)

#### **2.2 wifi_provisioning**

**Función:** Permite configurar credenciales WiFi del router sin hardcodearlas.

**Métodos de aprovisionamiento:**
- BLE (Bluetooth Low Energy)
- SoftAP (usuario se conecta al AP del ESP32)
- Console (comandos por puerto serie)

**Beneficio:** Usuario final puede configurar WiFi desde app móvil sin recompilar firmware.

---

### 3. **managed_components/** - Dependencias Administradas

Estos componentes son descargados automáticamente por el **IDF Component Manager** según `idf_component.yml`.

#### **3.1 espressif__iot_bridge**

**Función:** Proporciona funcionalidad de "bridge" (puente) de red.

**Capacidades:**
- NAT (Network Address Translation): Permite que nodos hijos accedan a internet a través del padre
- IP Forwarding: Reenvío de paquetes entre interfaces (STA ↔ AP)
- DHCP Server: Asigna IPs a dispositivos conectados al SoftAP
- DNS Relay: Reenvía consultas DNS

**Por qué es importante:** Es el corazón que permite que ESP-MESH-LITE funcione. Cada nodo actúa como router para sus hijos.

#### **3.2 espressif__qrcode**

**Función:** Generación de códigos QR en consola serial.

**Uso típico:** Mostrar QR con credenciales WiFi para aprovisionamiento rápido desde app móvil.

#### **3.3 espressif__esp_modem**

**Función:** Soporte para módems celulares (4G/LTE).

**Uso:** Si se desea que el nodo raíz obtenga internet desde módem en lugar de WiFi router.

---

### 4. **esp-mesh-lite/** - Submódulo Git

**Función:** Repositorio oficial de Espressif con código fuente de ESP-MESH-LITE.

**Estructura:**
- `components/`: Código de los componentes mesh_lite y wifi_provisioning
- `examples/`: Ejemplos de uso (este proyecto está basado en `mesh_local_control`)
- `docs/`: Documentación detallada

**Por qué submódulo:** Permite actualizar a nuevas versiones de mesh-lite sin copiar archivos manualmente.

---

### 5. **build/** - Directorio de Compilación

Generado por CMake/Ninja al ejecutar `idf.py build`. Contiene:

- **bootloader/**: Bootloader compilado (primera etapa de arranque)
- **partition_table**: Tabla de particiones (ubicación de app, nvs, etc.)
- **mesh_local_control.elf**: Ejecutable con símbolos de debug
- **mesh_local_control.bin**: Binario final para flashear
- **compile_commands.json**: Usado por IDEs para autocompletado y análisis
- **sdkconfig.h**: Configuración en formato C header
- **flash_args**: Argumentos para herramienta de flasheo

---

## Sistema de Configuración

ESP-IDF usa **Kconfig** para configuración centralizada. Similar a la configuración del kernel Linux.

### Archivos de Configuración

#### **sdkconfig**
- Archivo principal generado por `idf.py menuconfig`
- Contiene TODAS las opciones de configuración (miles de líneas)
- No se debe editar manualmente
- Específico de cada máquina (no commitear a Git)

#### **sdkconfig.defaults**
- Valores por defecto del proyecto
- Se aplica en builds limpios
- SÍ se commitea a Git
- Ejemplo:
  ```
  CONFIG_FREERTOS_HZ=1000              # Tick rate del scheduler
  CONFIG_LWIP_IP_FORWARD=y             # Habilitar forwarding de IP
  CONFIG_LWIP_IPV4_NAPT=y              # Habilitar NAT
  CONFIG_MESH_LITE_ENABLE=y            # Habilitar mesh lite
  ```

#### **Kconfig.projbuild**
- Define opciones de configuración específicas del proyecto
- Se integra en `idf.py menuconfig`
- Ejemplo de este proyecto:
  ```
  CONFIG_ROUTER_SSID       # SSID del router
  CONFIG_ROUTER_PASSWORD   # Password del router
  ```

### Acceso a configuración en código

```c
// Las opciones CONFIG_* se convierten en #define en sdkconfig.h
#include "sdkconfig.h"

char* ssid = CONFIG_ROUTER_SSID;      // Acceso directo
char* pass = CONFIG_ROUTER_PASSWORD;
```

---

## Sistema de Build

### CMake + Ninja

ESP-IDF usa CMake como sistema de build, que genera archivos para Ninja (herramienta de build rápida).

#### **CMakeLists.txt (raíz)**
```cmake
cmake_minimum_required(VERSION 3.5)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mesh_local_control)
```

**Función:**
- Define el nombre del proyecto
- Incluye el sistema de build de ESP-IDF
- ESP-IDF automáticamente descubre componentes en `components/` y `main/`

#### **main/CMakeLists.txt**
```cmake
idf_component_register(
    SRCS "local_control.c"
    INCLUDE_DIRS "."
)
```

**Función:**
- Registra los archivos fuente del componente main
- Define directorios de includes
- Opcionalmente, especifica dependencias con otros componentes

#### **main/idf_component.yml**
```yaml
dependencies:
  idf: ">=5.0"
  mesh_lite:
    version: "*"
    override_path: "../../../components/mesh_lite"
```

**Función:**
- Manifiesto del IDF Component Manager
- Especifica dependencias con versiones
- `override_path` indica usar versión local en lugar de descargar

### Proceso de Build

```bash
idf.py build
```

**Pasos internos:**
1. CMake analiza todos los CMakeLists.txt
2. Descarga managed_components si es necesario
3. Genera Kconfig y procesa sdkconfig
4. Genera build.ninja con reglas de compilación
5. Ninja ejecuta compilación paralela
6. Genera .elf, .bin, .map
7. Calcula direcciones de flash y genera flasher_args.json

---

## Flujo de Ejecución

### 1. Arranque del Hardware

1. **ROM Bootloader** (en chip):
   - Lee bootloader de segunda etapa desde flash
   - Verifica integridad (opcional)
   - Salta a bootloader de segunda etapa

2. **Bootloader de Segunda Etapa** (`build/bootloader/`):
   - Lee partition table
   - Verifica app (opcional)
   - Habilita seguridad (opcional)
   - Carga app en RAM y ejecuta

3. **App** (`mesh_local_control.bin`):
   - Inicializa heap, stacks
   - Inicia FreeRTOS scheduler
   - Llama a `app_main()`

### 2. Inicialización de la Red Mesh

1. **Configuración Inicial:**
   ```
   WiFi STA: Intenta conectarse a router
   WiFi AP:  Inicia SoftAP para aceptar nodos hijos
   ```

2. **Determinación de Rol:**
   - Si conecta al router → Se vuelve **Root Node**
   - Si no hay router pero hay mesh → Busca **Parent Node**
   - Si está solo → Se vuelve Root sin internet (mesh standalone)

3. **Jerarquía Dinámica:**
   ```
   Level 1: Root (conectado a router)
     ├─ Level 2: Child 1
     │    ├─ Level 3: Child 1.1
     │    └─ Level 3: Child 1.2
     └─ Level 2: Child 2
   ```

4. **Auto-sanación:**
   - Si Child 1 pierde conexión con Root
   - Child 1.1 y 1.2 buscan nuevo padre (pueden conectarse a Child 2)
   - Red se reorganiza automáticamente

### 3. Monitoreo del Sistema

- **Timer de información**: Cada 10 segundos se imprime el estado de la red
- **Información mostrada**:
  - Canal WiFi actual
  - Nivel en jerarquía mesh
  - MAC address propia
  - BSSID y RSSI del nodo padre
  - Lista de nodos hijos conectados
  - Memoria libre disponible

---

## Conceptos Clave de ESP-IDF

### FreeRTOS

**¿Qué es?** Sistema operativo en tiempo real (RTOS) incluido en ESP-IDF.

**Conceptos:**
- **Task (Tarea)**: Hilo de ejecución con prioridad y stack propio
  ```c
  xTaskCreate(tcp_client_write_task, "tcp_client", 4096, NULL, 5, NULL);
  //          función               nombre        stack  args  prio handle
  ```
- **Scheduler**: Decide qué tarea ejecutar según prioridades
- **Tick**: Unidad de tiempo (CONFIG_FREERTOS_HZ=1000 → 1ms por tick)
- **Queue**: Cola thread-safe para pasar datos entre tareas
- **Semaphore/Mutex**: Sincronización entre tareas
- **Timer**: Callback ejecutado periódicamente

### Event Loop

**Función:** Sistema de eventos asíncronos tipo pub/sub.

**Componentes:**
- **Event Base**: Categoría (IP_EVENT, WIFI_EVENT, etc.)
- **Event ID**: Evento específico (IP_EVENT_STA_GOT_IP, WIFI_EVENT_STA_CONNECTED)
- **Handler**: Función callback ejecutada al recibir evento

**Ejemplo:**
```c
ESP_ERROR_CHECK(esp_event_handler_instance_register(
    IP_EVENT,                          // Base
    IP_EVENT_STA_GOT_IP,              // ID
    &ip_event_sta_got_ip_handler,     // Handler
    NULL,                              // Arg
    NULL                               // Instance handle
));
```

### Netif (Network Interface)

**Función:** Abstracción de interfaces de red.

**Tipos:**
- `WIFI_STA_DEF`: Interfaz WiFi Station
- `WIFI_AP_DEF`: Interfaz WiFi Access Point
- `ETH_DEF`: Interfaz Ethernet (si hay)

**Capa:**
```
App ↔ [Netif] ↔ LwIP (TCP/IP stack) ↔ WiFi Driver ↔ Hardware
```

### NVS (Non-Volatile Storage)

**Función:** Sistema de almacenamiento clave-valor en flash.

**Uso típico:**
- Credenciales WiFi
- Configuración de usuario
- Contadores persistentes
- Calibración

**Particiones:**
- Se define en `partition_table` (típicamente 24KB)
- Soporta wear leveling (distribución de escrituras)

### LwIP (Lightweight IP)

**Función:** Stack TCP/IP embebido.

**Características habilitadas en este proyecto:**
- `CONFIG_LWIP_IP_FORWARD=y`: Forwarding entre interfaces (STA → AP)
- `CONFIG_LWIP_IPV4_NAPT=y`: NAT (cambiar IP origen/destino en paquetes)

**Flujo de paquete (nodo hijo):**
```
App (socket) → LwIP → Netif STA → WiFi → Nodo Padre
                                            ↓
                                    NAT (cambia IP src)
                                            ↓
                                        Router → Internet
```

---

## Resumen de Flujo Completo

1. **Power ON** → ROM Bootloader → 2nd Stage Bootloader → App

2. **`app_main()`**:
   - Inicializa NVS, netif, event loop
   - Configura WiFi (STA + AP)
   - Inicializa mesh-lite
   - Inicia mesh-lite
   - Crea timer de monitoreo

3. **Mesh Auto-organización**:
   - Busca router o nodos mesh existentes
   - Determina rol (root/child)
   - Asigna nivel en jerarquía
   - Inicia SoftAP para aceptar hijos

4. **Operación Normal**:
   - Timer imprime estado cada 10s
   - Mesh monitorea y auto-sana conexiones
   - Nodos hijos pueden entrar/salir dinámicamente

---

## Archivos de Configuración del Proyecto

### setup_components.sh

**Función:** Script bash que crea symlinks desde `esp-mesh-lite/components/` a `components/`.

**Por qué:** CMake de ESP-IDF busca componentes en `components/`. Los symlinks evitan duplicar código.

**Ejecución:** Una sola vez tras clonar repo:
```bash
./setup_components.sh
```

### .gitmodules

Define submódulos Git (esp-mesh-lite). Al clonar con `--recursive` se descargan automáticamente.

---

## Consideraciones de Desarrollo

### Debugging

- **Logs**: Usar `ESP_LOGI()`, `ESP_LOGW()`, `ESP_LOGE()`
- **Log level**: Configurar por tag con `esp_log_level_set()`
- **JTAG**: Soporta GDB para debugging hardware
- **Core dumps**: Guarda estado en flash tras crash

### OTA (Over-The-Air Updates)

- Partition table debe incluir partición `ota_data` y dos particiones `app`
- Usar componente `esp_https_ota`
- Mesh permite actualizar todos los nodos desde root

### Seguridad

- Flash encryption: Cifra firmware en flash
- Secure boot: Verifica firma digital del bootloader/app
- WiFi WPA2/WPA3: Credenciales cifradas
- TLS: Para comunicación TCP segura

---

## Próximos Pasos Sugeridos

1. **Entender el flujo**: Seguir con debugger paso a paso desde `app_main()`
2. **Experimentar**: Cambiar configuración en `menuconfig` y observar efectos
3. **Agregar funcionalidad**: Implementar nuevas características sobre la red mesh básica (ej: sensores, comunicación entre nodos)
4. **Mesh avanzado**: Implementar comunicación inter-nodos usando la API de mesh-lite
5. **Provisioning**: Integrar wifi_provisioning vía BLE para configuración dinámica

---

## Recursos Adicionales

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP-MESH-LITE User Guide](https://github.com/espressif/esp-mesh-lite/blob/release/v1.0/components/mesh_lite/User_Guide.md)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [LwIP Wiki](https://lwip.fandom.com/wiki/LwIP_Wiki)

---

**Documento creado:** Enero 2026  
**Versión del proyecto:** Basado en ESP-MESH-LITE v1.0 y ESP-IDF v5.x