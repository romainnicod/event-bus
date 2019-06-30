#include <iostream>
#include "event-bus.h"

struct C
{
	int j;
};
struct event1
{
	int i;
};
struct event2 : eb::custom_event<event2, void(const C&)>
{
};

struct event3 : eb::custom_event<event3, void(int)>
{
	bool b;
};

struct L
{
	void on(const event1& e) { std::cout << "e " << e.i << "\n"; }
	void on(const event2&, const C& c) { std::cout << c.j << std::endl; }
	void on(const event3& e, int i) { std::cout << i << " " << std::boolalpha << e.b << std::endl; }
};

void f(const event1& e)
{
	std::cout << "f\n";
}

int main()
{
	eb::bus<std::tuple<event1, event2, event3>> b;
	L l;
	b.sub(l);
	std::function<void(const event1&)> ff = std::bind(&f, std::placeholders::_1);
	b.sub<event1>(ff);
	event1 e1{ 7 };
	b.dispatch(e1);
	b.dispatch<event2>(C{ 5 });
	b.dispatch(event3{ {}, {true} }, 10);
	return 0;
}