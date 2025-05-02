#include "common.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <unordered_set>

const int NUM_PASSWORDS = 3;
const int MAX_PASSWORD_LENGTH = 6;

void crackPasswordsSequential(std::vector<PasswordData> &targets)
{
    std::cout << "Iniciando quebra sequencial de " << targets.size() << " senhas..." << std::endl;

    int found = 0;
    uint64_t attempts = 0;
    std::unordered_set<size_t> foundHashes;

    for (int length = 1; length <= MAX_PASSWORD_LENGTH && found < targets.size(); ++length)
    {
        std::cout << "Testando senhas de comprimento " << length << "..." << std::endl;

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

            // Verifica se o hash já foi encontrado antes
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
                    std::cout << "Senha encontrada: " << candidate
                              << " (original: " << targets[j].password << ")" << std::endl;
                    found++;

                    if (found == targets.size())
                    {
                        std::cout << "Todas as senhas foram encontradas!" << std::endl;
                        goto search_complete;
                    }
                    break;
                }
            }

            if (attempts % 1000000 == 0)
            {
                std::cout << "Progresso: " << attempts << " senhas testadas." << std::endl;
            }
        }
    }

search_complete:
    std::cout << "Processo concluído. " << found << " de " << targets.size()
              << " senhas foram encontradas. Total de tentativas: " << attempts << std::endl;
}

int main(int argc, char *argv[])
{
    std::cout << "==== Quebrador de Senhas por Forca Bruta (Versao Sequencial) ====" << std::endl;

    std::vector<PasswordData> targets = generateRandomPasswords(NUM_PASSWORDS);

    std::cout << "Senhas geradas para teste:" << std::endl;
    for (const auto &pwd : targets)
    {
        std::cout << " - " << pwd.password << " (hash: " << pwd.hash << ")" << std::endl;
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    crackPasswordsSequential(targets);
    auto endTime = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = endTime - startTime;
    double timeSeconds = duration.count();

    std::cout << "\nTempo total de execucao sequencial: "
              << std::fixed << std::setprecision(6) << timeSeconds << " segundos" << std::endl;

    std::vector<std::pair<std::string, double>> timings;
    timings.push_back({"Sequencial", timeSeconds});

    saveResultsToCSV("password_cracking_results.csv", targets, timings);

    displayStats(targets, timings);

    return 0;
}