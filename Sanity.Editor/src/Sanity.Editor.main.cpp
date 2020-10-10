#include "sanity_engine.hpp"
#include "rx/core/string.h"

int main(int argc, char** argv) {
    const auto exe_filename = Rx::String{argv[0]};
    const auto last_slash_pos = exe_filename.find_last_of('\\');
    const auto exe_dir = exe_filename.substring(0, last_slash_pos);
	auto engine = SanityEngine{exe_dir.data()};
	return 0; 
}