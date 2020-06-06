#include <cassert>
#include <string>

namespace qpsk::sim
{

void SimQPSK(std::string vcd_file);

extern "C"
int main(int argc, char* argv[])
{
    assert(argc > 1);
    auto vcd_file = std::string(argv[1]);
    SimQPSK(vcd_file);
}

}
