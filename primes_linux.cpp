#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

bool isPrime(int n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

int main() {
    const int NUM_PROCESSES = 10;
    const int STEP = 1000;

    int p2c[NUM_PROCESSES][2];
    int c2p[NUM_PROCESSES][2];
    pid_t pids[NUM_PROCESSES];

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        if (pipe(p2c[i]) == -1 || pipe(c2p[i]) == -1) {
            perror("Eroare la crearea pipe-ului");
            return 1;
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("Eroare la fork()");
            return 1;
        }

        if (pids[i] == 0) {
            for (int j = 0; j < i; ++j) {
                close(p2c[j][1]);
                close(c2p[j][0]);
            }

            close(p2c[i][1]); 
            close(c2p[i][0]); 

            int range[2];
            if (read(p2c[i][0], range, sizeof(range)) == sizeof(range)) {
                int start = range[0];
                int end = range[1];
                for (int num = start; num <= end; ++num) {
                    if (isPrime(num)) {
                        write(c2p[i][1], &num, sizeof(num));
                    }
                }
            }

            close(p2c[i][0]);
            close(c2p[i][1]);
            return 0;
        } else {
            close(p2c[i][0]); 
            close(c2p[i][1]); 

            int range[2] = { i * STEP + 1, (i + 1) * STEP };
            write(p2c[i][1], range, sizeof(range));
            close(p2c[i][1]); 
        }
    }

    int totalPrimes = 0;
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        std::vector<int> primes;
        int prime;
        while (read(c2p[i][0], &prime, sizeof(prime)) > 0) {
            primes.push_back(prime);
        }
        close(c2p[i][0]);
        waitpid(pids[i], nullptr, 0);

        std::cout << "=== Procesul " << (i + 1) << " (PID " << pids[i]
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