# RealSNI

RealSNI (Reality Scanner) is a fast, standalone, and professional Windows application built with C++ and ImGui to help you scan and find the best SNI domains for your X-UI Reality configuration.

## Features
- **Standalone Executable**: Fully compiled natively on Windows, requiring no external dependencies like Python.
- **Auto-downloads Xray Core**: Downloads the correct version of Xray core automatically if missing.
- **Clean UI**: A beautiful, dark-themed interface built using ImGui.
- **Config Management**: Automatically communicates with your X-UI panel to create, update, and test temporary configs.
- **Auto Cleanup**: Deletes the temporary config and client once scanning is complete to keep your panel clean.
- **Smart Sorting**: Sort domains by response delay, name, or status.

## Usage
Simply run `RealityScanner.exe`.

## Build
The project uses CMake. You can use the provided PowerShell script:
```powershell
.\auto_build.ps1
```
It will fetch dependencies, build the project, and create a standalone executable.
