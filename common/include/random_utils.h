#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

#include <vector>
#include <random>
#include <algorithm>

class RandomUtils
{
public:
    std::mt19937 gen;
    RandomUtils(unsigned int seed)
    {
        this->gen.seed(seed);
    }
    ~RandomUtils() {}

    inline int random_int(int down, int upper)
    {
        std::uniform_int_distribution<int> dist(down, upper);
        return dist(gen);
    }

    inline int random_double(double min, double max)
    {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(gen);
    }

    template <typename T>
    inline void shuffle(std::vector<T> &vec)
    {
        std::shuffle(vec.begin(), vec.end(), gen);
    }
};

#endif // RANDOM_UTILS_H
