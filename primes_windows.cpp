#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

bool isPrime(int n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

int runWorker() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    int range[2] = {0, 0};
    DWORD bytesRead = 0;

    if (!ReadFile(hIn, range, sizeof(range), &bytesRead, NULL) || bytesRead != sizeof(range)) {
        return 1;
    }

    int start = range[0];
    int end = range[1];
    DWORD bytesWritten = 0;

    for (int num = start; num <= end; ++num) {
        if (isPrime(num)) {
            WriteFile(hOut, &num, sizeof(num), &bytesWritten, NULL);
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "worker") {
        return runWorker();
    }

    const int NUM_PROCESSES = 10;
    const int STEP = 1000;

    HANDLE c2pRead[NUM_PROCESSES];
    PROCESS_INFORMATION pi[NUM_PROCESSES];

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string cmdLineStr = "\"" + std::string(exePath) + "\" worker";

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE; 
        sa.lpSecurityDescriptor = NULL;

        HANDLE p2cRead, p2cWrite;
        HANDLE c2pWrite;

        CreatePipe(&p2cRead, &p2cWrite, &sa, 0);
        CreatePipe(&c2pRead[i], &c2pWrite, &sa, 0);

        SetHandleInformation(p2cWrite, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(c2pRead[i], HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = p2cRead;
        si.hStdOutput = c2pWrite;
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

        ZeroMemory(&pi[i], sizeof(pi[i]));
        std::vector<char> cmdBuf(cmdLineStr.begin(), cmdLineStr.end());
        cmdBuf.push_back('\0');

        if (!CreateProcessA(NULL, cmdBuf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi[i])) {
            std::cerr << "Eroare la CreateProcess pentru procesul " << (i + 1) << "\n";
            return 1;
        }
        CloseHandle(p2cRead);
        CloseHandle(c2pWrite);

        int range[2] = { i * STEP + 1, (i + 1) * STEP };
        DWORD written = 0;
        WriteFile(p2cWrite, range, sizeof(range), &written, NULL);
        CloseHandle(p2cWrite);
    }

    int totalPrimes = 0;
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        std::vector<int> primes;
        int prime = 0;
        DWORD bytesRead = 0;

        while (ReadFile(c2pRead[i], &prime, sizeof(prime), &bytesRead, NULL) && bytesRead == sizeof(prime)) {
            primes.push_back(prime);
        }
        CloseHandle(c2pRead[i]);

        WaitForSingleObject(pi[i].hProcess, INFINITE);
        DWORD childPid = pi[i].dwProcessId;
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);

        std::cout << "=== Procesul " << (i + 1) << " (PID " << childPid
                  << ") | Interval [" << (i * STEP + 1) << " - " << ((i + 1) * STEP)
                  << "] | Gasite: " << primes.size() << " prime ===\n";
        for (size_t k = 0; k < primes.size(); ++k) {
            std::cout << primes[k] << (k + 1 < primes.size() ? " " : "");
        }
        std::cout << "\n\n";
        totalPrimes += primes.size();
    }

    std::cout << "====================================================\n";
    std::cout << "TOTAL NUMERE PRIME GASITE (1 - 10000): " << totalPrimes << "\n";
    std::cout << "====================================================\n";
    return 0;
}