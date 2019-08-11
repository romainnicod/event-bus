#pragma once

#include <tuple>
#include <vector>
#include <functional>
#include <map>

namespace eb
{

template <typename Es>
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
	using type = void(E&);
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
	using type = R(E&, Args...);
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
	template <typename B, typename T, typename... Args>
	void operator()(B&& bus, T& object, Args&& ... args);
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
	void dispatch(Args&& ... args);

	template <typename E, typename... Args>
	void dispatch(E&& event, Args&& ... args);

private:
	detail::map_tuple_t<events_t, callback_vector_t> m_event_vectors;
};

template <typename Es>
template <typename T>
void bus<Es>::sub(T& object)
{
	detail::auto_sub<Es>{}(*this, object);
}

template <typename Es>
template <typename E>
void bus<Es>::sub(callback_t<E> callback)
{
	callback_vector_t<E>& vector = std::get<callback_vector_t<E>>(m_event_vectors);
	vector.push_back(std::move(callback));
}

template <typename Es>
template <typename E, typename... Args>
void bus<Es>::dispatch(Args&& ... args)
{
	dispatch(E{}, std::forward<Args>(args)...);
}

template <typename Es>
template <typename E, typename... Args>
void bus<Es>::dispatch(E&& event, Args&& ... args)
{
	callback_vector_t<std::decay_t<E>>& vector = std::get<callback_vector_t<std::decay_t<E>>>(m_event_vectors);
	for(auto& callback : vector)
		callback(event, std::forward<Args>(args)...);
}

template <typename Es, typename K>
class categorized_bus
{
	using events_t = Es;
	template <typename E>
	using callback_t = std::function<detail::get_event_callback_t<E>>;
	template <typename E>
	using callback_vector_t = std::vector<callback_t<E>>;
	template <typename E>
	using callback_categories_t = std::map<K, callback_vector_t<E>>;

public:
	template <typename T>
	void sub(T& object, const K& category);

	template <typename E>
	void sub(callback_t<E> callback, const K& category);

	template <typename E, typename... Args>
	void dispatch(const K& category, Args&&... args);

	template <typename E, typename... Args>
	void dispatch(E&& event, const K& catgeory, Args&& ... args);

private:
	detail::map_tuple_t<events_t, callback_categories_t> m_event_categories;
};

template <typename Es, typename K>
template <typename T>
void categorized_bus<Es, K>::sub(T& object, const K& category)
{
	detail::auto_sub<Es>{}(*this, object, category);
}

template <typename Es, typename K>
template <typename E>
void categorized_bus<Es, K>::sub(callback_t<E> callback, const K& category)
{
	callback_categories_t<E>& categories = std::get<callback_categories_t<E>>(m_event_categories);
	callback_vector_t<E>& vector = categories[category];
	vector.push_back(std::move(callback));
}

template <typename Es, typename K>
template <typename E, typename... Args>
void categorized_bus<Es, K>::dispatch(const K& category, Args&& ... args)
{
	dispatch(E{}, category, std::forward<Args>(args)...);
}

template <typename Es, typename K>
template <typename E, typename... Args>
void categorized_bus<Es, K>::dispatch(E&& event, const K& category, Args&& ... args)
{
	using dE = std::decay_t<E>;
	callback_categories_t<dE>& categories = std::get<callback_categories_t<dE>>(m_event_categories);
	if(auto cat_it = categories.find(category); cat_it != categories.end())
	{
		callback_vector_t<dE>& vector = cat_it->second;
		for(auto& callback : vector)
			callback(event, std::forward<Args>(args)...);
	}
}

namespace detail
{

template <typename H, typename... Tail>
struct auto_sub<std::tuple<H, Tail...>>
{
	template <typename B, typename T, typename... Args>
	auto operator()(B&& bus, T& object, Args&& ... args)
	{
		if constexpr(has_on_v<T, get_event_callback_t<H>>)
			bus.sub<H>([&object](auto&& ... event) { object.on(std::forward<decltype(event)>(event)...); }, std::forward<Args>(args)...);
		auto_sub<std::tuple<Tail...>>{}(bus, object, std::forward<Args>(args)...);
	}
};

template <>
struct auto_sub<std::tuple<>>
{
	template <typename B, typename T, typename... Args>
	auto operator()(B&& bus, T& object, Args&& ... args) {}
};

} // namespace detail

template <typename E, typename F>
struct custom_event
{
	using callback_t = detail::add_event_parameter_t<E, F>;
};

} // namespace eb