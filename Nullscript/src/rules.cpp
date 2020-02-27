#include "nullscript/rules.h"
#include <tuple>

namespace NULLSCR
{
    std::unique_ptr<Token> LexicalRule::create(const std::string& source,unsigned id,unsigned pos) const
    {
        std::unique_ptr<Token> ret(std::move(creator(source,id)));
        ret -> setPos(pos);
        return std::move(ret);
    }
    void LexicalRule::savePoints(const std::string& source, std::vector<SavedPoint>& ps) const
    {
        ps.clear();
        auto send = std::sregex_iterator();

        //use entry points

        for (const auto& point: entry_points)
        {
            auto beg = std::sregex_iterator(source.begin(),source.end(),point.regex);
            for (; beg != send; ++beg)
            {
                auto& i = *beg;
                ps.emplace_back(    i.position(),
                                    point.state,
                                    point.id,
                                    i.length(),
                                    point.scoped);
            }
        }

        //use keywords trie

        std::vector<const WordsTrieNode*> matches;
        WordsTrieNode* tmp;
        matches.emplace_back(&keyword_points.getRoot());

        for (unsigned i=0; i < source.size(); ++i)
        {
            for (int j=0; j<static_cast<int>(matches.size()); ++j)
            {
                tmp = matches[j] -> step(source[i]);
                if (tmp == nullptr || matches[j] -> value)
                {
                    //try adding previous state to points

                    if (matches[j] -> value)
                    {
                        //check mode, determine need to push, push
                        bool insert = true;
                        std::vector<WordPoint>* s = matches[j] -> value.get();
                        for (auto v:*s)
                        {
                            if (i - v.size >= 1 && !WordPoint::checkChar(source[i -1 - v.size],v.mode))
                            {
                                insert = false;
                            }
                            if (!WordPoint::checkChar(source[i],v.mode))
                            {
                                insert = false;
                            }
                            if (insert)
                            {
                                ps.emplace_back(    i - v.size,
                                                    v.state,
                                                    v.id,
                                                    v.size,
                                                    v.scoped);
                            }
                        }
                    }
                    if (tmp == nullptr) //erase match
                    {
                        matches.erase(matches.begin()+j);
                        --j;
                    }
                    else
                    {
                        matches[j] = tmp;
                    }
                }
                else
                {
                    matches[j] = tmp;
                }
            }
            matches.emplace_back(&keyword_points.getRoot());
        }
        for (int j=0; j<static_cast<int>(matches.size()); ++j)
        {
            std::vector<WordPoint>* s = matches[j] -> value.get();
            if (s != nullptr)
            {
                for (auto v:*s)
                {
                    if (source.size() -1 - v.size < 0 || WordPoint::checkChar(source[source.size() -1 - v.size],v.mode))
                    {
                        ps.emplace_back(    source.size() - v.size,
                                                        v.state,
                                                        v.id,
                                                        v.size,
                                                        v.scoped);
                    }
                }
            }
        }
        std::sort(ps.begin(),ps.end(),[](const SavedPoint& a,const SavedPoint& b)
        {
            return (a.pos == b.pos) ? (a.size > b.size) : (a.pos < b.pos);
        });
    }

    bool LexicalRule::WordPoint::checkChar(char c,unsigned mode)
    {
        switch (mode)
        {
        case Modes::Word:
            {
                return ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9'));
                break;
            }
        case Modes::Keyword:
            {
                return ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9') && c != '_');
                break;
            }
        }
        return true;
    }

    LexicalRule::WordsTrieNode* LexicalRule::WordsTrieNode::step(char c) const
    {
        if (c > 0 && c < 128)
        {
            if (nodes[static_cast<unsigned char>(c)])
                return nodes[static_cast<unsigned char>(c)].get();
            return nullptr;
        }
        return nullptr;
    }

    void LexicalRule::WordsTrieNode::add(LexicalRule::WordsTrieNode* node,char c)
    {
        if (c > 0 && c < 128)
            nodes[static_cast<unsigned char>(c)].reset(node);
    }

