# Quebrador de Senhas com OpenMP

Este projeto implementa um sistema de quebra de senhas por força bruta, com versões sequencial e paralela (usando OpenMP), além de um programa de benchmark para comparação de desempenho.

## Estrutura do Projeto

- `sequential_cracker.cpp`: Versão sequencial do algoritmo.
- `parallel_cracker.cpp`: Versão paralela utilizando OpenMP.
- `benchmark.cpp`: Programa que executa testes de desempenho e compara os tempos de execução das versões.

## Requisitos

- Compilador compatível com C++17 (ex: `g++`)
- Suporte à biblioteca OpenMP
- Ambiente Linux, WSL ou terminal com suporte a execução de binários C++

## Como Compilar

Abra o terminal na pasta onde estão localizados os arquivos `.cpp` e execute os seguintes comandos:

```bash
g++ -std=c++17 -O2 -fopenmp -w parallel_cracker.cpp -o parallel_cracker
g++ -std=c++17 -O2 -w sequential_cracker.cpp -o sequential_cracker
g++ -std=c++17 -O2 -fopenmp -w benchmark.cpp -o benchmark
```

Esses comandos vão gerar três executáveis:

- `parallel_cracker`
- `sequential_cracker`
- `benchmark`

## Como Executar o Benchmark

Após a compilação, execute o programa principal com o comando:

```bash
./benchmark
```

Esse arquivo realiza testes automatizados comparando a performance entre os modelos sequencial e paralelo, utilizando senhas fixas e controladas.

## Notas Técnicas

- O programa `parallel_cracker` utiliza diretivas OpenMP para paralelizar a força bruta, com balanceamento dinâmico de carga.
- O `benchmark.cpp` foi criado para garantir consistência nos testes, utilizando o mesmo conjunto de senhas e medindo tempos com precisão.
- O uso de `unordered_set` e variáveis atômicas garante desempenho e evita duplicações na quebra de senhas.

## Autores

- Allan Knecht  
- Luís Felipe Montini Giaretta
