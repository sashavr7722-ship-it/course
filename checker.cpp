#include "checker.h"
#include <regex>
#include <fstream>
#include <algorithm>
#include <iostream>

std::vector<std::tuple<std::string, std::string, int>> findPersonalDataWithWeight(const std::string& text) {
    std::vector<std::tuple<std::string, std::string, int>> results;
    
    try {
        std::vector<std::pair<std::regex, std::pair<std::string, int>>> patterns = {
            {std::regex(R"(\d{4}\s?\d{6})"), {"Номер паспорта (серия+номер)", 10}},      
            {std::regex(R"(\d{2}\s?\d{2}\s?\d{6})"), {"Паспорт (старый формат)", 10}},     
            {std::regex(R"(\d{10})"), {"ИНН (10 цифр)", 8}},                             
            {std::regex(R"(\d{12})"), {"ИНН (12 цифр)", 8}},                               
            {std::regex(R"(\d{3}-\d{3}-\d{3}\s?\d{2})"), {"СНИЛС", 7}},                     
            {std::regex(R"(\d{11})"), {"СНИЛС (слитно)", 7}},                                
            {std::regex(R"(7\d{10})"), {"Телефон РФ (10 цифр после 7)", 6}},               
            {std::regex(R"(8\d{10})"), {"Телефон РФ (8)", 6}},                          
            {std::regex(R"(\+7\d{10})"), {"Телефон РФ (+7)", 6}},                          
            {std::regex(R"(\d{11})"), {"Телефон (11 цифр)", 6}},                          
            {std::regex(R"(\d{16})"), {"Номер карты (16 цифр)", 10}},                       
            {std::regex(R"(\d{4}\d{4}\d{4}\d{4})"), {"Номер карты (слитно)", 10}},           
            {std::regex(R"(\d{2}\.\d{2}\.\d{4})"), {"Дата (ДД.ММ.ГГГГ)", 4}},                
            {std::regex(R"(\d{4}-\d{2}-\d{2})"), {"Дата (ГГГГ-ММ-ДД)", 4}},                  
            {std::regex(R"(\d{2}\.\d{2}\.\d{2})"), {"Дата (ДД.ММ.ГГ)", 4}},                 
            {std::regex(R"(\d{8})"), {"Дата (8 цифр подряд)", 4}},                          
            {std::regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"), {"Email", 3}}, 
            {std::regex(R"(\d{6})"), {"Почтовый индекс", 2}},                                
            {std::regex(R"(паспорт\s*\d{4}\s*\d{6})"), {"Паспорт (текст)", 10}},             
            {std::regex(R"(passport\s*\d{4}\s*\d{6})"), {"Passport (text)", 10}},            
            {std::regex(R"(серия\s*\d{4}\s*номер\s*\d{6})"), {"Серия/номер паспорта", 10}},  
            {std::regex(R"([A-Za-z]+\d{4,})"), {"Логин: имя+много цифр", 3}},               
            {std::regex(R"(\d{4,}[A-Za-z]+)"), {"Логин: цифры+имя", 3}},                   
            {std::regex(R"([A-Za-z]+\d{2}\.\d{2}\.\d{4})"), {"Логин: имя+дата", 8}},        
            {std::regex(R"(password|parol|пaроль)"), {"Упоминание пароля", 5}},             
            {std::regex(R"(login|логін|логин)"), {"Упоминание логина", 3}},                
            {std::regex(R"(admin|root|user)"), {"Стандартный логин", 4}},                   
        };
        
        for (const auto& pattern : patterns) {
            std::smatch match;
            std::string::const_iterator searchStart(text.cbegin());
            while (std::regex_search(searchStart, text.cend(), match, pattern.first)) {
                results.push_back(std::make_tuple(pattern.second.first, match.str(), pattern.second.second));
                searchStart = match.suffix().first;
            }
        }
    } catch (const std::regex_error& e) {
        results.push_back(std::make_tuple("Ошибка", e.what(), 0));
    }
    
    return results;
}

int calculateSecurityLevel(const std::vector<std::tuple<std::string, std::string, int>>& findings) {
    int total_weight = 0;
    for (const auto& finding : findings) {
        total_weight += std::get<2>(finding);
    }
    return total_weight;
}

std::pair<std::vector<std::tuple<std::string, std::string, int>>, int> checkSingleLogin(const std::string& login) {
    std::vector<std::tuple<std::string, std::string, int>> results;
    
    results.push_back(std::make_tuple("Проверка логина", login, 0));
    
    auto personal_data = findPersonalDataWithWeight(login);
    
    if (!personal_data.empty()) {
        results.push_back(std::make_tuple("ВНИМАНИЕ", "Логин содержит персональные данные:", 0));
        results.insert(results.end(), personal_data.begin(), personal_data.end());
    } else {
        results.push_back(std::make_tuple("Информация", "Логин не содержит явных персональных данных", 0));
    }
    
    int total_weight = 0;
    for (const auto& result : results) {
        total_weight += std::get<2>(result);
    }
    
    return {results, total_weight};
}

std::pair<std::vector<std::tuple<std::string, std::string, int>>, int> checkAllLoginsInFile(const std::string& filename) {
    std::vector<std::tuple<std::string, std::string, int>> results;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        results.push_back(std::make_tuple("Ошибка", "Не удалось открыть файл: " + filename, 0));
        return {results, 0};
    }
    
    std::string line;
    int line_number = 0;
    int total_weight = 0;
    
    results.push_back(std::make_tuple("Информация", "ПРОВЕРКА ВСЕХ ЛОГИНОВ ИЗ ФАЙЛА", 0));
    results.push_back(std::make_tuple("Информация", "Файл: " + filename, 0));
    results.push_back(std::make_tuple("Информация", "", 0));
    
    while (std::getline(file, line)) {
        line_number++;
        
        if (line.empty()) continue;
        
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        
        results.push_back(std::make_tuple("Логин #" + std::to_string(line_number), line, 0));
        
        auto personal_data = findPersonalDataWithWeight(line);
        
        if (!personal_data.empty()) {
            results.push_back(std::make_tuple("⚠️ ВНИМАНИЕ", "Логин содержит персональные данные:", 0));
            
            int login_weight = 0;
            for (const auto& data : personal_data) {
                results.push_back(data);
                login_weight += std::get<2>(data);
            }
            total_weight += login_weight;
            
            std::string risk_level;
            if (login_weight >= 20) risk_level = "🔴 ВЫСОКИЙ РИСК";
            else if (login_weight >= 10) risk_level = "🟡 СРЕДНИЙ РИСК";
            else if (login_weight > 0) risk_level = "🟢 НИЗКИЙ РИСК";
            
            results.push_back(std::make_tuple("Оценка", risk_level + " (вес: " + std::to_string(login_weight) + ")", 0));
        } else {
            results.push_back(std::make_tuple("✅ Безопасно", "Персональные данные не обнаружены", 0));
        }
        
        results.push_back(std::make_tuple("", "---", 0));
    }
    
    file.close();
    
    results.push_back(std::make_tuple("ИТОГОВАЯ СТАТИСТИКА", "", 0));
    results.push_back(std::make_tuple("Всего проверено", "логинов: " + std::to_string(line_number), 0));
    results.push_back(std::make_tuple("Общий вес угроз", std::to_string(total_weight), 0));
    
    if (total_weight == 0) {
        results.push_back(std::make_tuple("✅ ИТОГО", "Файл безопасен", 0));
    } else if (total_weight < 50) {
        results.push_back(std::make_tuple("🟡 ИТОГО", "Файл содержит умеренные риски", 0));
    } else {
        results.push_back(std::make_tuple("🔴 ИТОГО", "Файл содержит критический уровень риска!", 0));
    }
    
    return {results, total_weight};
}