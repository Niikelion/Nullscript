#include <iostream>

#include <nullscript/nullscript.h>
#include <nullscript/rules.h>
#include <nullscript/tokens.h>

#include <functional>

using namespace std;
using namespace NULLSCR;

class KeywordToken: public StringToken
{
public:
    virtual char const* getName() const override
    {
        return "keyword";
    }
    virtual std::type_index getType() const override
    {
        return typeid(KeywordToken);
    }
    KeywordToken(unsigned pos,const std::string& s): StringToken(pos,s) {};
    KeywordToken(const KeywordToken&) = default;
    KeywordToken(KeywordToken&&) noexcept = default;
};
class CommentToken: public NULLSCR::StringToken
{
public:
    virtual std::unique_ptr<Token> clone() const override
    {
        return std::unique_ptr<CommentToken>(new CommentToken(*this));
    }
    virtual std::type_index getType() const override
    {
        return typeid(CommentToken);
    }
    using StringToken::StringToken;
};

class SemicolonToken: public TokenBase<SemicolonToken> {};

std::string offset(unsigned off)
{
    return std::string(off*2,' ');
}

void displayTokens(const std::vector<TokenEntity>& tokens,unsigned off = 0)
{
    for (const auto& i: tokens)
    {
        cout << offset(off) << i.type << endl;
        std::type_index t = i.token -> getType();
        if (t == typeid(KeywordToken))
        {
            cout << offset(off) << "Keyword token: " << i.token -> forceAs<KeywordToken>().str << endl;
        }
        else if (t == typeid(CommentToken))
        {
            cout << offset(off) << "Comment token: " << i.token -> forceAs<CommentToken>().str << endl;
        }
        else if (t == typeid(SemicolonToken))
        {
            cout << offset(off) << "Semicolon token" << endl;
        }
        else if (t == typeid(StringToken))
        {
            cout << offset(off) << "Raw token: " << i.token -> forceAs<StringToken>().str << endl;
        }
        else
        {
            cout << "Unknown token: " << t.name() << endl;
        }
    }
}


enum Ids
{
    None = 0,
    Keyword,
    Wee
};

std::unique_ptr<Token> creator(const std::string& source,unsigned id)
{
    switch (id)
    {
    case Ids::Keyword:
        {
            return std::unique_ptr<Token>(new KeywordToken(0,source));
            break;
        }
    }
    return std::move(std::unique_ptr<Token>(new StringToken(0,source)));
}

std::unique_ptr<Token> mergerFunc(unsigned b,unsigned e,unsigned t,const std::vector<TokenEntity>& tokens)
{
    return std::move(std::unique_ptr<Token>(new StringToken(tokens[b].token -> getPos(),"uwu")));
}

void setup1(Stage* stg)
{
    LexicalRule* lr = new LexicalRule();

    lr -> addParsePoint("key",Ids::Keyword,LexicalRule::Modes::String,LexicalRule::States::insert,false);
    lr -> addParsePoint("lelkey",0,LexicalRule::Modes::String,LexicalRule::States::forget,false);
    lr -> setTokenCreator(creator);

    stg -> rules.emplace_back(lr);
}

void setup2(Stage* mrg)
{
    LayeredMergingRule* lmr = new LayeredMergingRule();

    //lmr -> deep = true;
    lmr -> layers.emplace_back();
    lmr -> layers[0].addTypePath({Ids::Keyword,Ids::Keyword},Ids::Wee);
    lmr -> setTokenMerger(mergerFunc);

    mrg -> rules.emplace_back(lmr);
}

int main()
{
    string source = R"TXT(testkeykeyhehkeykeykeylelkeykey)TXT";

    cout << source << endl;

    Tokenizer interpreter;
    interpreter.addStage("1");
    interpreter.addStage("2");

    setup1(interpreter.findStage("1"));
    setup2(interpreter.findStage("2"));

    vector<TokenEntity> tokens = std::move(interpreter.tokenize(source));

    displayTokens(tokens);

    return 0;
}
