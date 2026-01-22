#!/bin/bash
# setup_components.sh - Crear symlinks de esp-mesh-lite components

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Verificar que el submodule existe
if [ ! -d "esp-mesh-lite/components" ]; then
    echo "Error: esp-mesh-lite/components no encontrado"
    echo "Ejecuta: git submodule update --init --recursive"
    exit 1
fi

# Crear directorio components si no existe
mkdir -p ./components

# Crear symlinks para cada componente de esp-mesh-lite
echo "Creando symlinks de esp-mesh-lite..."
for item in esp-mesh-lite/components/*; do
    if [ -d "$item" ]; then
        name=$(basename "$item")
        target="./components/$name"
        
        # Si ya existe, eliminarlo
        if [ -L "$target" ] || [ -e "$target" ]; then
            rm -rf "$target"
        fi
        
        # Crear symlink relativo
        ln -s "../$item" "$target"
        echo "  ✓ $name"
    fi
done

echo ""
echo "Symlinks creados exitosamente en ./components/"
echo "Componentes disponibles:"
ls -la ./components/
