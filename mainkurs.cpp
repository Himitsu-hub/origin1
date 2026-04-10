#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>
#include <stdexcept>

class ini_parser {
private:
    std::map<std::string, std::map<std::string, std::string>> data;

    std::string trim(const std::string& s) {
        size_t first = s.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = s.find_last_not_of(" \t\r\n");
        return s.substr(first, (last - first + 1));
    }

public:
    explicit ini_parser(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Не удалось открыть файл: " + filename);
        }

        std::string line, current_section;
        int line_number = 0;

        while (std::getline(file, line)) {
            line_number++;
            line = trim(line);

            if (line.empty() || line[0] == ';') continue;

            size_t comment_pos = line.find(';');
            if (comment_pos != std::string::npos) {
                line = trim(line.substr(0, comment_pos));
                if (line.empty()) continue;
            }

            if (line.front() == '[' && line.back() == ']') {
                current_section = trim(line.substr(1, line.size() - 2));
                if (current_section.empty()) {
                    throw std::runtime_error("Пустое имя секции в строке: " + std::to_string(line_number));
                }
            } 
            else {
                size_t eq_pos = line.find('=');
                if (eq_pos == std::string::npos) {
                    throw std::runtime_error("Некорректный синтаксис в строке " + std::to_string(line_number) + ": отсутствует '='");
                }

                if (current_section.empty()) {
                    throw std::runtime_error("Переменная объявлена вне секции в строке: " + std::to_string(line_number));
                }

                std::string key = trim(line.substr(0, eq_pos));
                std::string value = trim(line.substr(eq_pos + 1));

                if (key.empty()) {
                    throw std::runtime_error("Пустое имя переменной в строке: " + std::to_string(line_number));
                }

                data[current_section][key] = value;
            }
        }
    }

    template <typename T>
    T get_value(const std::string& query) const {
        size_t dot_pos = query.find('.');
        if (dot_pos == std::string::npos) {
            throw std::invalid_argument("Запрос должен быть в формате 'section.value'");
        }

        std::string section_name = query.substr(0, dot_pos);
        std::string key_name = query.substr(dot_pos + 1);

        auto sec_it = data.find(section_name);
        if (sec_it == data.end()) {
            throw std::runtime_error("Секция '" + section_name + "' не найдена.");
        }

        auto key_it = sec_it->second.find(key_name);
        if (key_it == sec_it->second.end()) {
            std::string hint = "Доступные переменные в этой секции: ";
            for (auto const& [name, val] : sec_it->second) {
                hint += name + ", ";
            }
            throw std::runtime_error("Ключ '" + key_name + "' не найден. " + hint);
        }

        std::stringstream ss(key_it->second);
        T result;
        if (!(ss >> result)) {
            throw std::runtime_error("Ошибка преобразования типа для значения: " + key_it->second);
        }
        return result;
    }
};

template <>
std::string ini_parser::get_value<std::string>(const std::string& query) const {
    size_t dot_pos = query.find('.');
    if (dot_pos == std::string::npos) {
        throw std::invalid_argument("Запрос должен быть в формате 'section.value'");
    }

    std::string section_name = query.substr(0, dot_pos);
    std::string key_name = query.substr(dot_pos + 1);

    auto sec_it = data.find(section_name);
    if (sec_it == data.end()) throw std::runtime_error("Секция не найдена.");

    auto key_it = sec_it->second.find(key_name);
    if (key_it == sec_it->second.end()) throw std::runtime_error("Ключ не найден.");

    return key_it->second;
}

int main() {
    try {
        std::ofstream outfile("test.ini");
        outfile << "[Section1]\nvar1=10.5\nvar2=Hello World\n";
        outfile.close();

        ini_parser parser("test.ini");

        double v1 = parser.get_value<double>("Section1.var1");
        std::string v2 = parser.get_value<std::string>("Section1.var2");

        std::cout << "Value 1 (double): " << v1 << std::endl;
        std::cout << "Value 2 (string): " << v2 << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Произошла ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}