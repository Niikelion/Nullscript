#ifndef NULLSCRIPT_H
#define NULLSCRIPT_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <exception>
#include <typeindex>

namespace NULLSCR
{
    class Token
    {
    private:
        unsigned pos;
    public:
        unsigned getPos() const noexcept;
        void setPos(unsigned p);

        virtual std::unique_ptr<Token> clone() const = 0;
        virtual std::type_index getType() const = 0;
        virtual char const* getName() const
        {
            return "";
        }

        template<typename T> T* as()
        {
            return dynamic_cast<T*>(this);
        }

        template<typename T> T& forceAs()
        {
            return *reinterpret_cast<T*>(this);
        }

        Token(): pos(0) {};
        virtual ~Token() = default;
    };

    class TokenizerException: public std::exception
    {
    private:
        std::string err_;
    public:
        const char* what() const noexcept;

        TokenizerException(unsigned pos,const std::string& error);
        TokenizerException(const std::string& error): err_(error) {};
    };

    class TokenEntity
    {
    public:
        std::unique_ptr<Token> token;
        unsigned type;

        TokenEntity& operator = (TokenEntity&&) noexcept = default;

        TokenEntity(): token(),type(0) {};
        TokenEntity(std::unique_ptr<Token>&& to,unsigned ty): token(std::move(to)), type(ty) {};
        TokenEntity(const TokenEntity&) = delete;
        TokenEntity(TokenEntity&&) noexcept = default;
    };

    class Rule
    {
    public:
        virtual void apply(std::vector<TokenEntity>& data) const = 0;
    };

    class Stage
    {
    private:
        std::string name_;
    public:
        std::vector<std::unique_ptr<Rule>> rules;

        void apply(std::vector<TokenEntity>& source) const;

        std::string getName() const;
        void setName(const std::string& new_name);

        Stage(const std::string& name): name_(name) {};
        Stage(Stage&&) noexcept = default;
    };
    class Tokenizer
    {
        std::vector<std::unique_ptr<Stage>> stages;
    public:
        bool addStage(const std::string& name);
        Stage& getStage(const std::string& name) const;
        Stage* findStage(const std::string& name) const;

        std::vector<TokenEntity> tokenize(const std::string& source) const;
    };

    namespace Interpreter
    {
        namespace Actions
        {
            enum ACTIONS
            {
                None = 0,
                PushState = 1,
                PopState = 2,
                ComplexAction = 4
            };
        }

        template<typename T> class ScopedTrieNode
        {
        private:
            std::map<T,ScopedTrieNode<T>*> nodes;
        public:
            unsigned action;

            ScopedTrieNode<T>* at(T t)
            {
                return nodes.at(t);
            }
            void set(T t,ScopedTrieNode<T>* node)
            {
                nodes[t] = node;
            }
            void remove(T t);
        };
        template<typename T> class ScopedTrie
        {
        private:
            std::set<ScopedTrieNode<T>*> nodes;
            ScopedTrieNode<T> root;
        public:
            ScopedTrieNode<T>* getRoot();

            ScopedTrieNode<T>* addPath(const std::vector<T>& path);
            ScopedTrieNode<T>* appendPath(ScopedTrieNode<T>* origin,const std::vector<T>& path);
        };

        class Frame
        {
        public:
            //
        };

        template<typename T> class State
        {
        public:
            ScopedTrie<T>* parsingScopePaths;
            Frame frame;
        };

        template<typename T> class Interpreter
        {
        private:
            std::map<std::string,ScopedTrie<T>> scopes;
        public:
            //
        };
    }
}

#endif // NULLSCRIPT_H
