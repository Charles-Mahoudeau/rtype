@echo off
rem R-Type server: starts the AYLO runtime headless on the game in `game\`; any argument is passed on
rem (`<directory>`, `--port <port>`...).
"%~dp0aylo-runtime.exe" --server %*
