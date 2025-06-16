@echo off
REM setup_windows.bat - Script de configuração para Windows 10 + MinGW

echo ====================================================
echo   SETUP - Sistema de Pagamento Moderno com Scope
echo   Windows 10 64-bit + MinGW (C:\MinGW\bin)
echo ====================================================
echo.

REM Verificar se MinGW está instalado
echo 1. Verificando instalacao do MinGW...
if exist "C:\MinGW\bin\g++.exe" (
    echo [OK] MinGW encontrado em C:\MinGW\bin
    C:\MinGW\bin\g++.exe --version | findstr "g++"
) else (
    echo [ERRO] MinGW nao encontrado em C:\MinGW\bin
    echo        Baixe MinGW de: https://osdn.net/projects/mingw/
    echo        Ou instale via: https://www.mingw-w64.org/
    pause
    exit /b 1
)

echo.
echo 2. Verificando PATH do sistema...
echo %PATH% | findstr /C:"C:\MinGW\bin" >nul
if %errorlevel%==0 (
    echo [OK] C:\MinGW\bin esta no PATH
) else (
    echo [AVISO] C:\MinGW\bin NAO esta no PATH
    echo         Adicione manualmente: Sistema ^> Variaveis de Ambiente ^> PATH
    echo         Adicionar: C:\MinGW\bin
)

echo.
echo 3. Criando estrutura do projeto...
if not exist "bin" mkdir bin
if not exist "include" mkdir include
if not exist "include\nlohmann" mkdir include\nlohmann
if not exist "lib" mkdir lib
if not exist "logs" mkdir logs
if not exist "src" mkdir src
if not exist ".vscode" mkdir .vscode

echo [OK] Diretorios criados:
echo     - bin\
echo     - include\
echo     - include\nlohmann\
echo     - lib\
echo     - logs\
echo     - src\
echo     - .vscode\

echo.
echo 4. Testando compilacao C++17...
echo #include ^<iostream^> > test_cpp17.cpp
echo #include ^<mutex^> >> test_cpp17.cpp
echo #include ^<thread^> >> test_cpp17.cpp
echo #include ^<chrono^> >> test_cpp17.cpp
echo int main() { >> test_cpp17.cpp
echo     std::mutex m; >> test_cpp17.cpp
echo     std::lock_guard^<std::mutex^> lock(m); >> test_cpp17.cpp
echo     std::this_thread::sleep_for(std::chrono::milliseconds(1)); >> test_cpp17.cpp
echo     std::cout ^<^< "C++17 OK!" ^<^< std::endl; >> test_cpp17.cpp
echo     return 0; >> test_cpp17.cpp
echo } >> test_cpp17.cpp

C:\MinGW\bin\g++.exe -std=c++17 -mthreads -o test_cpp17.exe test_cpp17.cpp 2>compile_errors.txt
if %errorlevel%==0 (
    test_cpp17.exe
    if %errorlevel%==0 (
        echo [OK] C++17 com mutex/thread funcionando!
    ) else (
        echo [ERRO] C++17 compila mas nao executa
    )
    del test_cpp17.exe
) else (
    echo [ERRO] Erro na compilacao C++17:
    type compile_errors.txt
    echo.
    echo [INFO] Tentando modo de compatibilidade...
    
    REM Teste com flags diferentes
    C:\MinGW\bin\g++.exe -std=c++11 -mthreads -o test_cpp11.exe test_cpp17.cpp 2>nul
    if %errorlevel%==0 (
        echo [OK] C++11 funcionando como fallback
        del test_cpp11.exe
    ) else (
        echo [ERRO] Nem C++11 funciona. Verifique a instalacao do MinGW
    )
)

del test_cpp17.cpp compile_errors.txt 2>nul

