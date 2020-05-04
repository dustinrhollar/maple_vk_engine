#ifndef ENGINE_UTILS_JTUPLE
#define ENGINE_UTILS_JTUPLE

// A two-type tuple

template<class T, class V>
class JTuple {
    
    public:
    
    JTuple(T t, V v);
    ~JTuple() = default;
    
    JTuple(JTuple& other);
    JTuple(JTuple&& other);
    
    JTuple operator=(JTuple& cpy);
    JTuple operator=(JTuple&& cpy);
    
    T First();
    V Second();
    
    private:
    
    T t;
    V v;
    
};

template<class T, class V>
JTuple<T,V>::JTuple(T _t, V _v)
: t(_t)
, v(_v)
{
}

template<class T, class V>
JTuple<T,V>::JTuple(JTuple& other)
: t(other.t)
, v(other.v)
{
}

template<class T, class V>
JTuple<T,V>::JTuple(JTuple&& other)
: t(other.t)
, v(other.v)
{
}

template<class T, class V>
JTuple<T,V> JTuple<T,V>::operator=(JTuple& cpy)
{
    t = cpy.t;
    v = cpy.v;
}

template<class T, class V>
JTuple<T,V> JTuple<T,V>::operator=(JTuple&& cpy)
{
    t = cpy.t;
    v = cpy.v;
}


template<class T, class V>
T JTuple<T,V>::First() {
    return t;
}


template<class T, class V>
V JTuple<T,V>::Second() {
    return v;
}


#endif // JENGINE_UTILS_JTUPLE
