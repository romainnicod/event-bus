# Event Bus
Simple event bus library with automatic subscription support

# Usage
## Simple events
```cpp
struct event1;
struct event2;
struct event3;

struct OddListener
{
    void on(event1) { std::cout << "OddListener event1\n"; }
    void on(event3) { std::cout << "OddListener event3\n"; }
};

struct EvenListener
{
    void on(event2) { std::cout << "EvenListener event2\n"; }
};


using events = std::tuple<
    event1,
    event2,
    event3
>;

eb::bus<events> bus;
OddListener ol;
EvenListener el;

bus.sub(ol);
bus.sub(el);

bus.dispatch(event1{});
bus.dispatch(event2{});
bus.dispatch(event3{});
```
Outputs:
```terminal
OddListener event1
EvenListener event2
OddListener event3
```

## Complex events
```cpp
struct C { int value; };

// Event with embedded data
struct event1 {	int value; };
// Event with additional data
struct event2 : eb::custom_event<event2, void(const C&)> {};
// Event with embedded and additional data
struct event3 : eb::custom_event<event3, void(int)> { bool value; };

struct AllEventListener
{
    void on(const event1& e) { std::cout << "AllEventListener event1::value=" << e.value << "\n"; }
    void on(event2, const C& data) { std::cout << "AllEventListener event2 C::value=" << data.value << "\n"; }
    void on(const event3& e, int additional) { std::cout << "AllEventListener event3::value=" << e.value << " additional=" << additional << "\n"; }
};

struct Event1Listener
{
    void on(event1) { std::cout << "EvenListener event1\n"; }
};


using events = std::tuple<
    event1,
    event2,
    event3
>;

eb::bus<events> bus;
AllEventListener ael;
Event1Listener e1l;

bus.sub(ael);
bus.sub(e1l);

C c{ 9 };
event1 e1{ 7 };

bus.dispatch(e1);
bus.dispatch<event2>(c);
bus.dispatch(event3{ {}, { true } }, 10);
```
Outputs:
```terminal
AllEventListener event1::value=7
EvenListener event1
AllEventListener event2 C::value=9
AllEventListener event3::value=1 additional=10
```

## Manual subscription
```cpp
struct event1;

void listener(event1) {}

eb::bus<std::tuple<event1>> bus;
bus.sub<event1>(std::bind(&listener, std::placeholders::_1));
```
