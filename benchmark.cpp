#include "common.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include <functional>
#include <omp.h>
#include <atomic>
#include <unordered_set>

std::vector<PasswordData> generateFixedPasswords()
{
    std::vector<PasswordData> passwords;
    std::string pwd1 = "ab3";
    size_t hash1 = std::hash<std::string>{}(pwd1);
    passwords.push_back({pwd1, hash1, false});

    std::string pwd2 = "te4F";
    size_t hash2 = std::hash<std::string>{}(pwd2);
    passwords.push_back({pwd2, hash2, false});

    std::string pwd3 = "r9dxA";
    size_t hash3 = std::hash<std::string>{}(pwd3);
    passwords.push_back({pwd3, hash3, false});

    return passwords;
}

double runSequential(std::vector<PasswordData> &targets)
{
    for (auto &pwd : targets)
    {
        pwd.found = false;
    }

    std::cout << "Executando versao sequencial..." << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();

    int found = 0;
    uint64_t attempts = 0;
    std::unordered_set<size_t> foundHashes;

    for (int length = 1; length <= 5 && found < targets.size(); ++length)
    {
        std::cout << "  Testando senhas de comprimento " << length << "..." << std::endl;
        uint64_t maxCombinations = 1;
        for (int i = 0; i < length; ++i)
        {
            maxCombinations *= CHARSET.size();
        }

        for (uint64_t i = 0; i < maxCombinations && found < targets.size(); ++i)
        {
            attempts++;

            std::string candidate = generatePasswordFromIndex(i, length);
            size_t candidateHash = std::hash<std::string>{}(candidate);

            // Verifica se o hash já foi encontrado antes - otimizado
            if (foundHashes.find(candidateHash) != foundHashes.end())
                continue;

            for (size_t j = 0; j < targets.size(); ++j)
            {
                if (targets[j].found)
                    continue;

                if (targets[j].hash == candidateHash)
                {
                    targets[j].found = true;
                    foundHashes.insert(candidateHash);
                    found++;
                    std::cout << "  Senha encontrada: " << candidate
                              << " (original: " << targets[j].password << ")" << std::endl;
                    break;
                }
            }

            if (attempts % 5000000 == 0)
            {
                std::cout << "  Progresso: " << attempts << " senhas testadas." << std::endl;
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = endTime - startTime;
    double timeSeconds = duration.count();

    std::cout << "  Tempo sequencial: " << std::fixed << std::setprecision(6)
              << timeSeconds << " segundos" << std::endl;

    return timeSeconds;
}

double runParallel(std::vector<PasswordData> &targets, int numThreads)
{
    for (auto &pwd : targets)
    {
        pwd.found = false;
    }

    std::cout << "Executando versao paralela com " << numThreads << " threads..." << std::endl;

    omp_set_num_threads(numThreads);
    auto startTime = std::chrono::high_resolution_clock::now();

    std::atomic<int> foundCounter(0);
    std::atomic<uint64_t> totalAttempts(0);
    std::atomic<bool> allPasswordsFound(false);

    // Usando std::atomic<bool> para melhor sincronização
    std::vector<std::atomic<bool>> foundFlags(targets.size());
    for (auto &flag : foundFlags)
    {
        flag.store(false);
    }

    for (int length = 1; length <= 5 && foundCounter.load() < targets.size(); ++length)
    {
        std::cout << "  Testando senhas de comprimento " << length << "..." << std::endl;
        uint64_t maxCombinations = 1;
        for (int i = 0; i < length; ++i)
        {
            maxCombinations *= CHARSET.size();
        }

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
                    std::cout << "  Progresso: ~" << totalAttempts.load() << " senhas testadas." << std::endl;
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
                                std::cout << "  Thread " << tid << " encontrou a senha: "
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
            std::cout << "  Todas as senhas encontradas para comprimento " << length << std::endl;
            break;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = endTime - startTime;
    double timeSeconds = duration.count();

    std::cout << "  Tempo paralelo: " << std::fixed << std::setprecision(6)
              << timeSeconds << " segundos" << std::endl;

    return timeSeconds;
}

void runThreadScalingTest(std::vector<PasswordData> &targets)
{
    std::cout << "\n=== Teste de Escalabilidade de Threads ===" << std::endl;

    std::vector<int> threadCounts = {1, 2, 4, 8, 16, 24};
    std::vector<std::pair<std::string, double>> timings;

    double seqTime = runSequential(targets);
    timings.push_back({"Sequencial", seqTime});

    for (int numThreads : threadCounts)
    {
        double parallelTime = runParallel(targets, numThreads);
        timings.push_back({std::to_string(numThreads) + " threads", parallelTime});
    }

    std::cout << "\n=== Resultados de Escalabilidade ===" << std::endl;
    std::cout << "Versao Sequencial: " << std::fixed << std::setprecision(6)
              << timings[0].second << " segundos" << std::endl;

    for (size_t i = 1; i < timings.size(); ++i)
    {
        int threads = threadCounts[i - 1];
        double speedup = timings[0].second / timings[i].second;
        double efficiency = speedup / threads * 100.0;

        std::cout << threads << " threads: " << std::fixed << std::setprecision(6)
                  << timings[i].second << " segundos (speedup: " << std::setprecision(2) << speedup
                  << "x, eficiencia: " << efficiency << "%)" << std::endl;
    }

    saveResultsToCSV("benchmark_results.csv", targets, timings);
}

int main(int argc, char *argv[])
{
    std::cout << "==== Benchmark de Quebrador de Senhas Sequencial vs. Paralelo ====" << std::endl;

    std::vector<PasswordData> targets = generateFixedPasswords();

    std::cout << "Senhas para teste:" << std::endl;
    for (const auto &pwd : targets)
    {
        std::cout << " - " << pwd.password << " (hash: " << pwd.hash << ")" << std::endl;
    }

    runThreadScalingTest(targets);

    return 0;
}