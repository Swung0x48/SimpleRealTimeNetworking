#include <iostream>
#include <unordered_map>

namespace args {
    uint16_t port = 60000;
    std::string name;

    bool is_int(std::string s) {
        for (int i = 0; i < s.length(); i ++) {
            if (isdigit(s[i]) == false) return false;
        };
        return true;
    };

    void help() {
        std::cout << "Usage: " << name << " [-p PORT]" << std::endl;
        port = 0;
    };

    void default_port() {
        std::cout << "[INFO] Port unspecified. Using default 60000." << std::endl;
    };

    void parse_port(int argc, char *argv[]) {
        for (int i = 1; i < argc; i ++) {
            if (std::string(argv[i]) == "-p") {
                if (i + 1 < argc) {
                    std::string port_s = std::string(argv[++ i]);
                    if (!is_int(port_s)) {
                        std::cerr << "Invalid port (must be an integer)." << std::endl;
                        help();
                    } else {
                        port = std::stoi(port_s);
                        if (port < 1 || port > 65535) {
                            std::cerr << "Invalid port (must be between 1 and 65535)." << std::endl;
                            port = 0;
                            return;
                        };
                        std::cout << "[INFO] Using port " << port << "." << std::endl;
                    };
                } else {
                    std::cerr << "-p option requires one argument." << std::endl;
                    help();
                };
                return;
            } else if (std::string(argv[i]) == "--help") {
                help();
                return;
            };
        };
        default_port();
    };

    void init(int argc, char *argv[]) {
        name = std::string(argv[0]);
        parse_port(argc, argv);
    };
};