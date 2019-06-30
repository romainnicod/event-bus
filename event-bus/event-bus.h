#pragma once

#include <tuple>
#include <vector>
#include <functional>

namespace eb
{

template <typename E>
class bus;

namespace detail
{

template <typename Tu, template <typename> typename T>
struct map_tuple;

template <typename... E, template <typename> typename T>
struct map_tuple<std::tuple<E...>, T>
{
	using type = std::tuple<T<E>...>;
};

template <typename E, template <typename> typename T>
using map_tuple_t = typename map_tuple<E, T>::type;

template <typename E, typename V = void>
struct get_event_callback
{
	using type = void(const E&);
};

template <typename E>
struct get_event_callback<E, typename std::void_t<typename E::callback_t>>
{
	using type = typename E::callback_t;
};

template <typename E>
using get_event_callback_t = typename get_event_callback<E>::type;

template <typename E, typename F>
struct add_event_parameter;

template <typename E, typename R, typename... Args>
struct add_event_parameter<E, R(Args...)>
{
	using type = R(const E&, Args...);
};

template <typename E, typename F>
using add_event_parameter_t = typename add_event_parameter<E, F>::type;

namespace detail
{

template<typename, template<typename...> class, typename...>
struct is_detected : std::false_type {};

template<template<class...> class Operation, typename... Arguments>
struct is_detected<std::void_t<Operation<Arguments...>>, Operation, Arguments...> : std::true_type {};

} // namespace detail

template<template<class...> class Operation, typename... Arguments>
using is_detected = detail::is_detected<std::void_t<>, Operation, Arguments...>;

template<template<class...> class Operation, typename... Arguments>
constexpr bool is_detected_v = detail::is_detected<std::void_t<>, Operation, Arguments...>::value;

template <typename T, typename... Args>
using on_t = decltype(std::declval<T>().on(std::declval<Args>()...));

template <typename T, typename CB>
struct has_on;

template <typename T, typename R, typename... Args>
struct has_on<T, R(Args...)>
{
	static constexpr bool value = is_detected_v<on_t, T, Args...>;
};

template <typename T, typename CB>
constexpr bool has_on_v = has_on<T, CB>::value;

template <typename Es, typename Tu = Es>
struct auto_sub
{
	template <typename T>
	void operator()(T& object);
};

template <typename Es, typename H, typename... Tail>
struct auto_sub<Es, std::tuple<H, Tail...>>
{
	template <typename T>
	auto operator()(T& object)
	{
		if constexpr(has_on_v<T, get_event_callback_t<H>>)
			host->sub<H>([&object](auto&&... event) { object.on(std::forward<decltype(event)>(event)...); });
		auto_sub<Es, std::tuple<Tail...>>{ host }(object);
	}
	bus<Es>* host;
};

template <typename Es>
struct auto_sub<Es, std::tuple<>>
{
	template <typename T>
	auto operator()(T& object) {}
	bus<Es>* host;
};

} // namespace detail

template <typename Es>
class bus
{
	using events_t = Es;
	template <typename E>
	using callback_t = std::function<detail::get_event_callback_t<E>>;
	template <typename E>
	using callback_vector_t = std::vector<callback_t<E>>;

public:
	template <typename T>
	void sub(T& object);

	template <typename E>
	void sub(callback_t<E> callback);

	template <typename E, typename... Args>
	void dispatch(Args&&... args);

	template <typename E, typename... Args>
	void dispatch(const E&& event, Args&&... args);

private:
	detail::map_tuple_t<events_t, callback_vector_t> m_event_vectors;
};

template <typename Es>
template <typename T>
void bus<Es>::sub(T& object)
{
	detail::auto_sub<Es>{ this }(object);
}

template <typename Es>
template <typename E>
void bus<Es>::sub(callback_t<E> callback)
{
	callback_vector_t<E>& vector = std::get<callback_vector_t<E>>(m_event_vectors);
	vector.emplace_back(std::move(callback));
}

template <typename Es>
template <typename E, typename... Args>
void bus<Es>::dispatch(Args&& ... args)
{
	dispatch(E{}, std::forward<Args>(args)...);
}

template <typename Es>
template <typename E, typename... Args>
void bus<Es>::dispatch(const E&& event, Args&&... args)
{
	callback_vector_t<E>& vector = std::get<callback_vector_t<E>>(m_event_vectors);
	for(auto& callback : vector)
		callback(event, args...);
}

template <typename E, typename F>
struct custom_event
{
	using callback_t = detail::add_event_parameter_t<E, F>;
};

} // namespace eb