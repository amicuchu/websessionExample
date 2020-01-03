#include <type_traits>
#include <list>

template <class ... CallerArgs>
class Broadcast
{
public:
    using CallerType = int(*)(CallerArgs ... args);

    Broadcast(){
        listeners = std::list<CallerType>();
    }

    ~Broadcast(){

    }
    
    void addListener(CallerType listener){
        listeners.push_front(listener);
    }

    void broadcast(CallerArgs ... args){
        for(CallerType listener : this->listeners){
            int result = listener(args...);
            if(result == 1) break;
        }
    }
private:
    std::list<CallerType> listeners;
};
