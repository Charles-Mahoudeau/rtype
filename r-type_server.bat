@echo off
rem R-Type server: starts the AYLO runtime headless on the game in `games\gyatt_rider_2\`; any argument is passed on
rem (`--port <port>`...).
"%~dp0aylo-runtime.exe" --server "%~dp0games\gyatt_rider_2" %*
