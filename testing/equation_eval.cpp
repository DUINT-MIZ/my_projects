#include <vector>
#include <list>
#include <iostream>
#include <string>
#include <charconv>
#include <string_view>
#include <optional>
#include <limits>

struct pseudoNode {

};

enum class Expect {
    TERM,
    OPERAND
};

enum class NodeType {
    OPERAND,
    NUMBER
};

struct Node {
    public :
    Node* left = nullptr;
    Node* right = nullptr;
    NodeType node_type;
    int value = 0;
    char operand = '\0';

    Node() = default;
    Node(int val) : value(val), node_type(NodeType::NUMBER) {}
    Node(char val) : operand(val), node_type(NodeType::OPERAND) {}
};

std::list<Node> nodes;

int operand_level(char c) {
    switch (c)
    {
    case '+' : case '-' :
    return 1;
    case '*' : case '/' :
    return 2;
    
    default:
    return -1;
    }

}

Node* make_node_from_num(const char* num_beg, const char* num_end) {
    Node node;
    node.node_type = NodeType::NUMBER;
    if(std::from_chars(num_beg, num_end, node.value).ec != std::errc{}) {
        std::cerr << "Can't convert : \"" << std::string(num_beg, (num_end - num_beg)) << "\", to a number !" << std::endl;
        return nullptr;
    }
    nodes.push_back(node);
    return &nodes.back();
}

Node* assemble_tree(std::vector<char>& operands, std::vector<Node*>& terms) {
    std::cerr << "Asemble tree" << std::endl;
    if(terms.empty()) return nullptr;
    if(terms.size() == 1) return terms[0];

    int split_index = -1;
    int minimum_precedence = std::numeric_limits<int>::max();
    int current_precedence;
    for(int i = operands.size() - 1; i >= 0; i--) {
        current_precedence = operand_level(operands[i]);
        if(current_precedence < minimum_precedence) {
            split_index = i;
            minimum_precedence = current_precedence;
        }
    }

    nodes.push_back(Node(operands[split_index]));
    Node* root = &nodes.back();

    std::cerr << "Minimum precedence of " << (int)operands[split_index] << "\n";

    std::vector<char> left_operands(operands.begin(), operands.begin() + split_index);
    std::vector<Node*> left_terms(terms.begin(), terms.begin() + split_index + 1);
    std::cerr << "Left ops and term\n";
    std::cerr << "Ops : ";
    for(auto c : left_operands) {
        std::cerr << "\'" << c << "\' ";
    }
    std::cerr << "\nTerm : ";
    for(auto& str : left_terms) {
        std::cerr << "\"" << str << "\" ";
    }

    std::vector<char> right_operands(operands.begin() + split_index + 1, operands.end());
    std::vector<Node*> right_terms(terms.begin() + split_index + 1, terms.end());
    std::cerr << "\nLeft ops and term\n";
    std::cerr << "Ops : ";
    for(auto c : left_operands) {
        std::cerr << "\'" << c << "\' ";
    }
    std::cerr << "\nTerm : ";
    for(auto& str : left_terms) {
        std::cerr << "\"" << str << "\" ";
    }

    
    root->left = assemble_tree(left_operands, left_terms);
    root->right = assemble_tree(right_operands, right_terms);
    return root;
}

// this function assume str_beg are opening parenthesis ( a.k.a "(" )
int search_end_of_parenthesis(const char* str_beg, const char* str_end) {
    const char* str_iter = str_beg;
    int result = -1;
    int opened_parenthesis = 1;
    while((str_iter < str_end) && (opened_parenthesis != 0)) {
        switch (*(++str_iter))
        {
        case '(' :
            ++opened_parenthesis;
            break;

        case ')' :
            --opened_parenthesis;
            break;

        default:
            break;
        }
    }
    return ((opened_parenthesis == 0) ? (str_iter - str_beg) : result);
}

struct equation_split_result {
    std::vector<char> operands;
    std::vector<std::string_view> terms;
};

