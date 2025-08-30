#include "std_support/Deque.hpp"
#include "std_support/New.hpp"
#include "std_support/UnorderedMap.hpp"
#include "std_support/UnorderedSet.hpp"
#include "std_support/Set.hpp"
#include "std_support/Map.hpp"
#include "std_support/List.hpp"
#include "std_support/Vector.hpp"

template<class Value>
using KStdDeque = kotlin::std_support::deque<Value>;

template<class Key, class Value>
using KStdUnorderedMap = kotlin::std_support::unordered_map<Key, Value>;

template<class Value>
using KStdUnorderedSet = kotlin::std_support::unordered_set<Value>;

template<class Value, class Compare = std::less<Value>>
using KStdOrderedSet = kotlin::std_support::set<Value, Compare>;

template<class Key, class Value, class Compare = std::less<Key>>
using KStdOrderedMap = kotlin::std_support::map<Key, Value, Compare>;

template<class Value>
using KStdVector = kotlin::std_support::vector<Value>;

template<class Value>
using KStdList = kotlin::std_support::list<Value>;

template <class Value>
using KStdUniquePtr = kotlin::std_support::unique_ptr<Value>;