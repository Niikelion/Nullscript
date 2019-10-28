#ifndef RULES_H
#define RULES_H

#include <nullscript/tokens.h>
#include <regex>
#include <functional>
#include <algorithm>
#include <list>
#include <set>

namespace NULLSCR
{
    class LexicalRule: public Rule
    {
    protected:
        struct RegexPoint
        {
            bool scoped;
            std::regex regex;
            unsigned state,id;
            RegexPoint(const std::regex& reg,unsigned i,unsigned s,bool sc): scoped(sc), regex(reg), state(s), id(i) {};
        };

        struct WordPoint
        {
            static bool checkChar(char c,unsigned mode);
            unsigned id,state,mode,size;
            bool scoped;
            WordPoint(unsigned i,unsigned s,unsigned m,unsigned siz,bool sc): id(i), state(s), mode(m), size(siz), scoped(sc) {};
            WordPoint(const WordPoint&) = default;
        };

        struct WordsTrieNode
        {
            std::unique_ptr<WordsTrieNode> nodes[128];
            unsigned refs;
            std::unique_ptr<WordPoint> value;

            WordsTrieNode* step(char c) const;
            void add(WordsTrieNode* node,char c);

            WordsTrieNode():refs(1) {};
            WordsTrieNode(const WordsTrieNode&) = delete;
            WordsTrieNode(WordsTrieNode&&) noexcept = default;
        };

        class WordsTrie
        {
        private:
            WordsTrieNode root;
        public:
            void add(const std::string& key,const WordPoint& value);

            const WordsTrieNode& getRoot() const;
        };

        struct SavedPoint
        {
            unsigned pos,state,id,size;
            bool scoped;
            SavedPoint(unsigned p,unsigned st,unsigned i,unsigned siz,bool sc): pos(p), state(st),id(i), size(siz), scoped(sc) {};
        };

        struct UnscopedBlock
        {
            unsigned start,end,id;
            UnscopedBlock(unsigned s,unsigned i): start(s), end(0), id(i) {};
        };

        std::function<std::unique_ptr<Token>(const std::string&,unsigned)> creator;
        std::vector<RegexPoint> entry_points;
        WordsTrie keyword_points; ///TODO: add connections to siblings?

        std::unique_ptr<Token> create(const std::string& source,unsigned id,unsigned pos) const;

        void savePoints(const std::string& source, std::vector<SavedPoint>& ps) const;
    public:

        void addParsePoint(const std::regex& reg,unsigned id,unsigned state = States::insert,bool scoped = false);
        void addParsePoint(const std::string& key,unsigned id,unsigned mode = Modes::Keyword,unsigned state = States::insert,bool scoped = false);
        void setTokenCreator(const std::function<std::unique_ptr<Token>(const std::string&,unsigned)>& f);

        enum States
        {
            push,
            pop,
            silentpop,
            insert,
            forget,
            ignore,
            toggle
        };

        enum Modes
        {
            Word,
            Keyword,
            String
        };

        virtual void apply(std::vector<TokenEntity>& source) const override;
    };

    class ComplexRule: public Rule
    {
    public:
        bool deep;
        std::function<void(std::vector<TokenEntity>&)> func;

        virtual void apply(std::vector<TokenEntity>& source) const override;

        ComplexRule(const std::function<void(std::vector<TokenEntity>&)>& f,bool d = false): deep(d), func(f) {};
        ComplexRule(const ComplexRule&) = default;
        ComplexRule(ComplexRule&&) noexcept = default;
    };

    class MergingLayer
    {
    public:
        struct TypesTrieNode
        {
            TypesTrieNode* nodes[128];
            unsigned refs;
            int value;

            TypesTrieNode& operator = (TypesTrieNode&& t) noexcept;

            TypesTrieNode* step(unsigned k) const;
            void set(unsigned k,TypesTrieNode* val);

            TypesTrieNode():refs(1),value(-1)
            {
                for(auto& i: nodes)
                    i = nullptr;
            };
            TypesTrieNode(int v):refs(0), value(v) {};
            TypesTrieNode(const TypesTrieNode&) = delete;
            TypesTrieNode(TypesTrieNode&&) noexcept = default;
        };
        struct TypePoint
        {
            unsigned type,begin,end;
            TypePoint(unsigned b,unsigned e,unsigned t): type(t), begin(b), end(e) {};
            TypePoint(): type(0), begin(0), end(0) {};
        };
    private:
        class TypesTrie
        {
        private:
            std::set<TypesTrieNode*> nodes;
            TypesTrieNode root;
        public:
            TypesTrieNode* add(const std::vector<unsigned>& key,int value);
            TypesTrieNode* append(TypesTrieNode* target,const std::vector<unsigned>& key,int value);
            void connect(const std::vector<unsigned>& key,TypesTrieNode* target);
            void connect(TypesTrieNode* start,const std::vector<unsigned>& key,TypesTrieNode* target);

            const TypesTrieNode* getRoot() const;
            TypesTrieNode* getRoot();

            TypesTrie() = default;
            TypesTrie(const TypesTrie&) = delete;
            TypesTrie(TypesTrie&&) noexcept;

            ~TypesTrie();
        };

        TypesTrie type_points;

        void savePoints(const std::vector<TypePoint>& in,std::vector<TypePoint>& out) const;
    public:
        TypesTrieNode* addTypePath(const std::vector<unsigned>& path,int v);
        TypesTrieNode* appendTypePath(TypesTrieNode* target,const std::vector<unsigned>& path,int v);
        void connectTypePath(const std::vector<unsigned>& path,TypesTrieNode* target);
        void connectTypePath(TypesTrieNode* start,const std::vector<unsigned>& path,TypesTrieNode* target);

        std::vector<TypePoint> apply(const std::vector<TypePoint>& in) const;

        MergingLayer() = default;
        MergingLayer(const MergingLayer&) = delete;
        MergingLayer(MergingLayer&&) noexcept = default;
    };

    class LayeredMergingRule: public Rule
    {
    private:
        std::function<std::unique_ptr<Token>(unsigned,unsigned,unsigned,const std::vector<TokenEntity>&)> merger;
        std::unique_ptr<Token>merge(unsigned begin,unsigned end,unsigned type,const std::vector<TokenEntity>& source) const;
    public:
        std::vector<MergingLayer> layers;
        bool deep;
        void setTokenMerger(const std::function<std::unique_ptr<Token>(unsigned,unsigned,unsigned,const std::vector<TokenEntity>&)>& f);

        virtual void apply(std::vector<TokenEntity>& source) const override;
        LayeredMergingRule(): deep(false) {};
    };
}

#endif // RULES_H
