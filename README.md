# EQScene

EQ paramétrico VST3 de 6 bandas (low cut, 4 campanas, high shelf) con interfaz
de curva interactiva, pensado para Reaper en Windows.

## Compilar en GitHub (sin instalar nada)

1. Sube el contenido de esta carpeta a un repositorio.
2. Ve a la pestaña **Actions** y espera a que termine el workflow *Build EQScene VST3*.
3. Descarga el artefacto **EQScene-VST3-Windows**. Contiene `EQScene.vst3`.

## Compilar en local (Windows)

Requisitos: Visual Studio 2022 con "Desarrollo para escritorio con C++", CMake y Git.

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --target EQScene_VST3

El plugin queda en `build/EQScene_artefacts/Release/VST3/EQScene.vst3`.

## Instalar

Copia `EQScene.vst3` a `C:\Program Files\Common Files\VST3\`, y en Reaper
ejecuta *Options > Preferences > VST* y pulsa *Re-scan*.

## Uso

- Arrastra los nodos de color sobre la curva: horizontal = frecuencia, vertical = ganancia.
- Rueda del ratón sobre un nodo: ajusta el Q.
- Los knobs inferiores dan control fino de cada banda.

## Limitaciones actuales

- Solo estéreo y solo VST3 para Windows.
- Sin analizador de espectro en vivo (la zona de fondo es estática).
- Low cut con pendiente fija de 12 dB/oct.
- Sin bypass por banda ni presets.
