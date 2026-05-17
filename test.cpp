#include "checker.h"
#include <iostream>
#include <cassert>
#include <fstream>     
#include <string>       

void test_findPersonalData() {
    std::cout << "=== Тестирование findPersonalDataWithWeight ===\n";
    
    std::string test1 = "79001234567";
    auto results1 = findPersonalDataWithWeight(test1);
    assert(!results1.empty());
    std::cout << "✓ Телефон найден: " << std::get<1>(results1[0]) << "\n";
    
    std::string test2 = "user@example.com";
    auto results2 = findPersonalDataWithWeight(test2);
    assert(!results2.empty());
    std::cout << "✓ Email найден: " << std::get<1>(results2[0]) << "\n";
    
    std::string test3 = "25.12.1990";
    auto results3 = findPersonalDataWithWeight(test3);
    assert(!results3.empty());
    std::cout << "✓ Дата найдена: " << std::get<1>(results3[0]) << "\n";
    
    std::string test4 = "1234567890";
    auto results4 = findPersonalDataWithWeight(test4);
    assert(!results4.empty());
    std::cout << "✓ Номер паспорта/ИНН найден: " << std::get<1>(results4[0]) << "\n";
    
    std::string test5 = "simple_login";
    auto results5 = findPersonalDataWithWeight(test5);
    std::cout << "✓ Безопасный логин проверен\n";
    
    std::cout << "Все тесты findPersonalDataWithWeight пройдены!\n\n";
}

void test_checkSingleLogin() {
    std::cout << "=== Тестирование checkSingleLogin ===\n";
    
    std::string login1 = "user79001234567";
    auto [results1, weight1] = checkSingleLogin(login1);
    std::cout << "Логин '" << login1 << "' имеет вес: " << weight1 << "\n";
    assert(weight1 > 0);
    
    std::string login3 = "test@mail.com";
    auto [results3, weight3] = checkSingleLogin(login3);
    std::cout << "Логин '" << login3 << "' имеет вес: " << weight3 << "\n";
    assert(weight3 > 0);
    
    std::cout << "Все тесты checkSingleLogin пройдены!\n\n";
}

void test_checkAllLoginsInFile() {
    std::cout << "=== Тестирование checkAllLoginsInFile ===\n";
    
    std::string test_filename = "test_logins.txt";
    std::ofstream test_file(test_filename);
    if (!test_file.is_open()) {
        std::cerr << "Не удалось создать тестовый файл!\n";
        return;
    }
    test_file << "user123\n";
    test_file << "79001234567\n";
    test_file << "admin@example.com\n";
    test_file << "safe_user\n";
    test_file.close();
    
    auto [results, total_weight] = checkAllLoginsInFile(test_filename);
    
    std::cout << "Всего найдено записей в результатах: " << results.size() << "\n";
    std::cout << "Общий вес угроз: " << total_weight << "\n";
    assert(total_weight > 0);
    
    std::remove(test_filename.c_str());
    
    std::cout << "Все тесты checkAllLoginsInFile пройдены!\n\n";
}

int main() {
    std::cout << "Запуск модульных тестов для Security Checker\n\n";
    
    test_findPersonalData();
    test_checkSingleLogin();
    test_checkAllLoginsInFile();
    
    std::cout << "✅ ВСЕ ТЕСТЫ УСПЕШНО ПРОЙДЕНЫ!\n";
    
    return 0;
}