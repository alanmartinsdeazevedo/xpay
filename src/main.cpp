// main.cpp - Versão corrigida com headers apropriados
// Compile com: g++ -std=c++17 -I./include -o scope-wrapper.exe main.cpp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Headers C++ padrão
#include <iostream>
#include <string>
#include <memory>
#include <map>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <exception>
#include <stdexcept>

// Headers C++11/17 - verificar disponibilidade
#if __cplusplus >= 201103L
    #include <chrono>
    #include <thread>
    #include <mutex>
    #include <condition_variable>
    #define CPP11_AVAILABLE 1
#else
    #define CPP11_AVAILABLE 0
    // Fallback para versões antigas
    #include <ctime>
#endif

// Para JSON, incluir nlohmann/json ou implementar parser simples
// Se não tiver nlohmann/json, usar implementação básica abaixo
#ifdef NLOHMANN_JSON_HPP
    #include <nlohmann/json.hpp>
    using json = nlohmann::json;
#else
    // Implementação JSON básica para compilar sem dependências externas
    #include "simple_json.hpp"
#endif

// === Definições de Tipos da SCOPEAPI ===
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef long LONG;
typedef char* LPSTR;
typedef const char* LPCSTR;

// Estrutura de coleta conforme documentação oficial
typedef struct _stPARAM_COLETA {
    WORD Bandeira;          // Código da bandeira
    WORD FormatoDado;       // Formato do dado a ser coletado
    WORD HabTeclas;         // Teclas habilitadas
    char MsgOp1[17];        // Mensagem linha 1 do operador
    char MsgOp2[17];        // Mensagem linha 2 do operador
    char MsgCl1[17];        // Mensagem linha 1 do cliente
    char MsgCl2[17];        // Mensagem linha 2 do cliente
    WORD TamMinDado;        // Tamanho mínimo do dado
    WORD TamMaxDado;        // Tamanho máximo do dado
    WORD TamMinCartao;      // Tamanho mínimo do cartão
    WORD TamMaxCartao;      // Tamanho máximo do cartão
    char CodSeguranca;      // Código de segurança
    char Reservado[13];     // Reservado para uso futuro
} stPARAM_COLETA, *ptPARAM_COLETA;

// Estrutura de coleta estendida
typedef struct _stPARAM_COLETA_EXT {
    BYTE FormatoDado;       // Formato do dado
    BYTE HabTeclas;         // Teclas habilitadas
    char MsgOp1[17];        // Mensagem operador linha 1
    char MsgCl1[17];        // Mensagem cliente linha 1
    WORD TamMinDado;        // Tamanho mínimo
    WORD TamMaxDado;        // Tamanho máximo
    char Reservado[32];     // Reservado
} stPARAM_COLETA_EXT, *ptPARAM_COLETA_EXT;

// Enumeração de ações
enum eACAO_APL {
    APP_ACAO_CONTINUAR = 1,
    APP_ACAO_RETORNAR = 2,
    APP_ACAO_CANCELAR = 3,
    APP_ACAO_ERRO_COLETA = 4,
    APP_ACAO_OK = 5
};

// Códigos de retorno importantes
const LONG RCS_TRN_EM_ANDAMENTO = 65024;    // 0xFE00
const LONG TC_IMPRIME_CUPOM = 64514;        // 0xFC02
const LONG TC_INFO_RET_FLUXO = 64766;       // 0xFCFE
const LONG TC_INFO_AGU_CONF_OP = 64767;     // 0xFCFF
const LONG TC_COLETA_DADO_ESPECIAL = 64756; // 0xFCF4
const LONG TC_COLETA_EXT = 64763;           // 0xFCFB
const LONG TC_OBTEM_QRCODE = 64755;         // 0xFCF3

