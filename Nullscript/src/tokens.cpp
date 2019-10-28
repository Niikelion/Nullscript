#include "tokens.h"

namespace NULLSCR
{
    ScopeToken::ScopeToken(const ScopeToken& target)
    {
        setPos(target.getPos());
        type = target.type;
        tokens.reserve(target.tokens.size());
        for (const auto& i: target.tokens)
            tokens.emplace_back(std::unique_ptr<Token>(i.token->clone()),i.type);
    }

    void printTokens(const std::vector<TokenEntity>& tokens,std::ostream& out,bool recursive,unsigned offset)
    {
        for (const auto& i:tokens)
        {
            std::type_index t = i.token -> getType();
            out << std::string(offset,' ') << "(";
            if (i.token -> getName()[0] != '\0')
            {
                out << i.token -> getName();
            }
            else
            {
                out << t.name();
            }
            out << ":" << i.type << ")";
            if (i.token -> as<StringToken>() != nullptr)
            {
                out << " " << i.token -> as<StringToken>() -> str;
            }
            else if (i.token -> as<ScopeToken>() != nullptr)
            {
                if (recursive)
                {
                    out << ":\n";
                    printTokens(i.token -> as<ScopeToken>() -> tokens,out,true,offset+2);
                }
                out << std::string(offset,' ') << "end\n";
            }
            out << "\n";
        }
    }
}
