@echo off
rem R-Type client: starts the AYLO runtime on the game in `game\`; any argument is passed on
rem (`--game <directory>`, `--host <host>`...).
"%~dp0aylo-runtime.exe" %*
