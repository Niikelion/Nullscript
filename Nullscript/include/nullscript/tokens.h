#ifndef TOKENS_H
#define TOKENS_H

#include <nullscript/nullscript.h>
#include <iostream>

namespace NULLSCR
{
    template<typename T> class TokenBase: public Token
    {
    public:
        virtual std::unique_ptr<Token> clone() const override
        {
            return std::unique_ptr<T>(new T(reinterpret_cast<const T&>(*this)));
        }
        virtual std::type_index getType() const override
        {
            return typeid(T);
        }
    };

    class StringToken: public TokenBase<StringToken>
    {
    public:
        virtual char const* getName() const
        {
            return "string";
        }
        std::string str;

        StringToken(unsigned pos,const std::string& s): str(s) { setPos(pos); };
        StringToken(const StringToken&) = default;
        StringToken(StringToken&&) noexcept = default;
    };

    class ScopeToken: public TokenBase<ScopeToken>
    {
    public:
        virtual char const* getName() const
        {
            return "scope";
        }
        unsigned type;
        std::vector<TokenEntity> tokens;

        ScopeToken(unsigned pos):type(0) { setPos(pos); };
        ScopeToken(unsigned pos,unsigned t):type(t) { setPos(pos); };
        ScopeToken(const ScopeToken&);
        ScopeToken(ScopeToken&&) noexcept = default;
    };

    void printTokens(const std::vector<TokenEntity>& tokens,std::ostream& out,bool recursive = false,unsigned off = 0);
}

#endif // TOKENS_H
