#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#ifdef NLOHMANN_JSON_HPP
    #include <nlohmann/json.hpp>
    using json = nlohmann::json;
#else
    #include "simple_json.hpp"
    using json = SimpleJson;
#endif

// === Definições de Tipos da SCOPEAPI ===
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef long LONG;
typedef char* LPSTR;
typedef const char* LPCSTR;

// Estrutura de coleta conforme documentação oficial
typedef struct _stPARAM_COLETA {
    WORD Bandeira;          
    WORD FormatoDado;       
    WORD HabTeclas;         
    char MsgOp1[17];        
    char MsgOp2[17];        
    char MsgCl1[17];        
    char MsgCl2[17];        
    WORD TamMinDado;        
    WORD TamMaxDado;        
    WORD TamMinCartao;      
    WORD TamMaxCartao;      
    char CodSeguranca;      
    char Reservado[13];     
} stPARAM_COLETA, *ptPARAM_COLETA;

// Estrutura de coleta estendida
typedef struct _stPARAM_COLETA_EXT {
    BYTE FormatoDado;       
    BYTE HabTeclas;         
    char MsgOp1[17];        
    char MsgCl1[17];        
    WORD TamMinDado;        
    WORD TamMaxDado;        
    char Reservado[32];     
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

// === Utilitários Modernos ===
class Logger {
private:
    std::mutex logMutex;
    
    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
public:
    void info(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cerr << "[INFO] " << getCurrentTimestamp() << " - " << message << std::endl;
    }
    
    void error(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cerr << "[ERROR] " << getCurrentTimestamp() << " - " << message << std::endl;
    }
    
    void debug(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cerr << "[DEBUG] " << getCurrentTimestamp() << " - " << message << std::endl;
    }
};

// Instância global do logger
Logger logger;

// === Classe ScopeWrapper Moderna ===
class ScopeWrapper {
private:
    HMODULE hScopeDLL = nullptr;
    std::atomic<bool> isInitialized{false};
    std::atomic<bool> transactionActive{false};
    std::mutex transactionMutex;
    std::condition_variable transactionCV;
    
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
    ScopeWrapper() = default;
    
    ~ScopeWrapper() {
        cleanup();
    }

    bool initialize(const std::string& dllPath = ".") {
        if (isInitialized.load()) return true;

        std::lock_guard<std::mutex> lock(transactionMutex);
        
        try {
            logger.info("Inicializando ScopeWrapper...");
            
            // Definir diretório da DLL
            if (!SetDllDirectoryA(dllPath.c_str())) {
                logger.error("Erro ao definir diretório da DLL: " + dllPath);
            }
            
            // Carregar DLL
            hScopeDLL = LoadLibraryA("SCOPEAPI.DLL");
            
            if (!hScopeDLL) {
                DWORD error = GetLastError();
                logger.error("Erro ao carregar SCOPEAPI.DLL. Código: " + std::to_string(error));
                return false;
            }

            logger.info("SCOPEAPI.DLL carregada com sucesso");

            // Carregar todas as funções
            if (!loadFunctions()) {
                cleanup();
                return false;
            }

            isInitialized.store(true);
            logger.info("ScopeWrapper inicializado com sucesso");
            return true;

        } catch (const std::exception& e) {
            logger.error("Exceção durante inicialização: " + std::string(e.what()));
            cleanup();
            return false;
        }
    }

    json processTransaction(const json& request) {
        std::unique_lock<std::mutex> lock(transactionMutex);
        
        // Aguardar se há transação ativa
        transactionCV.wait(lock, [this] { return !transactionActive.load(); });
        
        if (!isInitialized.load()) {
            return createErrorResponse("Wrapper não inicializado");
        }

        transactionActive.store(true);
        
        try {
            auto result = executeTransaction(request);
            transactionActive.store(false);
            transactionCV.notify_all();
            return result;
            
        } catch (const std::exception& e) {
            transactionActive.store(false);
            transactionCV.notify_all();
            return createErrorResponse("Erro durante transação: " + std::string(e.what()));
        }
    }

    json getStatus() const {
        json status;
        status.set("initialized", isInitialized.load());
        status.set("transactionActive", transactionActive.load());
        status.set("timestamp", getCurrentTimestamp());
        status.set("version", "modern-mingw-w64");
        status.set("threading", "std::mutex");
        return status;
    }

private:
    bool loadFunctions() {
        // Macro para carregar funções com logging detalhado
        #define LOAD_FUNCTION(var, name) \
            var = (PFN_##name)GetProcAddress(hScopeDLL, #name); \
            if (!var) { \
                logger.error("Erro ao carregar função: " #name); \
                return false; \
            } else { \
                logger.debug("Função carregada: " #name); \
            }

        logger.info("Carregando funções da SCOPEAPI...");

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
        
        logger.info("Todas as funções carregadas com sucesso");
        return true;
    }

    json executeTransaction(const json& request) {
        auto startTime = std::chrono::steady_clock::now();
        
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

            logger.info("Iniciando transação - Tipo: " + type + ", Valor: " + std::to_string(amount));

            // Conectar ao Scope
            LONG ret = pfnScopeOpen(const_cast<char*>("2"),
                                   const_cast<char*>(empresa.c_str()),
                                   const_cast<char*>(filial.c_str()),
                                   const_cast<char*>(pdv.c_str()));
            
            if (ret != 0) {
                return createErrorResponse("Falha ao conectar ao Scope: " + std::to_string(ret));
            }

            logger.info("Conectado ao Scope com sucesso");

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

            logger.info("Sessão TEF aberta com sucesso");

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

            logger.info("Transação iniciada, processando...");

            // Processar transação
            auto result = processTransactionLoop();
            
            // Fechar conexões
            closeScope();
            
            // Adicionar métricas
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            result.set("processingTimeMs", static_cast<int>(duration.count()));
            
            logger.info("Transação concluída em " + std::to_string(duration.count()) + "ms");
            return result;

        } catch (const std::exception& e) {
            closeScope();
            return createErrorResponse("Exceção durante transação: " + std::string(e.what()));
        }
    }

    json processTransactionLoop() {
        const auto maxDuration = std::chrono::minutes(2);
        const auto pollInterval = std::chrono::milliseconds(500);
        auto startTime = std::chrono::steady_clock::now();
        int iteration = 0;

        while (std::chrono::steady_clock::now() - startTime < maxDuration) {
            LONG status = pfnScopeStatus();
            
            logger.debug("Status da transação: " + std::to_string(status) + " (iteração " + std::to_string(iteration) + ")");
            
            if (status == RCS_TRN_EM_ANDAMENTO) {
                // Transação em andamento
                std::this_thread::sleep_for(pollInterval);
                iteration++;
                continue;
                
            } else if (status >= 0xFC00 && status <= 0xFCFF) {
                // Estados de coleta
                auto collectionResult = handleCollectionState(status);
                if (collectionResult.get("error") != "") {
                    return collectionResult;
                }
                
            } else if (status == TC_IMPRIME_CUPOM) {
                // Imprimir cupom
                auto cupomResult = handleCupomPrint();
                if (cupomResult.get("error") != "") {
                    return cupomResult;
                }
                
            } else if (status == 0) {
                // Sucesso
                logger.info("Transação aprovada com sucesso");
                return createSuccessResponse();
                
            } else {
                // Erro
                logger.error("Erro na transação: " + std::to_string(status));
                return createErrorResponse("Erro na transação: " + std::to_string(status));
            }
            
            iteration++;
        }

        return createErrorResponse("Timeout na transação");
    }

    json handleCollectionState(LONG status) {
        logger.info("Processando estado de coleta: " + std::to_string(status));
        
        try {
            if (status == TC_COLETA_EXT || status == TC_COLETA_DADO_ESPECIAL) {
                // Usar estrutura estendida
                stPARAM_COLETA_EXT coleta;
                std::memset(&coleta, 0, sizeof(coleta));
                
                LONG ret = pfnScopeGetParamExt(status, &coleta);
                if (ret == 0) {
                    logger.info("Coleta estendida - Mensagem Op: " + std::string(coleta.MsgOp1));
                    logger.info("Coleta estendida - Mensagem Cl: " + std::string(coleta.MsgCl1));
                    
                    // Processar dados de coleta e enviar resposta automática
                    pfnScopeResumeParam(status, const_cast<char*>(""), 0, APP_ACAO_CONTINUAR);
                } else {
                    logger.error("Erro ao obter parâmetros de coleta estendida: " + std::to_string(ret));
                    return createErrorResponse("Erro na coleta estendida: " + std::to_string(ret));
                }
            } else {
                // Usar estrutura normal
                stPARAM_COLETA coleta;
                std::memset(&coleta, 0, sizeof(coleta));
                
                LONG ret = pfnScopeGetParam(status, &coleta);
                if (ret == 0) {
                    logger.info("Coleta normal - Mensagem Op1: " + std::string(coleta.MsgOp1));
                    logger.info("Coleta normal - Mensagem Op2: " + std::string(coleta.MsgOp2));
                    logger.info("Coleta normal - Mensagem Cl1: " + std::string(coleta.MsgCl1));
                    logger.info("Coleta normal - Mensagem Cl2: " + std::string(coleta.MsgCl2));
                    
                    // Processar dados de coleta e enviar resposta automática
                    pfnScopeResumeParam(status, const_cast<char*>(""), 0, APP_ACAO_CONTINUAR);
                } else {
                    logger.error("Erro ao obter parâmetros de coleta: " + std::to_string(ret));
                    return createErrorResponse("Erro na coleta: " + std::to_string(ret));
                }
            }
        } catch (const std::exception& e) {
            logger.error("Exceção durante coleta: " + std::string(e.what()));
            return createErrorResponse("Exceção durante coleta: " + std::string(e.what()));
        }

        return json(); // Continuar processamento
    }

    json handleCupomPrint() {
        logger.info("Processando impressão de cupom");
        
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
            logger.info("Cupom obtido com sucesso");
            
            // Cupom obtido com sucesso
            pfnScopeResumeParam(TC_IMPRIME_CUPOM, const_cast<char*>(""), 0, APP_ACAO_CONTINUAR);
            
            // Retornar dados do cupom
            json cupomData;
            cupomData.set("header", std::string(cabec));
            cupomData.set("client", std::string(cpCliente));
            cupomData.set("store", std::string(cpLoja));
            if (nroLinhasReduz > 0) {
                cupomData.set("reduced", std::string(cpReduzido));
            }
            
            return cupomData;
        } else {
            logger.error("Erro ao obter cupom: " + std::to_string(ret));
            pfnScopeResumeParam(TC_IMPRIME_CUPOM, const_cast<char*>(""), 0, APP_ACAO_ERRO_COLETA);
            return createErrorResponse("Erro ao obter cupom: " + std::to_string(ret));
        }
    }

    void closeScope() {
        try {
            BYTE desfez = 0;
            if (pfnScopeFechaSessaoTEF) {
                LONG ret = pfnScopeFechaSessaoTEF(1, &desfez); // Confirmar transação
                logger.info("Sessão TEF fechada. Resultado: " + std::to_string(ret));
            }
            if (pfnScopeClose) {
                LONG ret = pfnScopeClose();
                logger.info("Conexão Scope fechada. Resultado: " + std::to_string(ret));
            }
        } catch (const std::exception& e) {
            logger.error("Erro ao fechar conexões Scope: " + std::string(e.what()));
        }
    }

    bool validateRequest(const json& request) const {
        std::string type = request.get("type");
        double amount = request.getDouble("amount");
        
        if (type.empty()) {
            logger.error("Tipo de transação não informado");
            return false;
        }
        
        if (amount <= 0) {
            logger.error("Valor inválido: " + std::to_string(amount));
            return false;
        }
        
        if (type != "credit" && type != "debit" && type != "pix") {
            logger.error("Tipo de transação inválido: " + type);
            return false;
        }
        
        return true;
    }

    json createSuccessResponse() const {
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
                        logger.info("NSU obtido: " + std::string(buffer));
                    }
                    
                    // Código de autorização (exemplo)
                    std::memset(buffer, 0, sizeof(buffer));
                    if (pfnScopeObtemCampoExt2(handle, 0x00000002, 0, 0, ';', buffer) == 0) {
                        response.set("authCode", std::string(buffer));
                        logger.info("Código de autorização obtido: " + std::string(buffer));
                    }
                }
            }
        } catch (const std::exception& e) {
            logger.error("Erro ao obter dados adicionais da transação: " + std::string(e.what()));
        }
        
        return response;
    }

    json createErrorResponse(const std::string& message) const {
        json response;
        response.set("status", "error");
        response.set("approved", false);
        response.set("message", message);
        response.set("timestamp", getCurrentTimestamp());
        logger.error(message);
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
        isInitialized.store(false);
    }

    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
};

