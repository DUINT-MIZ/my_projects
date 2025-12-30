#include <vector>
#include <list>
#include <charconv>
#include <string_view>
#include <optional>
#include <limits>
#include <span>

#include <string>
#include <iostream>
#include <chrono>

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
    long double value = 0;
    char operand = '\0';

    Node() = default;
    Node(long double val) : value(val), node_type(NodeType::NUMBER) {}
    Node(char val) : operand(val), node_type(NodeType::OPERAND) {}
};

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


class math_eval {
    // Encapsulate everythin into a class, it may immediately free heap memory after usage
    private :
    std::list<Node> nodes;

    Node* make_node_from_num(const char* num_beg, const char* num_end) {
        Node node;
        node.node_type = NodeType::NUMBER;
        if(std::from_chars(num_beg, num_end, node.value).ec != std::errc{}) {
            #ifdef EQ_EVAL_DBG
            std::cerr << "Can't convert : \"" << std::string(num_beg, (num_end - num_beg)) << "\", to a number !" << std::endl;
            #endif
            return nullptr;
        }
        nodes.push_back(node);
        return &nodes.back();
    }

    Node* assemble_tree(const std::span<char>& operands, const std::span<Node*>& terms) {
        #ifdef EQ_EVAL_DBG
        std::cerr << "Asemble tree" << std::endl;
        #endif
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

        #ifdef EQ_EVAL_DBG
        std::cerr << "Minimum precedence of " << (int)operands[split_index] << "\n";
        #endif

        std::span<char> left_operands(operands.begin(), operands.begin() + split_index);
        std::span<Node*> left_terms(terms.begin(), terms.begin() + split_index + 1);
        #ifdef EQ_EVAL_DBG
        std::cerr << "Left ops and term\n";
        std::cerr << "Ops : ";
        for(auto c : left_operands) {
            std::cerr << "\'" << c << "\' ";
        }
        std::cerr << "\nTerm : ";
        for(auto& str : left_terms) {
            std::cerr << "\"" << str << "\" ";
        }
        #endif

        std::span<char> right_operands(operands.begin() + split_index + 1, operands.end());
        std::span<Node*> right_terms(terms.begin() + split_index + 1, terms.end());

        #ifdef EQ_EVAL_DBG
        std::cerr << "\nLeft ops and term\n";
        std::cerr << "Ops : ";
        for(auto c : left_operands) {
            std::cerr << "\'" << c << "\' ";
        }
        std::cerr << "\nTerm : ";
        for(auto& str : left_terms) {
            std::cerr << "\"" << str << "\" ";
        }
        #endif


        root->left = assemble_tree(left_operands, left_terms);
        root->right = assemble_tree(right_operands, right_terms);
        return root;
    }

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

        auto process_digit = [&](){
            #ifdef EQ_EVAL_DBG
            std::cerr << "Digit" << std::endl;
            #endif

            if(next_token_expectation != Expect::TERM) {
                #ifdef EQ_EVAL_DBG
                std::cerr << "Unexpected token, token at position : " << (eqstr_iter - eqstr_beg) 
                          << "\nIs a TERM, while current expectation is OPERAND" << std::endl;
                #endif
                return false;
            }
            const char* num_beg = eqstr_iter;
            while((++eqstr_iter) < eqstr_end) {
                if(!std::isdigit(*(eqstr_iter))) break;
            }
            if(eqstr_iter == num_beg) ++eqstr_iter;
            terms.push_back(std::string_view(num_beg, (eqstr_iter - num_beg)));
            next_token_expectation = Expect::OPERAND;
            return true;
        };

