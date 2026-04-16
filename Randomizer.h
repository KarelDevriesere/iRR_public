#ifndef RANDOMIZER_H  
#define RANDOMIZER_H

#include <random>
#include <unordered_map>
#include <variant>

template <typename DataType>
class Randomizer {
    // static_assert -> checked during compile time
    static_assert(std::is_arithmetic_v<DataType>, "Template type must be numeric");

public:
    using DistType = std::conditional_t<
        std::is_integral_v<DataType>,
        std::uniform_int_distribution<DataType>,
        std::uniform_real_distribution<DataType>
    >;

    DistType dist;
    std::mt19937& gen;

    Randomizer(DataType a, DataType b, std::mt19937& g) : dist(a, b), gen(g) {}

    DataType Sample() {
        return dist(gen);
    }
};

using AnyRandomizer = std::variant<Randomizer<int>, Randomizer<double>, Randomizer<float>>;

template<typename Key>
class RandomizerCollection{
    protected:
        std::unordered_map<Key,AnyRandomizer>Randomizers;
    public:
        RandomizerCollection();
        template <typename DataType>
        void insert(const Key& k, const DataType a, const DataType b, std::mt19937& g){
            Randomizers[k] = Randomizer<DataType>(a,b,g);
        };
        AnyRandomizer get(const Key& k)const{Randomizers.at(k);};
};
#endif