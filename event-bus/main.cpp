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

struct cat_L
{
	cat_L(std::string_view cat) : cat(cat) {}
	void on(event1) { std::cout << cat << std::endl; }
	std::string cat;
};

using events = std::tuple<event1, event2, event3>;

int main()
{
	cat_L a("a"), b("b");
	eb::categorized_bus<events, std::string> cat_bus;
	cat_bus.sub(a, "a");
	cat_bus.sub(b, "b");
	cat_bus.dispatch(event1{}, "a");
	cat_bus.dispatch(event1{}, "b");
	return 0;

	eb::bus<events> bus;
	L l;
	bus.sub(l);

	event1 e1{ 7 };
	bus.dispatch(e1);
	C c{ 5 };
	bus.dispatch<event2>(c);
	bus.dispatch(event3{ {}, {true} }, 10);
	return 0;
}