// === Ponteiros para Funções da DLL ===
typedef LONG (__stdcall *PFN_ScopeOpen)(LPSTR tipoPdv, LPSTR empresa, LPSTR filial, LPSTR pdv);
typedef LONG (__stdcall *PFN_ScopeClose)(void);
typedef LONG (__stdcall *PFN_ScopeAbreSessaoTEF)(void);
typedef LONG (__stdcall *PFN_ScopeFechaSessaoTEF)(BYTE acao, BYTE *desfez);
typedef LONG (__stdcall *PFN_ScopeStatus)(void);
typedef LONG (__stdcall *PFN_ScopeCompraCartaoCredito)(LPSTR Valor, LPSTR TxServico);
typedef LONG (__stdcall *PFN_ScopeCompraCartaoDebito)(LPSTR Valor);
typedef LONG (__stdcall *PFN_ScopeCarteiraVirtualEx2)(LPSTR Valor, WORD CodBandeira, WORD CodRede, WORD CodServico);
typedef LONG (__stdcall *PFN_ScopeGetParam)(LONG tipoParam, ptPARAM_COLETA lpParam);
typedef LONG (__stdcall *PFN_ScopeGetParamExt)(LONG tipoParam, ptPARAM_COLETA_EXT lpParam);
typedef LONG (__stdcall *PFN_ScopeResumeParam)(LONG codTipoColeta, LPSTR dados, WORD dadosParam, eACAO_APL acao);
typedef LONG (__stdcall *PFN_ScopeGetCupomEx)(WORD CabecLen, LPSTR Cabec, WORD CupomClienteLen, LPSTR CupomCliente, 
                                              WORD CupomLojaLen, LPSTR CupomLoja, WORD CupomReduzLen, LPSTR CupomReduz, 
                                              BYTE *NroLinhasReduz);
typedef LONG (__stdcall *PFN_ScopeSetAplColeta)(void);
typedef LONG (__stdcall *PFN_ScopeObtemHandle)(WORD Tipo);
typedef LONG (__stdcall *PFN_ScopeObtemCampoExt2)(LONG Handle, LONG Masc1, LONG Masc2, LONG Masc3, char FieldSeparator, LPSTR Buffer);

// === Implementação JSON Simples (caso não tenha nlohmann/json) ===
#ifndef NLOHMANN_JSON_HPP
class SimpleJson {
private:
    std::map<std::string, std::string> data;
    
public:
    SimpleJson() = default;
    
    void set(const std::string& key, const std::string& value) {
        data[key] = value;
    }
    
    void set(const std::string& key, int value) {
        data[key] = std::to_string(value);
    }
    
    void set(const std::string& key, bool value) {
        data[key] = value ? "true" : "false";
    }
    
    std::string get(const std::string& key, const std::string& defaultValue = "") const {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : defaultValue;
    }
    
    int getInt(const std::string& key, int defaultValue = 0) const {
        auto value = get(key);
        return value.empty() ? defaultValue : std::atoi(value.c_str());
    }
    
    double getDouble(const std::string& key, double defaultValue = 0.0) const {
        auto value = get(key);
        return value.empty() ? defaultValue : std::atof(value.c_str());
    }
    
    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto value = get(key);
        return value == "true";
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (const auto& pair : data) {
            if (!first) oss << ",";
            oss << "\"" << pair.first << "\":\"" << pair.second << "\"";
            first = false;
        }
        oss << "}";
        return oss.str();
    }
    
    static SimpleJson parse(const std::string& jsonStr) {
        SimpleJson json;
        // Parser básico - implementação simplificada
        size_t start = jsonStr.find('{');
        size_t end = jsonStr.rfind('}');
        
        if (start == std::string::npos || end == std::string::npos) {
            return json;
        }
        
        std::string content = jsonStr.substr(start + 1, end - start - 1);
        std::istringstream iss(content);
        std::string token;
        
        while (std::getline(iss, token, ',')) {
            size_t colonPos = token.find(':');
            if (colonPos != std::string::npos) {
                std::string key = token.substr(0, colonPos);
                std::string value = token.substr(colonPos + 1);
                
                // Remove aspas e espaços
                key.erase(0, key.find_first_not_of(" \t\""));
                key.erase(key.find_last_not_of(" \t\"") + 1);
                value.erase(0, value.find_first_not_of(" \t\""));
                value.erase(value.find_last_not_of(" \t\"") + 1);
                
                json.set(key, value);
            }
        }
        
        return json;
    }
};

using json = SimpleJson;
#endif

