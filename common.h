#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <unordered_set>

const std::string CHARSET = "abcdefghijklmnopqrstuvwxyzABCDEF0123456789";

struct PasswordData
{
    std::string password;
    size_t hash;
    bool found;
};

struct ThreadResult
{
    uint64_t attemptsCount;
    std::vector<std::pair<int, std::string>> foundPasswords;
};

// gera senhas aleatórias
std::vector<PasswordData> generateRandomPasswords(int count)
{
    std::vector<PasswordData> passwords;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> lengthDist(3, 6);
    std::uniform_int_distribution<> charDist(0, CHARSET.size() - 1);

    for (int i = 0; i < count; ++i)
    {
        int length = lengthDist(gen);
        std::string password;
        for (int j = 0; j < length; ++j)
        {
            password += CHARSET[charDist(gen)];
        }
        size_t hash = std::hash<std::string>{}(password);
        passwords.push_back({password, hash, false});
    }
    return passwords;
}

// gera senha de forma determinística
std::string generatePasswordFromIndex(uint64_t index, int length)
{
    std::string password(length, ' ');
    size_t charsetSize = CHARSET.size();
    for (int i = 0; i < length; ++i)
    {
        password[length - 1 - i] = CHARSET[index % charsetSize];
        index /= charsetSize;
    }
    return password;
}

int checkPasswordHash(size_t passwordHash, const std::vector<PasswordData> &targets,
                      const std::unordered_set<size_t> &foundHashes = std::unordered_set<size_t>())
{
    for (size_t i = 0; i < targets.size(); ++i)
    {
        if (!targets[i].found && targets[i].hash == passwordHash &&
            foundHashes.find(passwordHash) == foundHashes.end())
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void saveResultsToCSV(const std::string &filename,
                      const std::vector<PasswordData> &passwords,
                      const std::vector<std::pair<std::string, double>> &timings)
{
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir arquivo para escrita: " << filename << std::endl;
        return;
    }
    file.seekp(0, std::ios::end);
    if (file.tellp() == 0)
    {
        file << "Timestamp,Tipo,Threads,Senha,Tempo(s)\n";
    }

    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::string timestamp = std::ctime(&now_time_t);
    if (!timestamp.empty() && timestamp[timestamp.length() - 1] == '\n')
    {
        timestamp.erase(timestamp.length() - 1);
    }

    for (const auto &timing : timings)
    {
        file << timestamp << "," << timing.first << ",";

        // Código corrigido para extração do número de threads
        if (timing.first == "Sequencial")
        {
            file << "N/A,";
        }
        else
        {
            // Procura por dígitos no início da string seguidos por " threads"
            size_t spacePos = timing.first.find(" ");
            if (spacePos != std::string::npos &&
                timing.first.substr(spacePos + 1) == "threads")
            {
                file << timing.first.substr(0, spacePos) << ",";
            }
            else
            {
                file << "N/A,";
            }
        }

        file << "\"";
        for (const auto &pwd : passwords)
        {
            file << pwd.password << " ";
        }
        file << "\",";
        file << std::fixed << std::setprecision(6) << timing.second << "\n";
    }
    file.close();
}

void displayStats(const std::vector<PasswordData> &targets,
                  const std::vector<std::pair<std::string, double>> &timings)
{
    std::cout << "\n=== Estatisticas de Execucao ===" << std::endl;
    std::cout << "Senhas encontradas:" << std::endl;
    for (const auto &pwd : targets)
    {
        std::cout << " - " << pwd.password << " (hash: " << pwd.hash << ")" << std::endl;
    }

    std::cout << "\nTempo de execucao:" << std::endl;
    for (const auto &timing : timings)
    {
        std::cout << " - " << timing.first << ": "
                  << std::fixed << std::setprecision(6) << timing.second << " segundos" << std::endl;
    }

    auto seqIt = std::find_if(timings.begin(), timings.end(),
                              [](const auto &pair)
                              { return pair.first == "Sequencial"; });
    if (seqIt != timings.end())
    {
        double seqTime = seqIt->second;
        std::cout << "\nSpeedups em relacao a versao sequencial:" << std::endl;
        for (const auto &timing : timings)
        {
            if (timing.first != "Sequencial")
            {
                double speedup = seqTime / timing.second;
                std::cout << " - " << timing.first << ": "
                          << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
            }
        }
    }
}

#endif // COMMON_H
