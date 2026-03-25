#include <vector>
#include <random>
#include <ctime>


std::vector<int> generate_bit_array(int number)
{
    std::vector<int> rnd;
    for (int i = 0; i < number; i++)
    {
        rnd.push_back(std::rand() % 2);
    }
    return rnd;
}