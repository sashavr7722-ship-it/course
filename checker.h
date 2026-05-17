#ifndef CHECKER_H
#define CHECKER_H

#include <string>
#include <vector>
#include <tuple>

struct Finding {
    std::string type;
    std::string value;
    int weight;
};


std::vector<std::tuple<std::string, std::string, int>> findPersonalDataWithWeight(const std::string& text);


std::pair<std::vector<std::tuple<std::string, std::string, int>>, int> checkSingleLogin(const std::string& login);


std::pair<std::vector<std::tuple<std::string, std::string, int>>, int> checkAllLoginsInFile(const std::string& filename);


bool saveResultsToFile(const std::vector<std::tuple<std::string, std::string, int>>& results, 
                       int weight, 
                       const std::string& input_filename,
                       const std::string& report_type);


int calculateSecurityLevel(const std::vector<std::tuple<std::string, std::string, int>>& findings);

#endif 