#include <stdio.h> //printf
#include <stdlib.h> //free
#include <stdint.h>
#include <complex.h>
#include <unistd.h>
#include <vector>

#include "matplotlibcpp.h"
#include "pulse_shaping.h"
namespace plt = matplotlibcpp;

int main(){
    plt::plot({1,3,2,4});
    plt::show();
    return 0;
}