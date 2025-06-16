@echo off
echo ====================================================
echo   VERIFICAÇÃO MINGW-W64
echo ====================================================
echo.

echo 1. Verificando versão:
g++ --version 2>nul
if %errorlevel% neq 0 (
    echo [ERRO] g++ não encontrado no PATH
    echo        Verifique se MinGW-w64 foi adicionado ao PATH
    goto :error
)

echo.
echo 2. Verificando detalhes:
g++ -v 2>&1 | findstr "Target\|Thread\|collect2\|version"

echo.
echo 3. Testando C++17:
echo #include ^<iostream^> > test_modern.cpp
echo #include ^<memory^> >> test_modern.cpp
echo int main() { >> test_modern.cpp
echo     auto ptr = std::make_unique^<int^>(42); >> test_modern.cpp
echo     std::cout ^<^< "C++17 works: " ^<^< *ptr ^<^< std::endl; >> test_modern.cpp
echo     return 0; >> test_modern.cpp
echo } >> test_modern.cpp

g++ -std=c++17 test_modern.cpp -o test_modern.exe 2>nul
if %errorlevel% equ 0 (
    echo [OK] ✓ C++17 funcionando
    test_modern.exe
    del test_modern.exe
) else (
    echo [ERRO] ✗ C++17 com problemas
)
del test_modern.cpp 2>nul

echo.
echo 4. Testando std::thread:
echo #include ^<iostream^> > test_thread.cpp
echo #include ^<thread^> >> test_thread.cpp
echo #include ^<chrono^> >> test_thread.cpp
echo void worker() { >> test_thread.cpp
echo     std::this_thread::sleep_for(std::chrono::milliseconds(100)); >> test_thread.cpp
echo     std::cout ^<^< "Thread funcionando!" ^<^< std::endl; >> test_thread.cpp
echo } >> test_thread.cpp
echo int main() { >> test_thread.cpp
echo     std::thread t(worker); >> test_thread.cpp
echo     t.join(); >> test_thread.cpp
echo     return 0; >> test_thread.cpp
echo } >> test_thread.cpp

g++ -std=c++17 -pthread test_thread.cpp -o test_thread.exe 2>nul
if %errorlevel% equ 0 (
    echo [OK] ✓ std::thread funcionando
    test_thread.exe
    del test_thread.exe
) else (
    echo [AVISO] ✗ std::thread com problemas (testando sem -pthread)
    g++ -std=c++17 test_thread.cpp -o test_thread.exe 2>nul
    if %errorlevel% equ 0 (
        echo [OK] ✓ std::thread funciona sem -pthread
        test_thread.exe
        del test_thread.exe
    ) else (
        echo [ERRO] ✗ std::thread não funciona
    )
)
del test_thread.cpp 2>nul

echo.
echo 5. Testando std::mutex:
echo #include ^<iostream^> > test_mutex.cpp
echo #include ^<mutex^> >> test_mutex.cpp
echo #include ^<thread^> >> test_mutex.cpp
echo std::mutex mtx; >> test_mutex.cpp
echo void safe_print() { >> test_mutex.cpp
echo     std::lock_guard^<std::mutex^> lock(mtx); >> test_mutex.cpp
echo     std::cout ^<^< "Mutex funcionando!" ^<^< std::endl; >> test_mutex.cpp
echo } >> test_mutex.cpp
echo int main() { >> test_mutex.cpp
echo     std::thread t1(safe_print); >> test_mutex.cpp
echo     std::thread t2(safe_print); >> test_mutex.cpp
echo     t1.join(); t2.join(); >> test_mutex.cpp
echo     return 0; >> test_mutex.cpp
echo } >> test_mutex.cpp

g++ -std=c++17 test_mutex.cpp -o test_mutex.exe 2>nul
if %errorlevel% equ 0 (
    echo [OK] ✓ std::mutex funcionando
    test_mutex.exe
    del test_mutex.exe
) else (
    echo [ERRO] ✗ std::mutex com problemas
)
del test_mutex.cpp 2>nul

echo.
echo 6. Testando localtime_s:
echo #include ^<iostream^> > test_time.cpp
echo #include ^<ctime^> >> test_time.cpp
echo int main() { >> test_time.cpp
echo     time_t t = time(0); >> test_time.cpp
echo     struct tm tm_buf; >> test_time.cpp
echo     errno_t err = localtime_s(^&tm_buf, ^&t); >> test_time.cpp
echo     if (err == 0) { >> test_time.cpp
echo         std::cout ^<^< "localtime_s funcionando!" ^<^< std::endl; >> test_time.cpp
echo     } else { >> test_time.cpp
echo         std::cout ^<^< "Usando localtime padrão" ^<^< std::endl; >> test_time.cpp
echo     } >> test_time.cpp
echo     return 0; >> test_time.cpp
echo } >> test_time.cpp

g++ -std=c++17 test_time.cpp -o test_time.exe 2>nul
if %errorlevel% equ 0 (
    test_time.exe
    del test_time.exe
) else (
    echo [INFO] localtime_s não disponível (normal no MinGW)
)
del test_time.cpp 2>nul

echo.
echo 7. Compilando versão completa do ScopeWrapper:
if exist "src\main.cpp" (
    echo [INFO] Testando compilação do projeto principal...
    
    g++ -std=c++17 ^
        -I./include ^
        -static-libgcc ^
        -static-libstdc++ ^
        -DWIN32_LEAN_AND_MEAN ^
        -O2 ^
        -o bin/scope-wrapper-modern.exe ^
        src/main.cpp 2>compile_test.txt
    
    if %errorlevel% equ 0 (
        echo [OK] ✓ ScopeWrapper compilou com MinGW-w64!
        
        if exist "bin\scope-wrapper-modern.exe" (
            for %%I in (bin\scope-wrapper-modern.exe) do (
                echo      Executável: bin\scope-wrapper-modern.exe
                echo      Tamanho: %%~zI bytes
            )
            
            echo [INFO] Testando execução...
            echo {"command":"status"} | bin\scope-wrapper-modern.exe 2>nul
            if %errorlevel% equ 0 (
                echo [OK] ✓ Execução funcionando!
            ) else (
                echo [AVISO] Execução com problema (provavelmente falta SCOPEAPI.DLL)
            )
        )
        del compile_test.txt 2>nul
    ) else (
        echo [ERRO] ✗ Erro na compilação:
        type compile_test.txt
        del compile_test.txt 2>nul
    )
) else (
    echo [INFO] src\main.cpp não encontrado - pule esta etapa
)

echo.
echo ====================================================
echo   RESULTADO DA VERIFICAÇÃO
echo ====================================================
echo.

g++ --version | findstr "gcc"
echo.

echo ✓ Se tudo passou, seu MinGW-w64 está perfeito!
echo ✓ Agora você pode usar C++17 completo com threading
echo ✓ std::mutex e std::thread funcionam nativamente
echo.

goto :end

:error
echo.
echo ====================================================
echo   PROBLEMAS ENCONTRADOS
echo ====================================================
echo.
echo Possíveis soluções:
echo 1. Verificar se MinGW-w64 está no PATH
echo 2. Reiniciar terminal/VS Code
echo 3. Verificar se não há conflito com MinGW antigo
echo.
echo Para limpar PATH do MinGW antigo:
echo set PATH=%PATH:C:\MinGW\bin;=%
echo.

:end
pause