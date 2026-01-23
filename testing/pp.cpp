#include "static_parser.hpp"

auto context = sp::make_context(
    sp::snOpt()("--ints")
        .nargs(1)
        .convert(sp::type_code::INT)
        .required()
);

int main(int argc, const char* argv[]) {
    std::array<sp::Blob, 5> ints;
    auto rcontext = sp::make_rcontext(
        context, 
        sp::Request(sp::ModProf().bind(sp::PointingArr(ints)), "--ints")
    );

    sp::parse(rcontext.mapper, ++argv, --argc, sp::parser::DumpSize<0>{});
    std::cout << "Value : ";
    for(auto& bl : ints) {
        if(std::holds_alternative<std::monostate>(bl)) 
            std::cout << "<NONE>, ";
        else
            std::cout << std::get<int>(bl) << ", ";
    }
    std::cout << std::endl;
    return 0;
}