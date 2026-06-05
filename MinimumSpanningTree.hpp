#pragma once

#include <vector>
#include <boost/graph/adjacency_list.hpp>

#include "GeometryTypes.hpp"

namespace tree
{

	using EdgeWeightProperty = boost::property<boost::edge_weight_t, double>;
	using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, EdgeWeightProperty>;
	using EdgeDescriptor = boost::graph_traits<Graph>::edge_descriptor;

    using Point = in::point_int;

    struct ResultEdge 
    {
        size_t u;
        size_t v;
        double weight;
    };

    struct ResultSegment
    {
        Point a;
        Point b;
        double weight;
    };

    std::vector<ResultEdge> rectilinearMST(const std::vector<Point>& points);
    std::vector<ResultEdge> getTSP(const std::vector<Point> &points);
    std::pair<std::vector<ResultEdge>, std::vector<Point>> rectinilearSteinerMST(const std::vector<Point>& points);
    std::vector<ResultSegment> getSegmentsFromMST(const std::vector<ResultEdge>& mst, const std::vector<Point>& points);
}