// === Classe ScopeWrapper ===
class ScopeWrapper {
private:
    HMODULE hScopeDLL = nullptr;
    bool isInitialized = false;
    
#if CPP11_AVAILABLE
    std::mutex transactionMutex;
    bool transactionActive = false;
#else
    // Fallback sem mutex para compiladores antigos
    bool transactionActive = false;
    CRITICAL_SECTION criticalSection;
#endif
    
    // Ponteiros para funções
    PFN_ScopeOpen pfnScopeOpen = nullptr;
    PFN_ScopeClose pfnScopeClose = nullptr;
    PFN_ScopeAbreSessaoTEF pfnScopeAbreSessaoTEF = nullptr;
    PFN_ScopeFechaSessaoTEF pfnScopeFechaSessaoTEF = nullptr;
    PFN_ScopeStatus pfnScopeStatus = nullptr;
    PFN_ScopeCompraCartaoCredito pfnScopeCompraCartaoCredito = nullptr;
    PFN_ScopeCompraCartaoDebito pfnScopeCompraCartaoDebito = nullptr;
    PFN_ScopeCarteiraVirtualEx2 pfnScopeCarteiraVirtualEx2 = nullptr;
    PFN_ScopeGetParam pfnScopeGetParam = nullptr;
    PFN_ScopeGetParamExt pfnScopeGetParamExt = nullptr;
    PFN_ScopeResumeParam pfnScopeResumeParam = nullptr;
    PFN_ScopeGetCupomEx pfnScopeGetCupomEx = nullptr;
    PFN_ScopeSetAplColeta pfnScopeSetAplColeta = nullptr;
    PFN_ScopeObtemHandle pfnScopeObtemHandle = nullptr;
    PFN_ScopeObtemCampoExt2 pfnScopeObtemCampoExt2 = nullptr;

public:
    ScopeWrapper() {
#if !CPP11_AVAILABLE
        InitializeCriticalSection(&criticalSection);
#endif
    }
    
    ~ScopeWrapper() {
        cleanup();
#if !CPP11_AVAILABLE
        DeleteCriticalSection(&criticalSection);
#endif
    }

    bool initialize(const std::string& dllPath = ".") {
        if (isInitialized) return true;

        try {
            // Definir diretório da DLL
            if (!SetDllDirectoryA(dllPath.c_str())) {
                logError("Erro ao definir diretório da DLL: " + dllPath);
            }
            
            // Carregar DLL
            hScopeDLL = LoadLibraryA("SCOPEAPI.DLL");
            
            if (!hScopeDLL) {
                DWORD error = GetLastError();
                logError("Erro ao carregar SCOPEAPI.DLL. Código: " + std::to_string(error));
                return false;
            }

            // Carregar todas as funções
            if (!loadFunctions()) {
                cleanup();
                return false;
            }

            isInitialized = true;
            logInfo("ScopeWrapper inicializado com sucesso");
            return true;

        } catch (const std::exception& e) {
            logError("Exceção durante inicialização: " + std::string(e.what()));
            cleanup();
            return false;
        }
    }

    json processTransaction(const json& request) {
#if CPP11_AVAILABLE
        std::lock_guard<std::mutex> lock(transactionMutex);
#else
        EnterCriticalSection(&criticalSection);
#endif
        
        if (transactionActive) {
#if !CPP11_AVAILABLE
            LeaveCriticalSection(&criticalSection);
#endif
            return createErrorResponse("Transação já em andamento");
        }

        if (!isInitialized) {
#if !CPP11_AVAILABLE
            LeaveCriticalSection(&criticalSection);
#endif
            return createErrorResponse("Wrapper não inicializado");
        }

        transactionActive = true;
        
        try {
            json response = executeTransaction(request);
            transactionActive = false;
#if !CPP11_AVAILABLE
            LeaveCriticalSection(&criticalSection);
#endif
            return response;
            
        } catch (const std::exception& e) {
            transactionActive = false;
#if !CPP11_AVAILABLE
            LeaveCriticalSection(&criticalSection);
#endif
            return createErrorResponse("Erro durante transação: " + std::string(e.what()));
        }
    }