/*
Splits equation from
example "10 + 20 * (30 - 10) - 98 / 1"
to

Operands = {'+', '*', '-', '/'}
terms = {"10", "20", "(30 - 1)", "98", "1"}
*/
std::optional<equation_split_result> equation_split(const char* eqstr_beg, const char* eqstr_end) { // eqstr = equation string
    std::vector<char> operands;
    std::vector<std::string_view> terms;
    char c;
    Expect next_token_expectation = Expect::TERM;
    const char* eqstr_iter = eqstr_beg;
    while(eqstr_iter < eqstr_end) {
        std::cerr << "Iter at : " << (eqstr_iter - eqstr_beg) << std::endl;
        c = *eqstr_iter;
        if(std::isdigit(c)) {
            std::cerr << "Digit" << std::endl;
            if(next_token_expectation != Expect::TERM) {
                std::cerr << "Unexpected token, token at position : " << (eqstr_iter - eqstr_beg) 
                          << "\nIs a TERM, while current expectation is OPERAND" << std::endl;
                return std::nullopt;
            }
            const char* num_beg = eqstr_iter;
            while((eqstr_iter + 1) < eqstr_end) {
                ++eqstr_iter;
                if(!std::isdigit(*(eqstr_iter))) break;
            }
            if(eqstr_iter == num_beg) ++eqstr_iter;
            terms.push_back(std::string_view(num_beg, (eqstr_iter - num_beg)));
            next_token_expectation = Expect::OPERAND;
            continue;

        } else if (c == '(') {
            const char* subequ_beg = eqstr_iter; // subequ = sub-equation or an equation in between parenthesis
            int end_parenthesis_pos = search_end_of_parenthesis(subequ_beg, eqstr_end);
            if(end_parenthesis_pos == -1) {
                std::cerr << "Sub Equation : \"" << std::string_view(subequ_beg, (eqstr_end - subequ_beg))
                          << "\", has no closing parenthesis" << std::endl;
                return std::nullopt;
            }
            eqstr_iter += end_parenthesis_pos;
            terms.push_back(std::string_view(subequ_beg, (eqstr_iter - subequ_beg + 1)));
            next_token_expectation = Expect::OPERAND;
        } else {
            switch (c)
            {
            case ' ' :
                break;
            case '+' :
            case '-' :
            case '*' :
            case '/' :
            if(next_token_expectation != Expect::OPERAND) {
                std::cerr << "Unexpected token, token at position " << (eqstr_iter - eqstr_beg)
                          << "\nIs an OPERAND, while current token expectation is TERM" << std::endl;
                return std::nullopt;
            }
            operands.push_back(c);
            next_token_expectation = Expect::TERM;
            break;
            

            default:
                std::cerr << "Unknown token at position : " << (eqstr_iter - eqstr_beg) << std::endl;
                goto func_end;
                break;
            }
        }

        ++eqstr_iter;
        continue;
        func_end :
        return std::nullopt;
    }
    
    std::cout << "Operands : ";
    for(auto c : operands) {
        std::cout << '\'' << c << "\' ";
    }
    std::cout << "\nTerms : ";
    for(auto& term : terms) {
        std::cout << '\"' << term << "\" ";
    }
    std::cout << std::endl;
    
    return equation_split_result{ std::move(operands), std::move(terms) };
}

Node* make_tree(std::string_view str) {
    std::vector<char> operands;
    std::vector<Node*> term_nodes;
    {
        auto res = equation_split(str.data(), str.data() + str.size());
        if(!res.has_value()) { return nullptr; }
        std::vector<char>& operands1 = res->operands;
        
        std::vector<std::string_view>& terms = res->terms;

        std::cout << "Operands : ";
        for(auto c : operands1) {
            std::cout << '\'' << c << "\' ";
        }
        std::cout << "\nTerms : ";
        for(auto& term : terms) {
            std::cout << '\"' << term << "\" ";
        }
        std::cout << std::endl;

        if(operands1.empty()) {
            if(terms.empty()) {
                std::cerr << "No operand and no term, can't evaluate anything" << std::endl;
                return nullptr;
            }
            std::cerr << "Operand empty !" << std::endl;
            return make_node_from_num(terms[0].data(), terms[0].data() + terms[0].size());
        }

        if(terms.size() != (operands1.size() + 1)) {
            std::cerr << "Term size doesn't match (operand size) + 1" << std::endl;
            return nullptr;
        }

        operands = std::move(operands1);
        term_nodes.reserve(terms.size());
        
        for(auto& term : terms) {
            if(term[0] == '(') {
                term_nodes.push_back(make_tree(std::string_view(term.data() + 1, (term.size() - 2))));
            } else {
                term_nodes.push_back(make_node_from_num(term.data(), term.data() + term.size()));
            }
        }
    }
    
    return assemble_tree(operands, term_nodes);
}

int evaluate(Node* root) {
    if(!root) return 0;
    if(root->node_type == NodeType::NUMBER) { 
        std::cerr << "This Node is a number Returning " << root->value << "\n";
        return root->value;
    }
    std::cerr << "This node is an operand of " << root->operand << "\n";

    std::cerr << "Going Left !\n";
    int left_value = evaluate(root->left);
    std::cerr << "Done !\n";

    std::cerr << "Going Right !\n";
    int right_value = evaluate(root->right);
    std::cerr << "Done !\n";
    
    switch (root->operand)
    {
    case '*' : return left_value * right_value;
    case '-' : return left_value - right_value;
    case '+' : return left_value + right_value;
    case '/' : return left_value / right_value;
    
    default:
        std::cerr << "Unknown Operand, Evaluation fail" << std::endl;
        return 0;
        break;
    }
}

int main(int argc, const char* argv[]) {
    std::cout << "Enter Equation" << std::endl;
    std::string eq;
    std::getline(std::cin, eq);
    std::cout << "Result : " << evaluate(make_tree(eq)) << std::endl;
}