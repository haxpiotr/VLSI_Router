#pragma once

#include <boost/geometry.hpp>

namespace in
{
	namespace bg = boost::geometry;
	using point_int = bg::model::d2::point_xy<int>;
	using box_int = bg::model::box<point_int>;
	using point = bg::model::d2::point_xy<double>;
	using box = bg::model::box<point>;
	enum class DoglegType
	{
		UPPER,
		LOWER,
		ANY,
		NONE
	};
}