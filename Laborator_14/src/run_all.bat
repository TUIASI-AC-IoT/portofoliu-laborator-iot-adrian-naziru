@echo off
REM Set IP
python set_esp32_ip.py

REM Start ESP32 sim 
start cmd /k python esp32_sim.py

REM wait 2s
timeout /t 2

REM Start REST server
start cmd /k python REST.py

REM wait 2s for REST server to start
timeout /t 2

REM run tests
python test_script.py

pause