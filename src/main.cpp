#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <PhyloParse/phyloParse.h>
#include <PhyloParse/status.h>

static std::mt19937 gen;

static void setupChoices(
    std::unordered_map<uint64_t, uint64_t> &choices,
    const std::vector<std::vector<PhyloParse::Edge>> &revAdjList,
    const std::vector<uint64_t> &reticulations
) {
    for (uint64_t r : reticulations) {
        std::vector<PhyloParse::Edge> parents = revAdjList[r];
        std::uniform_int_distribution<> distr(0, parents.size() - 1);

        choices[r] = parents[distr(gen)].to;
    }
}

static void usage(const std::string &prgName) {
    std::cout << prgName << std::endl;
    std::cout << std::endl;
    std::cout << "USAGE:" << std::endl;
    std::cout << "\tsampnoise -s 5 -n 5 -o out.enwk" << std::endl;
    std::cout << std::endl;
    std::cout << "FLAGS:" << std::endl;
    std::cout << "\t-h\tPrints help information." << std::endl;
    std::cout << "\t-s\tNumber of samples (contained subtree) to get." << std::endl;
    std::cout << "\t\tDefaults to 0." << std::endl;
    std::cout << "\t-n\tHow much noise (SPR) to add to each sample." << std::endl;
    std::cout << "\t\tDefaults to 0." << std::endl;
    std::cout << "\t-o\tOutput directory." << std::endl;
    std::cout << "\t\tDefaults to 'out' in the same directory." << std::endl;
    std::cout << "\t-sd\tOptionally set seed." << std::endl;
    std::cout << std::endl;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string file;
    std::string outDir;
    unsigned int samples = 0;
    unsigned int noisiness = 0;
    bool isSetSeed = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h") {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (arg == "-s") {
            i++;

            if (i >= argc) {
                std::cout << "Please provide a number after " << arg << "." << std::endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            samples = std::stoul(argv[i]);
        } else if (arg == "-n") {
            i++;

            if (i >= argc) {
                std::cout << "Please provide a number after " << arg << "." << std::endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            noisiness = std::stoul(argv[i]);
        } else if (arg == "-o") {
            i++;

            if (i >= argc) {
                std::cout << "Please provide a filepath after " << arg << "." << std::endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            outDir = argv[i];
        } else if (arg == "-sd") {
            i++;

            if (i >= argc) {
                std::cout << "Please provide a number after " << arg << "." << std::endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            gen.seed(std::stoul(argv[i]));
            isSetSeed = true;
        } else {
            file = arg;
        }
    }

    if (file.empty()) {
        std::cout << "Please provide an input." << std::endl;
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!isSetSeed) {
        std::random_device rd;
        gen.seed(rd());
    }

    std::string fileBase;

    {
        std::filesystem::path fullPath = file;

        fileBase = fullPath.stem();

        if (outDir.empty())  {
            outDir = fullPath.parent_path() / "out";
        }
    }

    std::filesystem::create_directories(outDir);

    PhyloParse::Graph g;
    PhyloParse::ExtendedNewickFormat f;

    PhyloParse::Status s = g.open(f, argv[1]);

    if (!s.isOk()) {
        std::cout << "Failed to open graph: " << s.getErrorMsg() << std::endl;
        return EXIT_FAILURE;
    }

    const std::vector<std::vector<PhyloParse::Edge>> &revAdjList = g.getRevAdjList();
    const std::vector<uint64_t> &reticulations = g.getReticulations();

    std::unordered_map<uint64_t, uint64_t> choices;
    choices.reserve(reticulations.size());

    for (unsigned int s = 0; s < samples; s++) {
        setupChoices(choices, revAdjList, reticulations);
        auto subtree = g.getSubtree(choices);

        for (unsigned int n = 0; n < noisiness; n++) {

        }
    }

}
