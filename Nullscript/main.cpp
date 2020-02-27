#include <iostream>

#include <nullscript/nullscript.h>
#include <nullscript/rules.h>
#include <nullscript/tokens.h>

#include <functional>

using namespace std;
using namespace NULLSCR;

class VariableToken: public TokenBase<VariableToken>
{
public:
    char const* getName() const override
    {
        return "variable";
    }
    string type;
    string name;

    VariableToken(const string& _type,const string& _name)
    {
        type = _type;
        name = _name;
    }
};

enum Ids
{
    None = 0,
    Uniform,
    Type,
    Scope,
    Variable,
    Block,
    Semicolon
};

const char* types[] = {
    "bool","int","uint","float","double",

    "bvec2","ivec2","uvec2","vec2","dvec2",

    "bvec3","ivec3","uvec3","vec3","dvec3",

    "bvec4","ivec4","uvec4","vec4","dvec4",

    "mat2","dmat2","mat2x2","dmat2x2",

    "mat3","dmat3","mat3x3","dmat3x3",

    "mat4","dmat4","mat4x4","dmat4x4",

    "mat2x3","dmat2x3","mat3x2","dmat3x2",

    "mat2x4","dmat2x4","mat4x2","dmat4x2",

    "mat3x4","dmat3x4","mat4x3","dmat4x3"
};

void setupLex(Stage& st)
{
    LexicalRule *rl = new LexicalRule();
    st.rules.push_back(std::unique_ptr<Rule>(rl));

    rl->addParsePoint(std::regex("\\s"),Ids::None,LexicalRule::States::forget,false);
    rl->addParsePoint("#",Ids::None,LexicalRule::Modes::String,LexicalRule::States::push,false);
    rl->addParsePoint("\n",Ids::None,LexicalRule::Modes::String,LexicalRule::States::silentpop,false);

    rl->addParsePoint(";",Ids::Semicolon,LexicalRule::Modes::String,LexicalRule::States::insert,false);

    rl->addParsePoint("{",Ids::Scope,LexicalRule::Modes::String,LexicalRule::States::push,true);
    rl->addParsePoint("}",Ids::Scope,LexicalRule::Modes::String,LexicalRule::States::pop,true);

    rl->addParsePoint("uniform",Ids::Uniform,LexicalRule::Modes::String,LexicalRule::States::insert,false);
    for (auto i:types)
        rl->addParsePoint(i,Ids::Type,LexicalRule::Modes::String,LexicalRule::States::insert,false);

    rl->setTokenCreator([](const string& source,unsigned type){
                            switch (type)
                            {
                            case Ids::Scope:
                                {
                                    return std::unique_ptr<Token>(new ScopeToken(0));
                                    break;
                                }
                            }
                            return std::unique_ptr<Token>(new StringToken(0,source));
                        });
}

void setupMrg(Stage& st)
{
    LayeredMergingRule *mrg = new LayeredMergingRule();
    st.rules.push_back(std::unique_ptr<Rule>(mrg));
    mrg->deep = true;
    mrg->layers.push_back(MergingLayer());
    mrg->layers[0].addTypePath({Ids::Type,Ids::None,Ids::Semicolon},Ids::Variable);

    mrg->setTokenMerger([](unsigned b,unsigned e,unsigned t,const std::vector<TokenEntity>& source)
                        {
                            switch (t)
                            {
                                case Ids::Variable:
                                {
                                    return std::unique_ptr<Token>(new VariableToken(source[b].token->forceAs<StringToken>().str,
                                                                                    source[b+1].token->forceAs<StringToken>().str));
                                }
                            }
                            return std::unique_ptr<Token>(new StringToken(0,""));
                        });
}

void setup(Tokenizer& t)
{
    t.addStage("lex");
    t.addStage("mrg");

    setupLex(t.getStage("lex"));
    setupMrg(t.getStage("mrg"));
}

int main()
{
    Tokenizer t;
    setup(t);

    string a;
    string l;
    getline(cin,l);
    a = l;
    while (!cin.eof() && l.length() > 0)
    {
        getline(cin,l);
        a +='\n' + l;
    }

    auto v = t.tokenize(a);
    printTokens(v,cout,true,0);

    return 0;
}
