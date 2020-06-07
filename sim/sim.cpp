#include <cassert>
#include <string>

namespace qpsk::sim
{

void SimQPSK(std::string work_dir);

extern "C"
int main(int argc, char* argv[])
{
    assert(argc > 1);
    auto work_dir = std::string(argv[1]);
    SimQPSK(work_dir);
}

}
