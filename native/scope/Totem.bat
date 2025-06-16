@echo off
cd C:\ScopePaymentApi
start C:\ScopePaymentApi\_start_server.bat

cd "C:\Program Files\Google\Chrome\Application\"
start chrome.exe --kiosk-printing "https://asdasdasdasd.homologteste.com.br/boletos/controle/pinpad/1/operacao/tttt/impressora/2"
taskkill /f /im TurboTop.exe
taskkill /f /im TurboTop.exe
taskkill /f /im TurboTop.exe
timeout 10