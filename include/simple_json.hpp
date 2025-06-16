#ifndef SIMPLE_JSON_HPP
#define SIMPLE_JSON_HPP

#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <iostream>
#include <iomanip>

class SimpleJson {
private:
    std::map<std::string, std::string> data;
    
    // Remove aspas e espaços de uma string
    std::string trim(const std::string& str) const {
        size_t start = str.find_first_not_of(" \t\n\r\"");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t\n\r\"");
        return str.substr(start, end - start + 1);
    }
    
    // Escape caracteres especiais para JSON
    std::string escape(const std::string& str) const {
        std::string escaped;
        escaped.reserve(str.length() * 2); // Reserve space to avoid reallocations
        
        for (char c : str) {
            switch (c) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                default: escaped += c; break;
            }
        }
        return escaped;
    }
    
public:
    // Construtor padrão
    SimpleJson() = default;
    
    // Construtor de cópia
    SimpleJson(const SimpleJson& other) : data(other.data) {}
    
    // Construtor de movimento
    SimpleJson(SimpleJson&& other) noexcept : data(std::move(other.data)) {}
    
    // Operador de atribuição
    SimpleJson& operator=(const SimpleJson& other) {
        if (this != &other) {
            data = other.data;
        }
        return *this;
    }
    
    // Operador de atribuição de movimento
    SimpleJson& operator=(SimpleJson&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
        }
        return *this;
    }
    
    // Destructor
    ~SimpleJson() = default;
    
    // Setters com diferentes tipos
    void set(const std::string& key, const std::string& value) {
        data[key] = value;
    }
    
    void set(const std::string& key, const char* value) {
        data[key] = std::string(value ? value : "");
    }
    
    void set(const std::string& key, int value) {
        data[key] = std::to_string(value);
    }
    
    void set(const std::string& key, long value) {
        data[key] = std::to_string(value);
    }
    
    void set(const std::string& key, long long value) {
        data[key] = std::to_string(value);
    }
    
    void set(const std::string& key, double value) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value;
        data[key] = oss.str();
    }
    
    void set(const std::string& key, float value) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value;
        data[key] = oss.str();
    }
    
    void set(const std::string& key, bool value) {
        data[key] = value ? "true" : "false";
    }
    
    // Getters com valores padrão
    std::string get(const std::string& key, const std::string& defaultValue = "") const {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : defaultValue;
    }
    
    int getInt(const std::string& key, int defaultValue = 0) const {
        auto value = get(key);
        if (value.empty()) return defaultValue;
        
        try {
            return std::stoi(value);
        } catch (const std::exception&) {
            return defaultValue;
        }
    }
    
    long getLong(const std::string& key, long defaultValue = 0) const {
        auto value = get(key);
        if (value.empty()) return defaultValue;
        
        try {
            return std::stol(value);
        } catch (const std::exception&) {
            return defaultValue;
        }
    }
    
    long long getLongLong(const std::string& key, long long defaultValue = 0) const {
        auto value = get(key);
        if (value.empty()) return defaultValue;
        
        try {
            return std::stoll(value);
        } catch (const std::exception&) {
            return defaultValue;
        }
    }
    
    double getDouble(const std::string& key, double defaultValue = 0.0) const {
        auto value = get(key);
        if (value.empty()) return defaultValue;
        
        try {
            return std::stod(value);
        } catch (const std::exception&) {
            return defaultValue;
        }
    }
    
    float getFloat(const std::string& key, float defaultValue = 0.0f) const {
        auto value = get(key);
        if (value.empty()) return defaultValue;
        
        try {
            return std::stof(value);
        } catch (const std::exception&) {
            return defaultValue;
        }
    }
    
    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto value = get(key);
        if (value.empty()) return defaultValue;
        
        // Normalizar para lowercase para comparação
        std::string lowerValue = value;
        for (char& c : lowerValue) {
            c = static_cast<char>(std::tolower(c));
        }
        
        return (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes");
    }
    
    // Métodos de verificação
    bool contains(const std::string& key) const {
        return data.find(key) != data.end();
    }
    
    bool empty() const {
        return data.empty();
    }
    
    size_t size() const {
        return data.size();
    }
    
    // Limpar todos os dados
    void clear() {
        data.clear();
    }
    
    // Remover uma chave específica
    bool remove(const std::string& key) {
        return data.erase(key) > 0;
    }
    
    // Obter todas as chaves
    std::vector<std::string> keys() const {
        std::vector<std::string> result;
        result.reserve(data.size());
        for (const auto& pair : data) {
            result.push_back(pair.first);
        }
        return result;
    }
    
    // Serializar para string JSON
    std::string toString() const {
        if (data.empty()) {
            return "{}";
        }
        
        std::ostringstream oss;
        oss << "{";
        
        bool first = true;
        for (const auto& pair : data) {
            if (!first) {
                oss << ",";
            }
            
            // Determinar se o valor precisa de aspas
            std::string value = pair.second;
            bool needsQuotes = true;
            
            // Números e booleanos não precisam de aspas
            if (value == "true" || value == "false" || value == "null") {
                needsQuotes = false;
            } else {
                // Verificar se é número
                try {
                    std::stod(value);
                    needsQuotes = false;
                } catch (const std::exception&) {
                    needsQuotes = true;
                }
            }
            
            oss << "\"" << escape(pair.first) << "\":";
            if (needsQuotes) {
                oss << "\"" << escape(value) << "\"";
            } else {
                oss << value;
            }
            
            first = false;
        }
        
        oss << "}";
        return oss.str();
    }
    
    // Parser JSON básico melhorado
    static SimpleJson parse(const std::string& jsonStr) {
        SimpleJson json;
        
        if (jsonStr.empty()) {
            return json;
        }
        
        // Encontrar início e fim do objeto JSON
        size_t start = jsonStr.find('{');
        size_t end = jsonStr.rfind('}');
        
        if (start == std::string::npos || end == std::string::npos || start >= end) {
            return json;
        }
        
        std::string content = jsonStr.substr(start + 1, end - start - 1);
        
        // Parser simples baseado em vírgulas
        std::vector<std::string> pairs;
        std::string current;
        bool inQuotes = false;
        int braceLevel = 0;
        
        for (size_t i = 0; i < content.length(); ++i) {
            char c = content[i];
            
            if (c == '"' && (i == 0 || content[i-1] != '\\')) {
                inQuotes = !inQuotes;
            } else if (!inQuotes) {
                if (c == '{' || c == '[') {
                    braceLevel++;
                } else if (c == '}' || c == ']') {
                    braceLevel--;
                } else if (c == ',' && braceLevel == 0) {
                    if (!current.empty()) {
                        pairs.push_back(current);
                    }
                    current.clear();
                    continue;
                }
            }
            
            current += c;
        }
        
        // Adicionar último par se não estiver vazio
        if (!current.empty()) {
            pairs.push_back(current);
        }
        
        // Processar cada par chave:valor
        for (const std::string& pair : pairs) {
            size_t colonPos = pair.find(':');
            if (colonPos == std::string::npos) continue;
            
            std::string key = pair.substr(0, colonPos);
            std::string value = pair.substr(colonPos + 1);
            
            key = json.trim(key);
            value = json.trim(value);
            
            if (!key.empty()) {
                json.set(key, value);
            }
        }
        
        return json;
    }
    
    // Operador de indexação para facilitar acesso
    std::string operator[](const std::string& key) const {
        return get(key);
    }
    
    // Método para merge com outro objeto JSON
    void merge(const SimpleJson& other) {
        for (const auto& pair : other.data) {
            data[pair.first] = pair.second;
        }
    }
    
    // Debug: imprimir todas as chaves e valores
    void debug() const {
        std::cerr << "SimpleJson Debug (" << data.size() << " items):" << std::endl;
        for (const auto& pair : data) {
            std::cerr << "  \"" << pair.first << "\" = \"" << pair.second << "\"" << std::endl;
        }
    }
    
    // Método para pretty print
    std::string toPrettyString(int indent = 2) const {
        if (data.empty()) {
            return "{}";
        }
        
        std::ostringstream oss;
        std::string indentStr(indent, ' ');
        
        oss << "{\n";
        
        bool first = true;
        for (const auto& pair : data) {
            if (!first) {
                oss << ",\n";
            }
            
            std::string value = pair.second;
            bool needsQuotes = true;
            
            if (value == "true" || value == "false" || value == "null") {
                needsQuotes = false;
            } else {
                try {
                    std::stod(value);
                    needsQuotes = false;
                } catch (const std::exception&) {
                    needsQuotes = true;
                }
            }
            
            oss << indentStr << "\"" << escape(pair.first) << "\": ";
            if (needsQuotes) {
                oss << "\"" << escape(value) << "\"";
            } else {
                oss << value;
            }
            
            first = false;
        }
        
        oss << "\n}";
        return oss.str();
    }
};

#endif // SIMPLE_JSON_HPP