    void LexicalRule::WordsTrie::add(const std::string& key,const WordPoint& value)
    {
        WordsTrieNode *current = &root,*next;
        for (auto i:key)
        {
            next = current -> step(i);
            if (next == nullptr)
            {
                current -> add(new LexicalRule::WordsTrieNode(),i);
                next = current -> step(i);
            }
            current = next;
        }
        if (!current -> value)
        {
            current = &root;
            for (unsigned i=0; i < key.size(); ++i)
            {
                ++(current -> refs);
                current = current -> step(key[i]);
            }
        }
        if (current != nullptr)
        {
            if (!(current -> value))
                current -> value.reset(new std::vector<WordPoint>());
            current -> value -> push_back(value);
        }
    }

    const LexicalRule::WordsTrieNode& LexicalRule::WordsTrie::getRoot() const
    {
        return root;
    }

    void LexicalRule::apply(std::vector<TokenEntity>& source) const
    {
        if (creator)
        {
            for (int i=0; i < static_cast<int>(source.size()); ++i)
            {
                if (source[i].token -> getType() == typeid(StringToken) && source[i].type == 0)
                {
                    StringToken* src = &source[i].token -> forceAs<StringToken>();
                    std::vector<SavedPoint> points;
                    savePoints(src -> str,points);
                    if (points.size())
                    {
                        TokenEntity tmp = std::move(source[i]);
                        std::vector<TokenEntity*> stack_list;
                        source.erase(source.begin()+i);

                        std::unique_ptr<UnscopedBlock> sb;

                        unsigned offset = 0,lastOffset = 0;
                        unsigned inserted = 0;
                        i -= 1;
                        bool advance,rush;
                        for (unsigned j=0; j < points.size(); ++j)
                        {
                            if (offset <= points[j].pos )
                            {
                                advance = true;
                                rush = true;
                                if (sb)
                                {
                                    advance = false;
                                    rush = false;
                                    if (points[j].state == States::pop || points[j].state == States::silentpop || points[j].state == States::toggle)
                                    {
                                        if (points[j].id == sb -> id) //found pop corresponding to unscoped push
                                        {
                                            UnscopedBlock* sbp = sb.get();
                                            sbp -> end = points[j].pos;
                                            if (points[j].state != States::silentpop)
                                            {
                                                if (stack_list.size()) // push to back of stack
                                                {
                                                    std::unique_ptr<Token> tmpu = std::move(create(src -> str.substr(sbp -> start,sbp -> end - sbp -> start + points[j].size),points[j].id,points[j].pos+src -> getPos()));
                                                    if (tmpu)
                                                        stack_list.back() -> token -> forceAs<ScopeToken>().tokens.emplace_back(std::move(tmpu),points[j].id);
                                                }
                                                else // push to source
                                                {
                                                    std::unique_ptr<Token> tmpu = std::move(create(src -> str.substr(sbp -> start,sbp -> end - sbp -> start + points[j].size),points[j].id,points[j].pos+src -> getPos()));
                                                    if (tmpu)
                                                    {
                                                        ++inserted;
                                                        source.emplace(source.begin()+i+inserted,std::move(tmpu),points[j].id);
                                                    }
                                                }
                                            }
                                            sb.release();
                                            advance = true;
                                            rush = true;
                                        }
                                    }
                                }
                                else
                                {
                                    if ( lastOffset != points[j].pos && points[j].state != States::ignore) //found unmatched part
                                    {
                                        //insert raw string in between
                                        if (stack_list.size()) //push to back of stack
                                        {
                                            std::unique_ptr<Token> tmpu = std::move(create(src -> str.substr(lastOffset,points[j].pos - lastOffset),0,lastOffset + src -> getPos()));
                                            if (tmpu)
                                                stack_list.back() -> token -> forceAs<ScopeToken>().tokens.emplace_back(std::move(tmpu),0);
                                        }
                                        else // push to source
                                        {
                                            std::unique_ptr<Token> tmpu = std::move(create(src -> str.substr(lastOffset,points[j].pos - lastOffset),0,lastOffset + src -> getPos()));
                                            if (tmpu)
                                            {
                                                ++inserted;
                                                source.emplace(source.begin()+i+inserted,std::move(tmpu),0);
                                            }
                                        }
                                    }
                                    //process point instruction
                                    switch (points[j].state)
                                    {
                                    case States::push:
                                        {
                                            if (points[j].scoped) //push scope
                                            {
                                                if (stack_list.size()) //push to back of stack
                                                {
                                                    ScopeToken* st = &stack_list.back() -> token -> forceAs<ScopeToken>();
                                                    std::unique_ptr<Token> tmpu = std::move(create(src -> str.substr(points[j].pos,points[j].size),points[j].id,points[j].pos+src -> getPos()));
                                                    if (tmpu && dynamic_cast<ScopeToken*>(tmpu.get()) != nullptr)
                                                    {
                                                        st -> tokens.emplace_back(std::move(tmpu),points[j].id);
                                                        stack_list.emplace_back(&st -> tokens.back());
                                                    }
                                                }
                                                else //push to source
                                                {
                                                    std::unique_ptr<Token> tmpu = std::move(create(std::string(),points[j].id,points[j].pos+src -> getPos()));
                                                    if (tmpu && dynamic_cast<ScopeToken*>(tmpu.get()) != nullptr)
                                                    {
                                                        ++inserted;
                                                        source.emplace(source.begin()+i+inserted,std::move(tmpu),points[j].id);
                                                        stack_list.emplace_back(&(*(source.begin()+i+inserted)) );
                                                    }
                                                }
                                            }
                                            else //prepare unscoped block
                                            {
                                                sb.reset(new UnscopedBlock(points[j].pos,points[j].id));
                                            }
                                            break;
                                        }
                                    case States::pop:
                                        {
                                            if (stack_list.size() && stack_list.back() -> type == points[j].id) //matching push pop
                                            {
                                                stack_list.pop_back();
                                            }
                                            else //error
                                            {
                                                throw TokenizerException(points[j].pos+src -> getPos(),"Scope boundaries type mismatch");
                                            }
                                            break;
                                        }
                                    case States::silentpop:
                                        {
                                            break;
                                        }
                                    case States::toggle:
                                        {
                                            sb.reset(new UnscopedBlock(points[j].pos,points[j].id));
                                            break;
                                        }
                                    case States::insert: //insert token created from matched sequence
                                        {
                                            if (stack_list.size()) //push to back of stack
                                            {
                                                std::unique_ptr<Token> tmpu = std::move(create(src -> str.substr(points[j].pos,points[j].size),points[j].id,points[j].pos+src -> getPos()));
                                                if (tmpu)
                                                {
                                                    stack_list.back() -> token -> forceAs<ScopeToken>().tokens.emplace_back(std::move(tmpu),points[j].id);
                                                }
                                            }
                                            else //push to source
                                            {
                                                std::unique_ptr<Token> tmpu = std::move(create(src -> str.substr(points[j].pos,points[j].size),points[j].id,points[j].pos+src -> getPos()));
                                                if (tmpu)
                                                {
                                                    ++inserted;
                                                    source.emplace(source.begin()+i+inserted,std::move(tmpu),points[j].id);
                                                }
                                            }
                                            break;
                                        }
                                    case States::forget:
                                        {
                                            break;
                                        }
                                    case States::ignore:
                                        {
                                            advance = false;
                                            break;
                                        }
                                    }
                                }
                                if (rush)
                                    offset = points[j].pos + points[j].size;
                                else
                                    offset = points[j].pos;
                                if (advance)
                                    lastOffset = offset;
                            }
                        }
                        if (stack_list.size() || sb)
                        {
                            //err
                        }
                        if (offset != src -> str.size())
                        {
                            ++inserted;
                            source.emplace(source.begin()+i+inserted,std::unique_ptr<Token>(new StringToken(offset + src -> getPos(),src -> str.substr(offset,src -> str.size() - offset))),0);
                        }
                        i += inserted;
                    }
                }
            }
        }
    }

