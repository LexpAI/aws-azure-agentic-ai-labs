#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

struct LogEntry {
    std::string timestamp;
    std::string server;
    std::string level;
    std::string message;
    int cpu = 0;
};

std::string trim(const std::string& value) {
    const auto start = value.find_first_not_of(" \t");

    if (start == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(" \t");
    return value.substr(start, end - start + 1);
}

int extract_cpu(const std::string& part) {
    const std::string key = "cpu=";
    const auto pos = part.find(key);

    if (pos == std::string::npos) {
        return 0;
    }

    return std::stoi(part.substr(pos + key.size()));
}

LogEntry parse_line(const std::string& line) {
    std::stringstream ss(line);
    std::string part;
    std::vector<std::string> parts;

    while (std::getline(ss, part, '|')) {
        parts.push_back(trim(part));
    }

    LogEntry entry;

    if (parts.size() >= 5) {
        entry.timestamp = parts[0];
        entry.server = parts[1];
        entry.level = parts[2];
        entry.message = parts[3];
        entry.cpu = extract_cpu(parts[4]);
    }

    return entry;
}

int main() {
    std::ifstream file("logs.txt");

    if (!file.is_open()) {
        std::cerr << "Could not open logs.txt" << std::endl;
        return 1;
    }

    std::vector<LogEntry> entries;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty()) {
            entries.push_back(parse_line(line));
        }
    }

    int info_count = 0;
    int warning_count = 0;
    int error_count = 0;
    int cpu_total = 0;
    int max_cpu = 0;
    std::string max_cpu_server;

    for (const auto& entry : entries) {
        if (entry.level == "INFO") {
            info_count++;
        } else if (entry.level == "WARNING") {
            warning_count++;
        } else if (entry.level == "ERROR") {
            error_count++;
        }

        cpu_total += entry.cpu;

        if (entry.cpu > max_cpu) {
            max_cpu = entry.cpu;
            max_cpu_server = entry.server;
        }
    }

    double average_cpu = 0.0;

    if (!entries.empty()) {
        average_cpu = static_cast<double>(cpu_total) / entries.size();
    }

    std::cout << "=== C++ Server Health Monitor ===" << std::endl;
    std::cout << std::endl;

    std::cout << "Total log entries : " << entries.size() << std::endl;
    std::cout << "INFO entries      : " << info_count << std::endl;
    std::cout << "WARNING entries   : " << warning_count << std::endl;
    std::cout << "ERROR entries     : " << error_count << std::endl;

    std::cout << "Average CPU       : "
              << std::fixed << std::setprecision(2)
              << average_cpu << "%"
              << std::endl;

    std::cout << "Highest CPU       : "
              << max_cpu << "% on "
              << max_cpu_server
              << std::endl;

    std::cout << std::endl;
    std::cout << "Incident Summary:" << std::endl;

    if (error_count > 0) {
        std::cout << "- Critical signals detected. The system has "
                  << error_count
                  << " error event(s)."
                  << std::endl;
    }

    if (warning_count > 0) {
        std::cout << "- Warning signals detected. The system has "
                  << warning_count
                  << " warning event(s)."
                  << std::endl;
    }

    if (average_cpu > 70.0) {
        std::cout << "- Average CPU is high. Capacity review is recommended."
                  << std::endl;
    } else {
        std::cout << "- Average CPU is within an acceptable range."
                  << std::endl;
    }

    if (!max_cpu_server.empty()) {
        std::cout << "- Server requiring attention: "
                  << max_cpu_server
                  << " with CPU="
                  << max_cpu
                  << "%."
                  << std::endl;
    }

    return 0;
}
