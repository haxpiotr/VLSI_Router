#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <stdexcept>
#include <unordered_map>

#include "DEFLoader.hpp"
#include "LEFLoader.hpp"
#include "DataTransformer.hpp"
#include "GlobalRouter.hpp"

namespace po = boost::program_options;

enum class Algorithm {
    SA,
    SA_PAR,
    SA_SP,
    SA_GPU,
    EDA,
    GA_FIXED,
    GA_CLASSIC
};


Algorithm parseAlgorithm(const std::string& name) {
    static const std::unordered_map<std::string, Algorithm> map = {
        {"Simulated_Annealing", Algorithm::SA},
        {"Simulated_Annealing_Parallel", Algorithm::SA_PAR},
        {"Simulated_Annealing_Parallel_SP", Algorithm::SA_SP},
        {"Simulated_Annealing_GPU", Algorithm::SA_GPU},
        {"EDA", Algorithm::EDA},
        {"GA_Fixed_Crossover", Algorithm::GA_FIXED},
        {"GA_Classic", Algorithm::GA_CLASSIC}
    };

    auto it = map.find(name);
    if (it == map.end()) {
        throw std::runtime_error("Nieznany algorytm: " + name);
    }
    return it->second;
}

int main(int argc, char* argv[])
{

    std::string lefFile, defFile, algorithmStr;
    unsigned int cols{ 0 }, rows{ 0 };
        
    size_t iterations{ 0 };
    float temperature{ 0.0f }, alpha{ 0.0f }, eps{ 0.0f };
    unsigned int threads{ 0 };


    unsigned int generations{ 0 }, populationSize{ 0 };
    float limit{ 0.0f };

    float mutationRate{ 0.0f }, crossoverRate{ 0.0f };

    // --- Opcje ---
    po::options_description desc("Opcje");
    desc.add_options()
        ("help,h", "wyswietl pomoc")

        ("LEF", po::value<std::string>(&lefFile)->required(), "LEF file path")
        ("DEF", po::value<std::string>(&defFile)->required(), "DEF file path")
        ("cols", po::value<unsigned int>(&cols)->required(), "column count for global grid")
        ("rows", po::value<unsigned int>(&rows)->required(), "row count for global grid")
        ("algorithm", po::value<std::string>(&algorithmStr)->required(), "algorithm of choice to perform optimization")

        // SA
        ("iterations", po::value<size_t>(&iterations), "number of iterations")
        ("temperature", po::value<float>(&temperature), "initial temperature")
        ("alpha", po::value<float>(&alpha), "cooling rate")
        ("eps", po::value<float>(&eps), "convergence threshold")
        ("threads", po::value<unsigned int>(&threads), "number of threads")

        // EDA
        ("generations", po::value<unsigned int>(&generations))
        ("populationSize", po::value<unsigned int>(&populationSize))
        ("limit", po::value<float>(&limit))

        // GA
        ("mutationRate", po::value<float>(&mutationRate))
        ("crossoverRate", po::value<float>(&crossoverRate));

    po::variables_map vm;

    try 
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) 
        {
            std::cout << desc << "\n";
            return 0;
        }

        po::notify(vm);
    }
    catch (const po::error& e) 
    {
        std::cerr << "Parse error: " << e.what() << "\n\n";
        std::cerr << desc << "\n";
        return 1;
    }

    // helper do walidacji
    auto require = [&](const char* opt) 
    {
        if (!vm.count(opt)) 
        {
            throw std::runtime_error(std::string("Missing parameter: ") + opt);
        }
    };

    // --- Dependent validation ---
    Algorithm algo;
    try 
    {
        algo = parseAlgorithm(algorithmStr);

        switch (algo) {
        case Algorithm::SA:
            require("iterations");
            require("temperature");
            require("alpha");
            require("eps");
            break;

        case Algorithm::SA_PAR:
        case Algorithm::SA_SP:
        case Algorithm::SA_GPU:
            require("iterations");
            require("temperature");
            require("alpha");
            require("eps");
            require("threads");
            break;

        case Algorithm::EDA:
            require("generations");
            require("populationSize");
            require("alpha");
            require("limit");
            break;

        case Algorithm::GA_FIXED:
            require("generations");
            require("populationSize");
            require("mutationRate");
            break;

        case Algorithm::GA_CLASSIC:
            require("generations");
            require("populationSize");
            require("mutationRate");
            require("crossoverRate");
            break;
        }
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Validation error: " << e.what() << "\n";
        return 2;
    }

    std::cout << "Loading design from LEF and DEF files...\n";

    try
    {
        in::def::Loader defLoader;
        in::lef::Loader lefLoader;
        const auto design = defLoader.get(defFile);
        const auto library = lefLoader.get(lefFile);

        std::cout << "Transforming data...\n";

        in::DataTransformer transformer(library, design);

        std::cout << "Creating global router...\n";

        in::GlobalRouter GRouter(transformer, cols, rows);

        std::cout << "Preparing initial solution...\n";

        GRouter.createInitialSolution();

        switch (algo)
        {
        case Algorithm::SA:
        {
            std::cout << "Performing SA with " << iterations << " iterations, initial temperature "
                << temperature << ", cooling rate " << alpha << ", and convergence threshold "
                << eps << ".\n";

            GRouter.performSA(iterations, temperature, alpha, eps);
            break;
        }

        case Algorithm::SA_PAR:
        {
            std::cout << "Performing parallel SA with " << iterations << " iterations, initial temperature "
                << temperature << ", cooling rate " << alpha << ", convergence threshold " << eps << ", and " << threads << " threads.\n";

            GRouter.performSAPar(iterations, temperature, alpha, eps, threads);
            break;
        }

        case Algorithm::SA_SP:
        {
            std::cout << "Performing parallel space partitioned SA with " << iterations << " iterations, initial temperature "
                << temperature << ", cooling rate " << alpha << ", convergence threshold " << eps << ", and " << threads << " threads.\n";

            GRouter.performSAParSpacePartitioned(iterations, temperature, alpha, eps, threads);
            break;
        }

        case Algorithm::SA_GPU:
        {
            std::cout << "Performing parallel space partitioned SA on GPU with " << iterations << " iterations, initial temperature "
                << temperature << ", cooling rate " << alpha << ", convergence threshold " << eps << ", and " << threads << " threads.\n";

            GRouter.performSAParSpacePartitionedOnGPU(iterations, temperature, alpha, eps, threads);
            break;
        }

        case Algorithm::EDA:
        {
            std::cout << "Performing EDA with " << generations << " generations, population size "
                << populationSize << ", alpha " << alpha << ", and limit " << limit << ".\n";

            GRouter.performEDA(generations, populationSize, alpha, limit);
            break;
        }

        case Algorithm::GA_FIXED:
        {
            std::cout << "Performing GA with fixed crossover with " << generations << " generations, population size "
                << populationSize << ", and mutation rate " << mutationRate << ".\n";

            GRouter.performGeneticAlgorithm(generations, populationSize, mutationRate);
            break;
        }

        case Algorithm::GA_CLASSIC:
        {
            std::cout << "Performing classic GA with " << generations << " generations, population size "
                << populationSize << ", mutation rate " << mutationRate << ", and crossover rate " << crossoverRate << ".\n";

            GRouter.performGeneticAlgorithmRandRatio(generations, populationSize, crossoverRate, mutationRate);
            break;
        }

        default:
            break;
        }

        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
    
}