    void LexicalRule::addParsePoint(const std::regex& reg,unsigned id,unsigned state,bool scoped)
    {
        entry_points.emplace_back(reg,id,state,scoped);
    }
    void LexicalRule::addParsePoint(const std::string& key,unsigned id,unsigned mode,unsigned state,bool scoped)
    {
        keyword_points.add(key,WordPoint(id,state,mode,key.size(),scoped));
    }
    void LexicalRule::setTokenCreator(const std::function<std::unique_ptr<Token>(const std::string&,unsigned)>& f)
    {
        creator = f;
    }

    void ComplexRule::apply(std::vector<TokenEntity>& source) const
    {
        if (deep)
        {
            for (auto& i: source)
            {
                ScopeToken* sc = i.token -> as<ScopeToken>();
                if (sc != nullptr)
                {
                    apply(sc -> tokens);
                }
            }
        }
        if (func)
            func(source);
    }

    MergingLayer::TypesTrieNode* MergingLayer::TypesTrieNode::step(unsigned k) const
    {
        if (k >= 128)
            return nullptr;
        return nodes[k];
    }

    void MergingLayer::TypesTrieNode::set(unsigned k,MergingLayer::TypesTrieNode* val)
    {
        if (k < 128)
            nodes[k] = val;
    }

    const MergingLayer::TypesTrieNode* MergingLayer::TypesTrie::getRoot() const
    {
        return &root;
    }