    json getStatus() {
        json status;
        status.set("initialized", isInitialized);
        status.set("transactionActive", transactionActive);
        status.set("timestamp", getCurrentTimestamp());
        return status;
    }

private:
    bool loadFunctions() {
        // Macro para carregar funções com verificação de erro
        #define LOAD_FUNCTION(var, name) \
            var = (PFN_##name)GetProcAddress(hScopeDLL, #name); \
            if (!var) { \
                logError("Erro ao carregar função: " #name); \
                return false; \
            }

        LOAD_FUNCTION(pfnScopeOpen, ScopeOpen);
        LOAD_FUNCTION(pfnScopeClose, ScopeClose);
        LOAD_FUNCTION(pfnScopeAbreSessaoTEF, ScopeAbreSessaoTEF);
        LOAD_FUNCTION(pfnScopeFechaSessaoTEF, ScopeFechaSessaoTEF);
        LOAD_FUNCTION(pfnScopeStatus, ScopeStatus);
        LOAD_FUNCTION(pfnScopeCompraCartaoCredito, ScopeCompraCartaoCredito);
        LOAD_FUNCTION(pfnScopeCompraCartaoDebito, ScopeCompraCartaoDebito);
        LOAD_FUNCTION(pfnScopeCarteiraVirtualEx2, ScopeCarteiraVirtualEx2);
        LOAD_FUNCTION(pfnScopeGetParam, ScopeGetParam);
        LOAD_FUNCTION(pfnScopeGetParamExt, ScopeGetParamExt);
        LOAD_FUNCTION(pfnScopeResumeParam, ScopeResumeParam);
        LOAD_FUNCTION(pfnScopeGetCupomEx, ScopeGetCupomEx);
        LOAD_FUNCTION(pfnScopeSetAplColeta, ScopeSetAplColeta);
        LOAD_FUNCTION(pfnScopeObtemHandle, ScopeObtemHandle);
        LOAD_FUNCTION(pfnScopeObtemCampoExt2, ScopeObtemCampoExt2);

        #undef LOAD_FUNCTION
        return true;
    }

    json executeTransaction(const json& request) {
#if CPP11_AVAILABLE
        auto startTime = std::chrono::steady_clock::now();
#else
        DWORD startTime = GetTickCount();
#endif
        
        try {
            // Validar request
            if (!validateRequest(request)) {
                return createErrorResponse("Request inválido");
            }

            // Extrair dados do request
            std::string type = request.get("type");
            double amount = request.getDouble("amount");
            int installments = request.getInt("installments", 1);
            
            std::string empresa = request.get("empresa", "0001");
            std::string filial = request.get("filial", "0001");
            std::string pdv = request.get("pdv", "001");

            // Conectar ao Scope
            LONG ret = pfnScopeOpen(const_cast<char*>("2"),
                                   const_cast<char*>(empresa.c_str()),
                                   const_cast<char*>(filial.c_str()),
                                   const_cast<char*>(pdv.c_str()));
            
            if (ret != 0) {
                return createErrorResponse("Falha ao conectar ao Scope: " + std::to_string(ret));
            }

            // Configurar interface de coleta
            ret = pfnScopeSetAplColeta();
            if (ret != 0) {
                pfnScopeClose();
                return createErrorResponse("Falha ao configurar interface: " + std::to_string(ret));
            }

            // Abrir sessão TEF
            ret = pfnScopeAbreSessaoTEF();
            if (ret != 0) {
                pfnScopeClose();
                return createErrorResponse("Falha ao abrir sessão TEF: " + std::to_string(ret));
            }

            // Iniciar transação
            std::string amountStr = std::to_string(static_cast<int>(amount * 100));
            
            if (type == "credit") {
                ret = pfnScopeCompraCartaoCredito(const_cast<char*>(amountStr.c_str()), 
                                                 const_cast<char*>("0"));
            } else if (type == "debit") {
                ret = pfnScopeCompraCartaoDebito(const_cast<char*>(amountStr.c_str()));
            } else if (type == "pix") {
                ret = pfnScopeCarteiraVirtualEx2(const_cast<char*>(amountStr.c_str()), 
                                               0, 0, 169);
            } else {
                closeScope();
                return createErrorResponse("Tipo de transação inválido: " + type);
            }

            if (ret != 0) {
                closeScope();
                return createErrorResponse("Falha ao iniciar transação: " + std::to_string(ret));
            }

            // Processar transação
            json result = processTransactionLoop();
            
            // Fechar conexões
            closeScope();
            
            // Adicionar métricas
#if CPP11_AVAILABLE
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            result.set("processingTimeMs", static_cast<int>(duration.count()));
#else
            DWORD endTime = GetTickCount();
            DWORD duration = endTime - startTime;
            result.set("processingTimeMs", static_cast<int>(duration));
#endif
            
            return result;

        } catch (const std::exception& e) {
            closeScope();
            return createErrorResponse("Exceção durante transação: " + std::string(e.what()));
        }
    }

    json processTransactionLoop() {
        const int maxIterations = 240; // 2 minutos com intervalos de 500ms
        int iteration = 0;

        while (iteration < maxIterations) {
            LONG status = pfnScopeStatus();
            
            if (status == RCS_TRN_EM_ANDAMENTO) {
                // Transação em andamento
#if CPP11_AVAILABLE
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
#else
                Sleep(500); // Windows Sleep function
#endif
                iteration++;
                continue;
                
            } else if (status >= 0xFC00 && status <= 0xFCFF) {
                // Estados de coleta
                json collectionResult = handleCollectionState(status);
                if (collectionResult.get("error") != "") {
                    return collectionResult;
                }
                
            } else if (status == TC_IMPRIME_CUPOM) {
                // Imprimir cupom
                json cupomResult = handleCupomPrint();
                if (cupomResult.get("error") != "") {
                    return cupomResult;
                }
                
            } else if (status == 0) {
                // Sucesso
                return createSuccessResponse();
                
            } else {
                // Erro
                return createErrorResponse("Erro na transação: " + std::to_string(status));
            }
            
            iteration++;
        }

        return createErrorResponse("Timeout na transação");
    }

    json handleCollectionState(LONG status) {
        // Implementar lógica de coleta baseada no status
        // Por enquanto, simular aprovação automática
        
        if (status == TC_COLETA_EXT || status == TC_COLETA_DADO_ESPECIAL) {
            // Usar estrutura estendida
            stPARAM_COLETA_EXT coleta;
            std::memset(&coleta, 0, sizeof(coleta));
            
            LONG ret = pfnScopeGetParamExt(status, &coleta);
            if (ret == 0) {
                // Processar dados de coleta e enviar resposta automática
                pfnScopeResumeParam(status, const_cast<char*>(""), 0, APP_ACAO_CONTINUAR);
            }
        } else {
            // Usar estrutura normal
            stPARAM_COLETA coleta;
            std::memset(&coleta, 0, sizeof(coleta));
            
            LONG ret = pfnScopeGetParam(status, &coleta);
            if (ret == 0) {
                // Processar dados de coleta e enviar resposta automática
                pfnScopeResumeParam(status, const_cast<char*>(""), 0, APP_ACAO_CONTINUAR);
            }
        }

        return json(); // Continuar processamento
    }

    json handleCupomPrint() {
        char cabec[1024] = {0};
        char cpCliente[2048] = {0};
        char cpLoja[2048] = {0};
        char cpReduzido[512] = {0};
        BYTE nroLinhasReduz = 0;

        LONG ret = pfnScopeGetCupomEx(sizeof(cabec), cabec,
                                     sizeof(cpCliente), cpCliente,
                                     sizeof(cpLoja), cpLoja,
                                     sizeof(cpReduzido), cpReduzido, 
                                     &nroLinhasReduz);

        if (ret == 0) {
            // Cupom obtido com sucesso
            pfnScopeResumeParam(TC_IMPRIME_CUPOM, const_cast<char*>(""), 0, APP_ACAO_CONTINUAR);
            
            // Retornar dados do cupom (opcional)
            json cupomData;
            cupomData.set("header", std::string(cabec));
            cupomData.set("client", std::string(cpCliente));
            cupomData.set("store", std::string(cpLoja));
            if (nroLinhasReduz > 0) {
                cupomData.set("reduced", std::string(cpReduzido));
            }
            
            return cupomData;
        } else {
            pfnScopeResumeParam(TC_IMPRIME_CUPOM, const_cast<char*>(""), 0, APP_ACAO_ERRO_COLETA);
            return createErrorResponse("Erro ao obter cupom: " + std::to_string(ret));
        }
    }

    void closeScope() {
        try {
            BYTE desfez = 0;
            if (pfnScopeFechaSessaoTEF) {
                pfnScopeFechaSessaoTEF(1, &desfez); // Confirmar transação
            }
            if (pfnScopeClose) {
                pfnScopeClose();
            }
        } catch (...) {
            // Ignorar erros no fechamento
        }
    }

    bool validateRequest(const json& request) {
        std::string type = request.get("type");
        double amount = request.getDouble("amount");
        
        return !type.empty() && amount > 0;
    }

    json createSuccessResponse() {
        json response;
        response.set("status", "success");
        response.set("approved", true);
        response.set("timestamp", getCurrentTimestamp());
        
        // Tentar obter dados adicionais da transação
        try {
            if (pfnScopeObtemHandle) {
                LONG handle = pfnScopeObtemHandle(0);
                if (handle > 0xFFFF && pfnScopeObtemCampoExt2) {
                    char buffer[256] = {0};
                    
                    // NSU (exemplo de máscara - verificar documentação oficial)
                    if (pfnScopeObtemCampoExt2(handle, 0x00000001, 0, 0, ';', buffer) == 0) {
                        response.set("nsu", std::string(buffer));
                    }
                    
                    // Código de autorização (exemplo)
                    std::memset(buffer, 0, sizeof(buffer));
                    if (pfnScopeObtemCampoExt2(handle, 0x00000002, 0, 0, ';', buffer) == 0) {
                        response.set("authCode", std::string(buffer));
                    }
                }
            }
        } catch (...) {
            // Continuar mesmo se não conseguir obter dados adicionais
        }
        
        return response;
    }

    json createErrorResponse(const std::string& message) {
        json response;
        response.set("status", "error");
        response.set("approved", false);
        response.set("message", message);
        response.set("timestamp", getCurrentTimestamp());
        return response;
    }

    void cleanup() {
        if (hScopeDLL) {
            try {
                closeScope();
            } catch (...) {}
            
            FreeLibrary(hScopeDLL);
            hScopeDLL = nullptr;
        }
        isInitialized = false;
    }

    std::string getCurrentTimestamp() {
#if CPP11_AVAILABLE
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &time_t);
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
#else
        // Fallback para compiladores sem C++11
        time_t rawtime;
        struct tm timeinfo;
        char buffer[80];
        
        time(&rawtime);
        localtime_s(&timeinfo, &rawtime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        
        return std::string(buffer);
#endif
    }

    void logInfo(const std::string& message) {
        std::cerr << "[INFO] " << getCurrentTimestamp() << " - " << message << std::endl;
    }

    void logError(const std::string& message) {
        std::cerr << "[ERROR] " << getCurrentTimestamp() << " - " << message << std::endl;
    }
};

// === Função Principal ===
int main(int argc, char* argv[]) {
    ScopeWrapper wrapper;
    
    // Inicializar wrapper
    std::string dllPath = ".";
    if (argc > 1) {
        dllPath = argv[1];
    }
    
    if (!wrapper.initialize(dllPath)) {
        json errorResponse;
        errorResponse.set("status", "fatal_error");
        errorResponse.set("message", "Falha ao inicializar ScopeWrapper");
        std::cout << errorResponse.toString() << std::endl;
        return 1;
    }

    // Loop principal para processar comandos JSON
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            if (line.empty()) continue;
            
            if (line == "shutdown") {
                break;
            }
            
            json request = json::parse(line);
            json response;
            
            std::string command = request.get("command");
            
            if (command == "transaction") {
                // Para dados aninhados, seria necessário um parser mais complexo
                // Por simplicidade, assumindo que os dados estão no nível raiz
                response = wrapper.processTransaction(request);
            } else if (command == "status") {
                response = wrapper.getStatus();
            } else {
                response.set("status", "error");
                response.set("message", "Comando desconhecido: " + command);
            }
            
            std::cout << response.toString() << std::endl;
            
        } catch (const std::exception& e) {
            json errorResponse;
            errorResponse.set("status", "error");
            errorResponse.set("message", "Erro ao processar comando: " + std::string(e.what()));
            std::cout << errorResponse.toString() << std::endl;
        }
    }

    return 0;
}