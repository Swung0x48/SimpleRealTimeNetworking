#include <iostream>

class argument_parser {
public:
    uint16_t port = 0;
    argument_parser(int argc, char *argv[]) {
        executable_path = std::string(argv[0]);
        parse(argc, argv);
    };
private:
    std::string executable_path;

    static bool is_int(const std::string& s) {
        for (char i : s) {
            if (!isdigit(i))
                return false;
        };
        return true;
    }

    void help() {
        std::cout << "Usage: " << executable_path << " [-p PORT]" << std::endl;
        port = 0;
    }

    void parse(int argc, char **argv) {
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "-p") {
                if (i + 1 < argc) {
                    std::string port_s = std::string(argv[++i]);
                    if (!is_int(port_s)) {
                        std::cerr << "Invalid port (must be an integer)." << std::endl;
                        help();
                    } else {
                        port = std::stoi(port_s);
                        if (port < 1 || port > 65535) {
                            std::cerr << "Invalid port (must between 1 and 65535)." << std::endl;
                            port = 0;
                            return;
                        }
                        std::cout << "[INFO] Using port " << port << "." << std::endl;
                    }
                } else {
                    std::cerr << "-p option requires one argument." << std::endl;
                    help();
                }
                return;
            } else if (std::string(argv[i]) == "--help") {
                help();
                return;
            }
        }
        std::cout << "[WARN] Port unspecified. Falling back to default value: 60000." << std::endl;
        std::cout << "[INFO] " << "To specify which port to bind to, run " << executable_path << " --help" << std::endl;
    }
};