    MergingLayer::TypesTrieNode* MergingLayer::TypesTrie::getRoot()
    {
        return &root;
    }

    MergingLayer::TypesTrieNode* MergingLayer::TypesTrie::add(const std::vector<unsigned>& path,int value)
    {
        return append(getRoot(),path,value);
    }

    MergingLayer::TypesTrieNode* MergingLayer::TypesTrie::append(MergingLayer::TypesTrieNode* target,const std::vector<unsigned>& path,int value)
    {
        for (auto i:path)
            if (i >= 128)
                return nullptr;
        MergingLayer::TypesTrieNode *c = target,*n = target;
        for (auto i:path)
        {
            n = c -> step(i);
            if (n == nullptr)
            {
                MergingLayer::TypesTrieNode* t = new MergingLayer::TypesTrieNode();
                c -> set(i,t);
                nodes.emplace(t);
                c = t;
            }
            else
            {
                c = n;
            }
        }
        c -> value = value;
        return c;
    }

    void MergingLayer::TypesTrie::connect(const std::vector<unsigned>& path,MergingLayer::TypesTrieNode* target)
    {
        connect(getRoot(),path,target);
    }

    void MergingLayer::TypesTrie::connect(MergingLayer::TypesTrieNode* start,const std::vector<unsigned>& path,MergingLayer::TypesTrieNode* target)
    {
        for (auto i:path)
            if (i >= 128)
                return;
        MergingLayer::TypesTrieNode *c = start,*n = start;
        unsigned i=0;
        for (; i+1<path.size(); ++i)
        {
            n = c -> step(path[i]);
            if (n == nullptr)
            {
                MergingLayer::TypesTrieNode* t = new MergingLayer::TypesTrieNode();
                c -> set(path[i],t);
                nodes.emplace(t);
                c = t;
            }
            else
            {
                c = n;
            }
        }
        c -> set(path[i],target);
    }


    MergingLayer::TypesTrieNode& MergingLayer::TypesTrieNode::operator = (MergingLayer::TypesTrieNode&& t) noexcept
    {
        refs = t.refs;
        value = t.value;
        for (unsigned i=0; i<128; ++i)
        {
            nodes[i] = t.nodes[i];
            t.nodes[i] = nullptr;
        }
        return *this;
    }

    MergingLayer::TypesTrie::TypesTrie(MergingLayer::TypesTrie&& t) noexcept
    {
        nodes = std::move(t.nodes);
        t.nodes.clear();
        root = std::move(t.root);
    }

    MergingLayer::TypesTrie::~TypesTrie()
    {
        for (auto i:nodes)
        {
            delete i;
        }
    }