        while(eqstr_iter < eqstr_end) {
            #ifdef EQ_EVAL_DBG
            std::cerr << "Iter at : " << (eqstr_iter - eqstr_beg) << std::endl;
            #endif
            c = *eqstr_iter;
            if(std::isdigit(c)) {
                if(!process_digit()) return std::nullopt;
                continue;

            } else if (c == '(') {
                const char* subequ_beg = eqstr_iter; // subequ = sub-equation or an equation in between parenthesis
                int end_parenthesis_pos = search_end_of_parenthesis(subequ_beg, eqstr_end);
                if(end_parenthesis_pos == -1) {
                    #ifdef EQ_EVAL_DBG
                    std::cerr << "Sub Equation : \"" << std::string_view(subequ_beg, (eqstr_end - subequ_beg))
                              << "\", has no closing parenthesis" << std::endl;
                    #endif
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
                
                case '-' :
                    if((eqstr_iter + 1) < eqstr_end) {
                        if(std::isdigit(*(eqstr_iter + 1))) {
                            if(!process_digit()) return std::nullopt;
                            break; 
                        }
                    }
                
                case '+' :
                case '*' :
                case '/' :
                if(next_token_expectation != Expect::OPERAND) {
                    #ifdef EQ_EVAL_DBG
                    std::cerr << "Unexpected token, token at position " << (eqstr_iter - eqstr_beg)
                              << "\nIs an OPERAND, while current token expectation is TERM" << std::endl;
                    #endif
                    return std::nullopt;
                }
                operands.push_back(c);
                next_token_expectation = Expect::TERM;
                break;


                default:
                    #ifdef EQ_EVAL_DBG
                    std::cerr << "Unknown token at position : " << (eqstr_iter - eqstr_beg) << std::endl;
                    #endif
                    goto func_end;
                    break;
                }
            }

            ++eqstr_iter;
            continue;
            func_end :
            return std::nullopt;
        }
        #ifdef EQ_EVAL_DBG
        std::cout << "Operands : ";
        for(auto c : operands) {
            std::cout << '\'' << c << "\' ";
        }
        std::cout << "\nTerms : ";
        for(auto& term : terms) {
            std::cout << '\"' << term << "\" ";
        }
        std::cout << std::endl;
        #endif

        return equation_split_result{ std::move(operands), std::move(terms) };
    }

    Node* make_tree(std::string_view str) {
        std::vector<char> operands;
        std::vector<Node*> term_nodes;
        {
            auto res = equation_split(str.data(), str.data() + str.size());
        if(!res.has_value()) { return nullptr; }
        operands = std::move(res->operands);
        
        std::vector<std::string_view>& terms = res->terms;
        term_nodes.reserve(terms.size());
        #ifdef EQ_EVAL_DBG
        std::cout << "Operands : ";
        for(auto c : operands) {
            std::cout << '\'' << c << "\' ";
        }
        std::cout << "\nTerms : ";
        for(auto& term : terms) {
            std::cout << '\"' << term << "\" ";
        }
        std::cout << std::endl;
        #endif

        if(operands.empty()) {
            if(terms.empty()) {
                #ifdef EQ_EVAL_DBG
                std::cerr << "No operand and no term, can't evaluate anything" << std::endl;
                #endif
                return nullptr;
            }
            #ifdef EQ_EVAL_DBG
            std::cerr << "Operand empty !" << std::endl;
            #endif
            return make_node_from_num(terms[0].data(), terms[0].data() + terms[0].size());
        }

        if(terms.size() != (operands.size() + 1)) {
            #ifdef EQ_EVAL_DBG
            std::cerr << "Term size doesn't match (operand size) + 1" << std::endl;
            #endif
            return nullptr;
        }
        
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

    long double evaluate(Node* root) {
        if(!root) return 0;
        if(root->node_type == NodeType::NUMBER) { 
            #ifdef EQ_EVAL_DBG
            std::cerr << "This Node is a number Returning " << root->value << "\n";
            #endif
            return root->value;
        }
        #ifdef EQ_EVAL_DBG
        std::cerr << "This node is an operand of " << root->operand << "\n";

        std::cerr << "Going Left !\n";
        #endif
        long double left_value = evaluate(root->left);
        #ifdef EQ_EVAL_DBG
        std::cerr << "Done !\n";

        std::cerr << "Going Right !\n";
        #endif
        long double right_value = evaluate(root->right);
        #ifdef EQ_EVAL_DBG
        std::cerr << "Done !\n";
        #endif

        switch (root->operand)
        {
        case '*' : return left_value * right_value;
        case '-' : return left_value - right_value;
        case '+' : return left_value + right_value;
        case '/' : return left_value / right_value;
        
        default:
            #ifdef EQ_EVAL_DBG
            std::cerr << "Unknown Operand, Evaluation fail" << std::endl;
            #endif
            return 0;
            break;
        }
    }

    public : 

    long double solve(std::string_view eq) {
        return evaluate(make_tree(eq));
    }
};

int main(int argc, const char* argv[]) {
    std::cout << "Enter Equation" << std::endl;
    std::string eq;
    std::getline(std::cin, eq);
    auto start = std::chrono::high_resolution_clock::now();
    long double res = math_eval().solve(eq);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Result : " << res << std::endl;
    std::cout << "Time take " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " microsecs" << std::endl;
}