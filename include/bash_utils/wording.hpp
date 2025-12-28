#include <list>
#include <array>
#include <limits>
#include <cstdint>
#include <stdexcept>
#include <iostream>

namespace wording {

struct TrieNode {
    std::array<TrieNode*, 26> nodes{ nullptr };
    bool end_of_word = false;
};

/*
CTOIDX here is just an abbreviation to Char TO InDeX
*/

struct make_trie_result {
    std::list<TrieNode> trie_nodes;
    TrieNode* root;

    make_trie_result() : trie_nodes() { this->root = nullptr; }
    make_trie_result(const make_trie_result& _) = delete;
    make_trie_result& operator=(const make_trie_result& _) = delete;

    make_trie_result(make_trie_result&& oth) {

        this->trie_nodes = std::move(oth.trie_nodes);
        this->root = oth.root;
        oth.cancel();
    }

    make_trie_result& operator=(make_trie_result&& oth) {

        this->trie_nodes = std::move(oth.trie_nodes);
        this->root = oth.root;
        oth.cancel();
        return *this;
    }

    void cancel() noexcept {
        this->trie_nodes.clear();
        this->root = nullptr;
    }
};

using CtoidxType = std::array<char, std::numeric_limits<uint8_t>::max()>;

constexpr CtoidxType make_ctoidx() {
    CtoidxType res{-1};
    for(auto& c : res) { c = -1; }

    for(char pos = 'A'; pos <= 'Z'; pos++) {
        res[pos] = pos - 'A';
    }

    for(char pos = 'a'; pos <= 'z'; pos++) {
        res[pos] = pos - 'a';
    }
    return res;
}

constexpr CtoidxType ctoidx_table = make_ctoidx();

constexpr std::array<int, 26> index_to_char_table = {
    'a', 'b', 'c', 'd', 'e',
    'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y',
    'z'
};

void suggestion_add(make_trie_result& dat, const char* str) {
    TrieNode* crawler = dat.root;
    while(*str != '\0'){
        char index = ctoidx_table[*str];
        if(index == -1) {
            throw std::invalid_argument(std::string(str) + " <- is Bad string, can't make Suggestion Tree");
        }
        if(!crawler->nodes[index]) {
            dat.trie_nodes.push_back(TrieNode());
            crawler->nodes[index] = &dat.trie_nodes.back();
        }
        crawler = crawler->nodes[index];
        ++str;
    }
    crawler->end_of_word = true;
}

make_trie_result suggestion_make_tree(const char** first_el, const char** last_el) {
    make_trie_result res;

    auto& trie_nodes = res.trie_nodes;
    trie_nodes.push_back(TrieNode());
    TrieNode* root = &trie_nodes.back();
    res.root = root;
    const char** str_iter = first_el;
    while(str_iter <= last_el) {
        TrieNode* crawler = root;
        suggestion_add(res, *str_iter);
        ++str_iter;
    }

    for(auto& ptr : root->nodes) {

    }   
    return res;
}

void print_possible_suggestion(TrieNode* root, const char* appends) {
    static std::string buffer = "";
    if(!root && !appends) {
        buffer = "";
        return;
    }
    buffer += appends;
    char added_char[2] = {'\0', '\0'};

    if(root->end_of_word)
        std::cout << buffer << "\n";
    
    for(int i = 0; i < 26; i++) {
        if(root->nodes[i]) {
            added_char[0] = index_to_char_table[i];
            print_possible_suggestion(root->nodes[i], added_char);
        }
    }

    buffer.pop_back();
    return;
}

void print_suggestion(TrieNode* root, const char* str) {
    
    const char* str_iter = str;
    while(*str_iter != '\0') {
        char index = ctoidx_table[*str_iter];
        if(index == -1) { 
            std::cout << str << " <- is a bad string, can't print suggestions" << std::endl;
            return;
        }

        if(root->nodes[index] == nullptr) {

            std::cout << "No suggestions..." << std::endl;
            return;
        }

        root = root->nodes[index];
        ++str_iter;
    }

    print_possible_suggestion(root, str);
}
}