    void MergingLayer::savePoints(const std::vector<MergingLayer::TypePoint>& in,std::vector<MergingLayer::TypePoint>& out) const
    {
        out.clear();

        std::vector<std::pair<unsigned,const MergingLayer::TypesTrieNode*>> states;

        for (unsigned i=0; i<in.size(); ++i)
        {
            states.emplace_back(0,type_points.getRoot());
            const MergingLayer::TypesTrieNode* next;
            for (int j=0; static_cast<unsigned>(j) < states.size(); ++j)
            {
                next = states[j].second -> step(in[i].type);
                if (states[j].second -> value != -1)
                {
                    out.emplace_back(i - states[j].first,i,states[j].second -> value);
                }
                if (next == nullptr)
                {
                    states.erase(states.begin()+j);
                    --j;
                }
                else
                {
                    ++states[j].first;
                    states[j].second = next;
                }
            }
        }
        for (const auto& i:states)
        {
            if (i.second != nullptr && i.second -> value != -1)
            {
                out.emplace_back(in.size() - i.first,in.size(),i.second -> value);
            }
        }
        std::sort(out.begin(),out.end(),[](const MergingLayer::TypePoint& a,const MergingLayer::TypePoint& b)
        {
            return (a.begin < b.begin) ? true : (a.begin == b.begin ? (a.end - a.begin > b.end - b.begin) : false);
        });
    }

    MergingLayer::TypesTrieNode* MergingLayer::addTypePath(const std::vector<unsigned>& path,int v)
    {
        return type_points.add(path,v);
    }

    MergingLayer::TypesTrieNode* MergingLayer::appendTypePath(MergingLayer::TypesTrieNode* target,const std::vector<unsigned>& path,int v)
    {
        return type_points.append(target,path,v);
    }

    void MergingLayer::connectTypePath(const std::vector<unsigned>& path,MergingLayer::TypesTrieNode* node)
    {
        type_points.connect(path,node);
    }

    void MergingLayer::connectTypePath(MergingLayer::TypesTrieNode* start,const std::vector<unsigned>& path,MergingLayer::TypesTrieNode* node)
    {
        type_points.connect(start,path,node);
    }


    std::vector<MergingLayer::TypePoint> MergingLayer::apply(const std::vector<MergingLayer::TypePoint>& in) const
    {
        //TODO: switch from list of changes to list after changes approach
        std::vector<MergingLayer::TypePoint> ret;
        std::vector<MergingLayer::TypePoint> tmp;
        savePoints(in,tmp);
        unsigned offset = 0;

        for (const auto& i:tmp)
        {
            if (i.begin < offset)
                continue;
            for (unsigned j=offset; j<i.begin; ++j)
            {
                ret.emplace_back(in[j]);
            }
            offset = i.end;
            ret.emplace_back(i);
        }

        return std::move(ret);
    }

    void LayeredMergingRule::setTokenMerger(const std::function<std::unique_ptr<Token>(unsigned,unsigned,unsigned,const std::vector<TokenEntity>&)>& f)
    {
        merger = f;
    }

    std::unique_ptr<Token> LayeredMergingRule::merge(unsigned begin,unsigned end,unsigned type,const std::vector<TokenEntity>& source) const
    {
        std::unique_ptr<Token> tmp = std::move(merger(begin,end,type,source));
        tmp -> setPos(source[begin].token -> getPos());
        return std::move(tmp);
    }

    void LayeredMergingRule::apply(std::vector<TokenEntity>& source) const
    {
        if (merger)
        {
            if (deep)
            {
                for (auto& i: source)
                {
                    ScopeToken* sc = i.token -> as<ScopeToken>();
                    if (sc != nullptr)
                    {
                        apply(sc -> tokens);
                    }
                }
            }

            std::vector<MergingLayer::TypePoint> types;
            types.resize(source.size());

            //get types as TypePoint array

            for (unsigned i=0; i<source.size(); ++i)
            {
                types[i] = MergingLayer::TypePoint(i,i+1,source[i].type);
            }

            //pass through layers

            for (const auto& i: layers)
            {
                types = std::move(i.apply(types));
            }

            //merge on array difference

            unsigned offset = 0;

            for (const auto& i:types)
            {
                if (i.begin + 1 != i.end)
                {
                    //merge
                    std::unique_ptr<Token> tmp = merge(i.begin - offset,i.end - offset,i.type,source);

                    source.erase(source.begin() + i.begin - offset,source.begin() + i.end - offset);
                    source.emplace(source.begin() + i.begin - offset,std::move(tmp),i.type);

                    offset += (i.end - i.begin - 1);
                }
            }
        }
    }
}
