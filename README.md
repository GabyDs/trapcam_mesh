# Mi Proyecto Mesh Lite

Basado en el ejemplo [mesh_local_control](https://github.com/espressif/esp-mesh-lite/tree/release/v1.0/examples/mesh_local_control) de ESP-Mesh-Lite.

## Requisitos

- ESP-IDF v5.x
- Python 3.x

## Instalación

```bash
# 1. Clonar con submodules
git clone --recursive https://github.com/TU_USUARIO/mi_proyecto_mesh.git
cd mi_proyecto_mesh

# O si ya clonaste sin --recursive:
git submodule update --init --recursive

# 2. Crear symlinks de componentes
./setup_components.sh

# 3. Configurar y compilar
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py flash monitor