// === Função Principal ===
int main(int argc, char* argv[]) {
    ScopeWrapper wrapper;
    
    logger.info("Iniciando ScopeWrapper moderno (MinGW-w64 + C++17)...");
    
    // Inicializar wrapper
    std::string dllPath = ".";
    if (argc > 1) {
        dllPath = argv[1];
    }
    
    logger.info("Caminho da DLL: " + dllPath);
    
    if (!wrapper.initialize(dllPath)) {
        json errorResponse;
        errorResponse.set("status", "fatal_error");
        errorResponse.set("message", "Falha ao inicializar ScopeWrapper");
        std::cout << errorResponse.toString() << std::endl;
        return 1;
    }

    logger.info("ScopeWrapper inicializado. Aguardando comandos...");

    // Loop principal para processar comandos JSON
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            if (line.empty()) continue;
            
            logger.debug("Comando recebido: " + line);
            
            if (line == "shutdown") {
                logger.info("Shutdown solicitado");
                break;
            }
            
            json request = json::parse(line);
            json response;
            
            std::string command = request.get("command");
            
            if (command == "transaction") {
                logger.info("Processando transação...");
                response = wrapper.processTransaction(request);
            } else if (command == "status") {
                logger.debug("Verificando status...");
                response = wrapper.getStatus();
            } else {
                logger.error("Comando desconhecido: " + command);
                response.set("status", "error");
                response.set("message", "Comando desconhecido: " + command);
            }
            
            std::cout << response.toString() << std::endl;
            logger.debug("Resposta enviada");
            
        } catch (const std::exception& e) {
            logger.error("Erro ao processar comando: " + std::string(e.what()));
            json errorResponse;
            errorResponse.set("status", "error");
            errorResponse.set("message", "Erro ao processar comando: " + std::string(e.what()));
            std::cout << errorResponse.toString() << std::endl;
        }
    }

    logger.info("ScopeWrapper finalizando...");
    return 0;
}