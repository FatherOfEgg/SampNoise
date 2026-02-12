#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include <PhyloParse/phyloParse.h>
#include <PhyloParse/status.h>
#include <PhyloParse/util.h>

static std::mt19937 gen;

static inline void setupChoices(
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

bool isDescendant(
    const std::vector<std::vector<PhyloParse::Edge>> &adjList,
    uint64_t parent,
    uint64_t child
) {
    if (parent == child) {
        return true;
    }

    std::stack<uint64_t> s;

    for (PhyloParse::Edge child : adjList[parent]) {
        s.push(child.to);
    }

    while (!s.empty()) {
        uint64_t current = s.top();
        s.pop();

        if (current == child) {
            return true;
        }

        for (PhyloParse::Edge child : adjList[current]) {
            s.push(child.to);
        }
    }

    return false;
}

static std::pair<uint64_t, uint64_t> pickEdge(
    const std::vector<std::vector<PhyloParse::Edge>> &adjList,
    uint64_t *rootPtr
) {
    std::uniform_int_distribution<> distr(0, adjList.size() - 1);

    do {
        uint64_t start;

        do {
            start = distr(gen);
        } while (
            (rootPtr != nullptr && *rootPtr == start) ||
            adjList[start].empty()
        );

        std::uniform_int_distribution<> distrChild(0, adjList[start].size() - 1);
        uint64_t end = adjList[start][distrChild(gen)].to;

        return std::make_pair(start, end);
    } while (true);
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
    std::cout << "\t\tDefaults to 1." << std::endl;
    std::cout << "\t-n\tHow much noise (SPR) to add to each sample." << std::endl;
    std::cout << "\t\tDefaults to 1." << std::endl;
    std::cout << "\t-o\tOutput directory." << std::endl;
    std::cout << "\t\tDefaults to 'out' in the same directory." << std::endl;
    std::cout << "\t-sd\tOptionally set seed." << std::endl;
    std::cout << std::endl;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string file;
    std::string outDir;
    unsigned int samples = 1;
    unsigned int noisiness = 1;
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

    std::ostringstream outPath;

    {
        std::filesystem::path fullPath(file);

        if (outDir.empty())  {
            outDir = fullPath.parent_path() / "out";
        }

        outPath << outDir << "/" << fullPath.stem().string();
    }

    PhyloParse::Graph g;
    PhyloParse::ExtendedNewickFormat f;

    PhyloParse::Status s = g.open(f, file);

    if (!s.isOk()) {
        std::cout << "Failed to open graph: " << s.getErrorMsg() << std::endl;
        return EXIT_FAILURE;
    }

    std::filesystem::create_directories(outDir);

    uint64_t root = g.getRoot();
    const std::vector<std::vector<PhyloParse::Edge>> &revAdjList = g.getRevAdjList();
    const std::vector<uint64_t> &reticulations = g.getReticulations();

    std::unordered_map<uint64_t, uint64_t> choices;
    choices.reserve(reticulations.size());

    for (unsigned int s = 0; s < samples; s++) {
        setupChoices(choices, revAdjList, reticulations);
        auto subtree = g.getSubtree(choices);

        std::vector<std::vector<PhyloParse::Edge>> adjList = subtree.first;
        std::vector<std::vector<PhyloParse::Edge>> revAdjList = subtree.second;

        for (unsigned int n = 0; n < noisiness; n++) {
            std::pair<uint64_t, uint64_t> edge1 = pickEdge(adjList, &root);
            std::pair<uint64_t, uint64_t> edge2;

            do {
                edge2 = pickEdge(adjList, nullptr);
            } while (isDescendant(adjList, edge1.first, edge2.first));

            PhyloParse::Util::spr(
                adjList,
                revAdjList,
                edge1.first,
                edge1.second,
                edge2.first,
                edge2.second
            );
        }

        std::ostringstream oss;
        oss << outPath.str();
        oss << std::setfill('0') << std::setw(2) << s + 1;
        oss << ".enwk";

        PhyloParse::Graph temp(adjList, revAdjList, g.getLeaves(), {});
        temp.save(f, oss.str());
    }
}
