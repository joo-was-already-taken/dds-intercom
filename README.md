# DDS Audio Intercom

This project is a personal learning exercise created to explore and zero-configuration
communication using Data Distribution Service (DDS).

## Build Instructions
Build the project using CMake & Ninja:
```bash
cmake -B build -G Ninja
cmake --build build
```
(or use `flake.nix`)

## Usage

Once built, you can start your intercom node. The application requires a name/callsign
as a positional argument.
```bash
build/dds-intercom <NAME> [OPTIONS]
```
Run `build/dds-intercom --help` for options.

### Example
Start an active two-way node named "Alpha" on domain 42:
```bash
./build/dds-intercom Alpha -d 42
```
Start a listen-only node named "Bravo" on the same domain:
```bash
./build/dds-intercom Bravo --domain 42 --role listen
```
