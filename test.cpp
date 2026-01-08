#include <frozen/unordered_map.h>
#include <string_view>

constexpr auto map = frozen::make_unordered_map<std::string_view, int>({
    {"abc", 10}
});

int main() {

}