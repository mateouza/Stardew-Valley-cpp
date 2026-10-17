# Stardew Valley - TP Final

Trabajo final grupal de Taller de Programacion I, Catedra Veiga, FIUBA.

**Cuatrimestre:** 2C de 2026  
**Corrector asignado:** Mateo Capon

## Integrantes

| Nombre | Padron |
|---|---|
| José Gonzalez Casartelli | 113633 |
| Mateo Josue Uzarralde | 113581 |
| Juana de la Torre | 113534 |
| Federica Guasch | 112453 |

## Dependencias

Ubuntu 24.04, incluyendo WSL. Se requiere C++20 y CMake 3.28 o superior.

```bash
sudo apt update
sudo apt install build-essential cmake git libgtest-dev clang-format-18 cppcheck pre-commit valgrind
```

Controles verificados con clang-format 18, cppcheck 2.13.0 y pre-commit 3.6.2.

## Comandos

Ejecutar desde la raiz del proyecto:

| Comando | Funcion |
|---|---|
| `make` | Compilar en Debug, incluyendo los tests |
| `make build JOBS=4` | Compilar con cuatro trabajos en paralelo |
| `make test` | Compilar y ejecutar todos los tests |
| `make format` | Aplicar clang-format |
| `make lint` | Ejecutar cppcheck |
| `make check` | Ejecutar clang-format, cppcheck, compilacion y tests |
| `make valgrind` | Compilar y revisar memoria en los tests |
| `make hooks` | Activar el pre-commit |
| `make clean` | Limpiar productos compilados conservando la configuracion |

## Pre-commit

Activar una vez por clon:

```bash
make hooks
```

Ejecuta clang-format y cppcheck antes de cada commit. Clang-format aplica el formato automaticamente.

Ejecucion manual:

```bash
pre-commit run --all-files
```

Los archivos de catedra estan excluidos como entradas directas de los hooks y no deben modificarse.

## Tests y Valgrind

```bash
make test
make valgrind
```

Los tests de protocolo necesitan el puerto TCP 18080 libre. No ejecutar simultaneamente distintas ejecuciones de tests o Valgrind que utilicen ese puerto.

Valgrind ejecuta los binarios de tests secuencialmente. La lista se mantiene en `scripts/check_memory.sh`.

Cada binario tiene un limite de 60 segundos y 5 segundos adicionales antes del cierre forzado.