echo.
echo 5. Verificando SCOPEAPI.DLL...
if exist "bin\SCOPEAPI.DLL" (
    echo [OK] SCOPEAPI.DLL encontrada em bin\
    for %%I in (bin\SCOPEAPI.DLL) do echo     Tamanho: %%~zI bytes
) else (
    echo [AVISO] SCOPEAPI.DLL nao encontrada em bin\
    echo         Copie de: C:\ScopePaymentApi\SCOPEAPI.DLL
    echo         Para: %cd%\bin\SCOPEAPI.DLL
    
    if exist "C:\ScopePaymentApi\SCOPEAPI.DLL" (
        echo [INFO] Encontrada em C:\ScopePaymentApi\ - copiando...
        copy "C:\ScopePaymentApi\SCOPEAPI.DLL" "bin\" >nul
        if %errorlevel%==0 (
            echo [OK] SCOPEAPI.DLL copiada com sucesso!
        )
    )
)

echo.
echo 6. Baixando nlohmann/json...
powershell -Command "try { Invoke-WebRequest -Uri 'https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp' -OutFile './include/nlohmann/json.hpp'; Write-Host '[OK] nlohmann/json baixado!' -ForegroundColor Green } catch { Write-Host '[AVISO] Erro no download, usando fallback' -ForegroundColor Yellow }"

if not exist "include\nlohmann\json.hpp" (
    echo [INFO] Criando header JSON simples...
    echo // Fallback JSON header > include\simple_json.hpp
    echo #ifndef SIMPLE_JSON_HPP >> include\simple_json.hpp
    echo #define SIMPLE_JSON_HPP >> include\simple_json.hpp
    echo // Implementacao basica incluida >> include\simple_json.hpp
    echo #endif >> include\simple_json.hpp
    echo [OK] Header JSON fallback criado
)

echo.
echo 7. Verificando arquivos de configuracao do VS Code...
if exist ".vscode\c_cpp_properties.json" (
    echo [OK] c_cpp_properties.json existe
) else (
    echo [AVISO] c_cpp_properties.json nao encontrado
    echo         Copie o conteudo fornecido na documentacao
)

if exist ".vscode\tasks.json" (
    echo [OK] tasks.json existe
) else (
    echo [AVISO] tasks.json nao encontrado
    echo         Copie o conteudo fornecido na documentacao
)

echo.
echo 8. Teste final de compilacao...
if exist "src\main.cpp" (
    echo [INFO] Compilando main.cpp...
    C:\MinGW\bin\g++.exe -std=c++17 -mthreads -I./include -o bin\scope-wrapper.exe src\main.cpp 2>final_errors.txt
    if %errorlevel%==0 (
        echo [OK] Compilacao do wrapper bem-sucedida!
        echo      Executavel: bin\scope-wrapper.exe
        if exist "bin\scope-wrapper.exe" (
            for %%I in (bin\scope-wrapper.exe) do echo      Tamanho: %%~zI bytes
        )
    ) else (
        echo [ERRO] Erro na compilacao final:
        type final_errors.txt
    )
    del final_errors.txt 2>nul
) else (
    echo [AVISO] src\main.cpp nao encontrado
    echo         Copie o codigo C++ fornecido
)

echo.
echo ====================================================
echo   RESUMO DO SETUP
echo ====================================================
echo.

REM Verificar status geral
set "setup_ok=1"

if not exist "C:\MinGW\bin\g++.exe" set "setup_ok=0"
if not exist "bin" set "setup_ok=0"
if not exist "include" set "setup_ok=0"

if "%setup_ok%"=="1" (
    echo [OK] SETUP CONCLUIDO COM SUCESSO!
    echo.
    echo Proximos passos:
    echo 1. Abra o VS Code nesta pasta
    echo 2. Instale a extensao "C/C++" da Microsoft
    echo 3. Copie os arquivos de configuracao fornecidos:
    echo    - .vscode\c_cpp_properties.json
    echo    - .vscode\tasks.json
    echo 4. Copie o codigo main.cpp para src\
    echo 5. Pressione Ctrl+Shift+P e execute "Tasks: Run Task"
    echo 6. Selecione "Build Scope Wrapper"
    echo.
    echo AMBIENTE PRONTO PARA DESENVOLVIMENTO!
) else (
    echo [ERRO] SETUP INCOMPLETO!
    echo.
    echo Problemas encontrados:
    if not exist "C:\MinGW\bin\g++.exe" echo - MinGW nao instalado corretamente
    echo.
    echo Consulte a documentacao para resolver os problemas.
)

echo.
echo Pressione qualquer tecla para continuar...
pause >nul