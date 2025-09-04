#include "SharedQueue.h"


// a function to print primes smaller or equal to input n
void printPrimes (unsigned long long n, int priority) {

    if(priority > 10 || priority < 1){
        throw std::invalid_argument("wrong thread priority!!");
    }

    std::cout << "Task will scheduled with priority " << priority << std::endl;

    auto isPrime = [] (unsigned long long n) ->bool {
        if (n <= 1) return false;

        // Check from 2 to n-1
        for(unsigned long long i = 2; i < n; i++) {
            if (n % i == 0) {
                return false;
            }
        }
        return true;
    };

    for (unsigned long long i = 2; i <= n; i++) {
        if (isPrime(i)) {
            std::cout << i << " ";
        }
    }

    std::cout << std::endl;

}

int main(int argc, char* argv[]) {
    int duration_seconds = 300; // default
   
    ThreadPool firstPool(1);
    std::string cliCommand = "compute";
    bool destroyThePool = false;

    // Parse command-line arguments
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "-i") {
            duration_seconds = std::stoi(argv[i + 1]);
        }
    }
    
    std::cout << "Application will run " << std::to_string(duration_seconds) << " secs!" << std::endl;

    auto start = std::chrono::steady_clock::now();
    auto duration = std::chrono::seconds(duration_seconds);


    do {
      
        if(cliCommand == "restart") {
            std::cout << "How many threads?" << std::endl;
            std::cin >> cliCommand;
            unsigned int numThreads = std::stoi(cliCommand);
            firstPool.Restart(numThreads);
        }
        else if(cliCommand == "exit") {
            firstPool.Terminate();
            destroyThePool = true;
        }
        else if(cliCommand == "compute") {
            unsigned long long primeCandidate = 3000;
            int threadPriority = 2;

            if (primeCandidate > 0 && threadPriority > 0) {
                std::cout << "primeCandidate: " << primeCandidate << ", " << "threadPriority: " << threadPriority << std::endl;
            }
            else {
                std::cout << "Error: negative integers entered...." << std::endl;
            }

            firstPool.QueueJob(printPrimes, primeCandidate, threadPriority);

            cliCommand = ""; // reset command
        }
        else {
            if((std::chrono::steady_clock::now() - start) > duration) {
                destroyThePool = true;
                std::cerr << "Time out...." << std::endl;
            }
        }

    } while(!destroyThePool);

    return 0;
}
