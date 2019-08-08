#include <iostream>
#include "event-bus.h"

struct C { int j; };
struct event1 {	int i; };
struct event2 : eb::custom_event<event2, void(C&)> {};
struct event3 : eb::custom_event<event3, void(int)> { bool b; };

struct L
{
	void on(const event1& e) { std::cout << "event1 " << e.i << "\n"; }
	void on(event2, C& c) { std::cout << "event2 " << c.j << std::endl; }
	void on(const event3& e, int i) { std::cout << "event3 " << std::boolalpha << e.b << " " << i << std::endl; }
};

int main()
{
	eb::bus<std::tuple<event1, event2, event3>> b;
	L l;
	b.sub(l);

	event1 e1{ 7 };
	b.dispatch(e1);
	C c{ 5 };
	b.dispatch<event2>(c);
	b.dispatch(event3{ {}, {true} }, 10);
	return 0;
}