@echo off
rem R-Type client: starts the AYLO runtime on the game in `games\gyatt_rider_2\`; any argument is passed on
rem (`--host <host>`, `--port <port>`...).
"%~dp0aylo-runtime.exe" "%~dp0games\gyatt_rider_2" %*
