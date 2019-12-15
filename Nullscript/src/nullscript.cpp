#include "nullscript.h"
#include "tokens.h"

namespace NULLSCR
{
    unsigned Token::getPos() const noexcept
    {
        return pos;
    }
    void Token::setPos(unsigned p)
    {
        pos = p;
    }

    const char* TokenizerException::what() const noexcept
    {
        return err_.c_str();
    }

    TokenizerException::TokenizerException(unsigned pos,const std::string& error)
    {
        err_ = error;
        err_ += " at: ";
        err_ += std::to_string(pos);
    }

    void Stage::apply(std::vector<TokenEntity>& source) const
    {
        try
        {
            for (const auto& rule: rules)
            {
                rule -> apply(source);
            }
        }
        catch (TokenizerException& e)
        {
            std::string err = getName();
            err += ": ";
            err += e.what();
            throw TokenizerException(err);
        }
    }

    std::string Stage::getName() const
    {
        return name_;
    }

    void Stage::setName(const std::string& name)
    {
        name_ = name;
    }

    std::vector<TokenEntity> Tokenizer::tokenize(const std::string& source) const
    {
        std::vector<TokenEntity> ret;


        StringToken* tmp = new StringToken(0,source);

        ret.emplace_back(std::unique_ptr<Token>(tmp),0);

        for (const auto& stage: stages)
        {
            stage -> apply(ret);
        }
        return std::move(ret);
    }

    bool Tokenizer::addStage(const std::string& name)
    {
        for (const auto& i:stages)
        {
            if (i -> getName() == name)
                return false;
        }
        stages.emplace_back(new Stage(name));
        return true;
    }
    Stage& Tokenizer::getStage(const std::string& name) const
    {
        for (const auto& i:stages)
        {
            if (i -> getName() == name)
                return *i;
        }
        throw std::logic_error("Tokenizer exception: stage not found");
    }
    Stage* Tokenizer::findStage(const std::string& name) const
    {
        for (const auto& i:stages)
        {
            if (i -> getName() == name)
                return i.get();
        }
        return nullptr;
    }
}
