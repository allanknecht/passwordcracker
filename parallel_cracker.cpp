#include "common.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <omp.h>
#include <unordered_set>
#include <mutex>
#include <vector>

const int NUM_PASSWORDS = 3;
const int MAX_PASSWORD_LENGTH = 6;

void crackPasswordsParallel(std::vector<PasswordData> &targets, int numThreads)
{
    std::cout << "Iniciando quebra paralela de " << targets.size()
              << " senhas com " << numThreads << " threads usando OpenMP..." << std::endl;

    omp_set_num_threads(numThreads);
    std::atomic<int> foundCounter(0);
    std::atomic<bool> allPasswordsFound(false);
    std::atomic<uint64_t> totalAttempts(0);

    // Usando std::atomic<bool> em vez de chars para melhor sincronização
    std::vector<std::atomic<bool>> foundFlags(targets.size());
    for (auto &flag : foundFlags)
    {
        flag.store(false);
    }

    for (int length = 1; length <= MAX_PASSWORD_LENGTH && foundCounter.load() < targets.size(); ++length)
    {
        std::cout << "Testando senhas de comprimento " << length << "..." << std::endl;
        uint64_t maxCombinations = 1;
        for (int i = 0; i < length; ++i)
            maxCombinations *= CHARSET.size();

        // Aumentamos a granularidade para reduzir overhead de scheduling
        uint64_t chunkSize = std::max(uint64_t(10000), maxCombinations / (numThreads * 4));

#pragma omp parallel
        {
            int tid = omp_get_thread_num();
            uint64_t localAttempts = 0;

            // Cada thread mantém um conjunto local de senhas encontradas
            std::unordered_set<size_t> localFoundHashes;

#pragma omp for schedule(dynamic, chunkSize)
            for (uint64_t i = 0; i < maxCombinations; ++i)
            {
                if (allPasswordsFound.load(std::memory_order_relaxed))
                    continue;

                localAttempts++;

                // Feedback de progresso menos frequente para reduzir overhead
                if (tid == 0 && localAttempts % 5000000 == 0)
                {
                    totalAttempts.fetch_add(5000000, std::memory_order_relaxed);
                    std::cout << "Progresso: ~" << totalAttempts.load() << " senhas testadas." << std::endl;
                }

                std::string candidate = generatePasswordFromIndex(i, length);
                size_t candidateHash = std::hash<std::string>{}(candidate);

                // Verificar primeiro na cache local para evitar synchronization
                if (localFoundHashes.find(candidateHash) != localFoundHashes.end())
                    continue;

                for (size_t j = 0; j < targets.size(); ++j)
                {
                    // Verificar flag com ordem relaxada para melhor performance
                    if (foundFlags[j].load(std::memory_order_relaxed))
                        continue;

                    if (targets[j].hash == candidateHash)
                    {
                        // Técnica de compare_exchange_strong para operação atômica
                        bool expected = false;
                        if (foundFlags[j].compare_exchange_strong(expected, true))
                        {
                            // Esta thread foi a primeira a marcar esta senha
                            targets[j].found = true;
                            localFoundHashes.insert(candidateHash);

#pragma omp critical(cout)
                            {
                                std::cout << "Thread " << tid << " encontrou a senha: "
                                          << candidate << " (original: " << targets[j].password << ")" << std::endl;
                            }

                            // Incrementa contador e verifica se encontrou todas as senhas
                            if (foundCounter.fetch_add(1) + 1 == targets.size())
                            {
                                allPasswordsFound.store(true, std::memory_order_relaxed);
                            }
                        }
                        break;
                    }
                }
            }
        }

        if (foundCounter.load() == targets.size())
        {
            std::cout << "Todas as senhas foram encontradas!" << std::endl;
            break;
        }
    }

    std::cout << "Processo concluído. Total de tentativas aproximado: " << totalAttempts.load() << std::endl;
}

int main(int argc, char *argv[])
{
    int numThreads = omp_get_max_threads();
    if (argc > 1)
    {
        numThreads = std::atoi(argv[1]);
        if (numThreads <= 0)
            numThreads = 1;
    }

    std::cout << "==== Quebrador de Senhas por Forca Bruta (Versao Paralela com OpenMP) ====" << std::endl;
    std::cout << "Utilizando " << numThreads << " threads" << std::endl;

    std::vector<PasswordData> targets = generateRandomPasswords(NUM_PASSWORDS);

    std::cout << "Senhas geradas para teste:" << std::endl;
    for (const auto &pwd : targets)
        std::cout << " - " << pwd.password << " (hash: " << pwd.hash << ")" << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();
    crackPasswordsParallel(targets, numThreads);
    auto endTime = std::chrono::high_resolution_clock::now();

    double timeSeconds = std::chrono::duration<double>(endTime - startTime).count();

    std::cout << "\nTempo total de execucao paralela (" << numThreads << " threads): "
              << std::fixed << std::setprecision(6) << timeSeconds << " segundos" << std::endl;

    std::vector<std::pair<std::string, double>> timings;
    timings.push_back({std::to_string(numThreads) + " threads", timeSeconds});

    saveResultsToCSV("password_cracking_results.csv", targets, timings);
    displayStats(targets, timings);

    return 0;
}