
#!/bin/bash

# Crear directorio para entornos virtuales si no existe
if [[ ! -d "$HOME/venvs" ]]; then
    echo "Creando directorio ~/venvs..."
    mkdir -p ~/venvs
else
    echo "Directorio ~/venvs ya existe"
fi

# Verificar si el entorno virtual de platformio ya existe
if [[ ! -d "$HOME/venvs/platformio" ]]; then
    echo "Creando entorno virtual de PlatformIO..."
    python3 -m venv ~/venvs/platformio
else
    echo "Entorno virtual de PlatformIO ya existe"
fi

# Activar entorno de PlatformIO
echo "Activando entorno virtual de PlatformIO..."
source ~/venvs/platformio/bin/activate

# Verificar si PlatformIO está instalado
if ! python -c "import platformio" 2>/dev/null; then
    echo "Instalando PlatformIO..."
    pip install --upgrade pip
    pip install platformio
else
    echo "PlatformIO ya está instalado"
    # Opcional: actualizar pip de todas formas
    pip install --upgrade pip
fi

echo "Setup completado. Entorno PlatformIO listo para usar."