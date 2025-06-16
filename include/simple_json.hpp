#ifndef SIMPLE_JSON_HPP
#define SIMPLE_JSON_HPP

#include <string>
#include <map>
#include <sstream>
#include <vector>

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
        for (char c : str) {
            switch (c) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += c; break;
            }
        }
        return escaped;
    }
    
public:
    SimpleJson() = default;
    
    // Construtor de cópia
    SimpleJson(const SimpleJson& other) : data(other.data) {}
    
    // Operador de atribuição
    SimpleJson& operator=(const SimpleJson& other) {
        if (this != &other) {
            data = other.data;
        }
        return *this;
    }
    
    // Setters
    void set(const std::string& key, const std::string& value) {
        data[key] = value;
    }
    
    void set(const std::string& key, int value) {
        data[key] = std::to_string(value);
    }
    
    void set(const std::string& key, long value) {
        data[key] = std::to_string(value);
    }
    
    void set(const std::string& key, double value) {
        std::ostringstream oss;
        oss << value;
        data[key] = oss.str();
    }
    
    void set(const std::string& key, bool value) {
        data[key] = value ? "true" : "false";
    }
    
    // Getters
    std::string get(const std::string& key, const std::string& defaultValue = "") const {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : defaultValue;
    }
    
    int getInt(const std::string& key, int defaultValue = 0) const {
        auto value = get(key);
        if (value.empty()) return defaultValue;
        
        try {
            return std::stoi(value);
        } catch (...) {
            return defaultValue;
        }
    }
    
    long getLong(const std::string& key, long defaultValue = 0) const {
        auto value = get(key);
        if (value.empty()) return defaultValue;
        
        try {
            return std::stol(value);
        } catch (...) {
            return defaultValue;
        }
    }
    
    double getDouble(const std::string& key, double defaultValue = 0.0) const {
        auto value = get(key);
        if (value.empty()) return defaultValue;
        
        try {
            return std::stod(value);
        } catch (...) {
            return defaultValue;
        }
    }
    
    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto value = get(key);
        return value == "true";
    }
    
    // Verificar se existe uma chave
    bool contains(const std::string& key) const {
        return data.find(key) != data.end();
    }
    
    // Verificar se está vazio
    bool empty() const {
        return data.empty();
    }
    
    // Limpar todos os dados
    void clear() {
        data.clear();
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
            if (!first) oss << ",";
            
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
                } catch (...) {
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
    
    // Parser JSON básico
    static SimpleJson parse(const std::string& jsonStr) {
        SimpleJson json;
        
        if (jsonStr.empty()) {
            return json;
        }
        
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
            } else if (c == '{' || c == '[') {
                braceLevel++;
            } else if (c == '}' || c == ']') {
                braceLevel--;
            } else if (c == ',' && !inQuotes && braceLevel == 0) {
                pairs.push_back(current);
                current.clear();
                continue;
            }
            
            current += c;
        }
        
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
    
    // Debug: imprimir todas as chaves e valores
    void debug() const {
        std::cerr << "SimpleJson Debug:" << std::endl;
        for (const auto& pair : data) {
            std::cerr << "  \"" << pair.first << "\" = \"" << pair.second << "\"" << std::endl;
        }
    }
};

#endif // SIMPLE_JSON_HPP