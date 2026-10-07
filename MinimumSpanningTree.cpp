#include "MinimumSpanningTree.hpp"
#include "NetSolution.hpp"
#include <boost/graph/kruskal_min_spanning_tree.hpp>
#include <boost/graph/metric_tsp_approx.hpp>

namespace tree
{

    std::vector<ResultEdge> getTSP(
        const std::vector<Point>& points,
        const std::vector<size_t>& indices)
    {
        const size_t n = indices.size();

        if (n < 2)
            return {};

        Graph g(n);

        auto global = [&](size_t local) 
            {
            return indices[local];
            };

        for (size_t i = 0; i < n; ++i)
        {
            for (size_t j = i + 1; j < n; ++j)
            {
                size_t gi = global(i);
                size_t gj = global(j);

                const double dx = points[gi].x() - points[gj].x();
                const double dy = points[gi].y() - points[gj].y();
                const double dist = std::sqrt(dx * dx + dy * dy);

                boost::add_edge(i, j, EdgeWeightProperty(dist), g);
            }
        }

        std::vector<size_t> tour;
        boost::metric_tsp_approx_tour(g, std::back_inserter(tour));

        // --- usuń najdłuższą krawędź ---
        double maxDist = -1.0;
        size_t maxIndex = 0;

        for (size_t i = 0; i + 1 < tour.size(); ++i)
        {
            size_t u = global(tour[i]);
            size_t v = global(tour[i + 1]);

            double dx = points[u].x() - points[v].x();
            double dy = points[u].y() - points[v].y();
            double dist = std::sqrt(dx * dx + dy * dy);

            if (dist > maxDist)
            {
                maxDist = dist;
                maxIndex = i;
            }
        }

        std::vector<size_t> path;

        for (size_t i = maxIndex + 1; i < tour.size() - 1; ++i)
            path.push_back(tour[i]);

        for (size_t i = 0; i <= maxIndex; ++i)
            path.push_back(tour[i]);

        std::vector<ResultEdge> result;

        for (size_t i = 0; i + 1 < path.size(); ++i)
        {
            size_t u = global(path[i]);
            size_t v = global(path[i + 1]);

            double dx = points[u].x() - points[v].x();
            double dy = points[u].y() - points[v].y();
            double dist = std::sqrt(dx * dx + dy * dy);

            result.push_back({ u, v, dist });
        }

        return result;
    }

    std::vector<ResultEdge> getTSP(const std::vector<Point>& points)
    {
        const int n = points.size();

        if (n < 2)
        {
            return {};
        }

        Graph g(n);

        for (int i = 0; i < n; ++i)
        {
            for (int j = i + 1; j < n; ++j)
            {
                const double dx = points[i].x() - points[j].x();
                const double dy = points[i].y() - points[j].y();
                const double dist = std::sqrt(dx * dx + dy * dy);

                boost::add_edge(i, j, EdgeWeightProperty(dist), g);
            }
        }

        std::vector<size_t> tour;

        boost::metric_tsp_approx_tour(g, std::back_inserter(tour));

        double maxDist = -1.0;
        size_t maxIndex = 0;

        for (size_t i = 0; i + 1 < tour.size(); ++i)
        {
            size_t u = tour[i];
            size_t v = tour[i + 1];

            const double dx = points[u].x() - points[v].x();
            const double dy = points[u].y() - points[v].y();
            const double dist = std::sqrt(dx * dx + dy * dy);

            if (dist > maxDist)
            {
                maxDist = dist;
                maxIndex = i;
            }
        }

        std::vector<size_t> path;

        for (size_t i = maxIndex + 1; i < tour.size() - 1; ++i)
        {
            path.push_back(tour[i]);
        }

        for (size_t i = 0; i <= maxIndex; ++i)
        {
            path.push_back(tour[i]);
        }

        std::vector<ResultEdge> result;

        for (size_t i = 0; i + 1 < path.size(); ++i)
        {
            size_t u = path[i];
            size_t v = path[i + 1];

            const double dx = points[u].x() - points[v].x();
            const double dy = points[u].y() - points[v].y();
            const double dist = std::sqrt(dx * dx + dy * dy);

            result.push_back(ResultEdge{ u, v, dist });
        }

        return result;
    }


    std::vector<ResultEdge> rectilinearMST(const std::vector<Point>& points)
    {
        const int n = points.size();

        if (n < 2)
        {
            return {};
        }

        Graph g(n);

        for (int i = 0; i < n; ++i)
        {
            for (int j = i + 1; j < n; ++j)
            {
                const double squared = std::pow(points[i].x() - points[j].x(),2) + std::pow(points[i].y() - points[j].y(), 2);
                const double dist = std::sqrt(squared);
                boost::add_edge(i, j, EdgeWeightProperty(dist), g);
            }
        }

        std::vector<EdgeDescriptor> mst_edges;
        boost::kruskal_minimum_spanning_tree(g, std::back_inserter(mst_edges));

        std::vector<ResultEdge> result;
        for (auto const& ed : mst_edges)
        {
            result.push_back({
                boost::source(ed, g),
                boost::target(ed, g),
                boost::get(boost::edge_weight, g, ed) });
        }

        return result;
    }

    int getHorizontalOverlap(const in::DoglegSegment& a, const in::DoglegSegment& b)
    {
        if (a.horizontalSegment.first.y() != b.horizontalSegment.first.y())
        {
            return 0;
        }
        
        int low = std::max(std::min(a.horizontalSegment.first.x(), a.horizontalSegment.second.x()),std::min(b.horizontalSegment.first.x(), b.horizontalSegment.second.x()));
        int high = std::min(std::max(a.horizontalSegment.first.x(), a.horizontalSegment.second.x()), std::max(b.horizontalSegment.first.x(), b.horizontalSegment.second.x()));

        return high - low;
    }

    int getVerticalOverlap(const in::DoglegSegment& a, const in::DoglegSegment& b)
    {
        if (a.verticalSegment.first.x() != b.verticalSegment.first.x())
        {
            return 0;
        }

        int low = std::max(std::min(a.verticalSegment.first.y(), a.verticalSegment.second.y()), std::min(b.verticalSegment.first.y(), b.verticalSegment.second.y()));
        int high = std::min(std::max(a.verticalSegment.first.y(), a.verticalSegment.second.y()), std::max(b.verticalSegment.first.y(), b.verticalSegment.second.y()));

        return high - low;
    }

    int getBestHorizontalOverlap(const in::Segment& a, const in::Segment& b)
    {
        int bestOverlap{ 0 };
        in::NetSolution firstUpperDoglegSolution;
        firstUpperDoglegSolution.endpoints = a;
        firstUpperDoglegSolution.type = in::DoglegType::UPPER;
        auto firstUpperDogleg = route(firstUpperDoglegSolution);
        auto firstLowerDoglegSolution = firstUpperDoglegSolution;
        firstLowerDoglegSolution.type = in::DoglegType::LOWER;
        auto firstLowerDogleg = route(firstLowerDoglegSolution);

        in::NetSolution neighUpperDoglegSolution;
        neighUpperDoglegSolution.endpoints = b;
        neighUpperDoglegSolution.type = in::DoglegType::UPPER;
        auto neighUpperDogleg = route(neighUpperDoglegSolution);
        auto neighLowerDoglegSolution = neighUpperDoglegSolution;
        neighLowerDoglegSolution.type = in::DoglegType::LOWER;
        auto neighLowerDogleg = route(neighLowerDoglegSolution);

        bestOverlap = std::max(getHorizontalOverlap(firstUpperDogleg, neighUpperDogleg), bestOverlap);
        bestOverlap = std::max(getHorizontalOverlap(firstUpperDogleg, neighLowerDogleg), bestOverlap);
        bestOverlap = std::max(getHorizontalOverlap(firstLowerDogleg, neighUpperDogleg), bestOverlap);
        bestOverlap = std::max(getHorizontalOverlap(firstLowerDogleg, neighLowerDogleg), bestOverlap);

        return bestOverlap;
    }

    int getBestVerticalOverlap(const in::Segment& a, const in::Segment& b)
    {
        int bestOverlap{ 0 };
        in::NetSolution firstUpperDoglegSolution;
        firstUpperDoglegSolution.endpoints = a;
        firstUpperDoglegSolution.type = in::DoglegType::UPPER;
        auto firstUpperDogleg = route(firstUpperDoglegSolution);
        auto firstLowerDoglegSolution = firstUpperDoglegSolution;
        firstLowerDoglegSolution.type = in::DoglegType::LOWER;
        auto firstLowerDogleg = route(firstLowerDoglegSolution);

        in::NetSolution neighUpperDoglegSolution;
        neighUpperDoglegSolution.endpoints = b;
        neighUpperDoglegSolution.type = in::DoglegType::UPPER;
        auto neighUpperDogleg = route(neighUpperDoglegSolution);
        auto neighLowerDoglegSolution = neighUpperDoglegSolution;
        neighLowerDoglegSolution.type = in::DoglegType::LOWER;
        auto neighLowerDogleg = route(neighLowerDoglegSolution);

        bestOverlap = std::max(getVerticalOverlap(firstUpperDogleg, neighUpperDogleg), bestOverlap);
        bestOverlap = std::max(getVerticalOverlap(firstUpperDogleg, neighLowerDogleg), bestOverlap);
        bestOverlap = std::max(getVerticalOverlap(firstLowerDogleg, neighUpperDogleg), bestOverlap);
        bestOverlap = std::max(getVerticalOverlap(firstLowerDogleg, neighLowerDogleg), bestOverlap);

        return bestOverlap;
    }

    in::Coord getCommonCoord(const in::Segment& a, const in::Segment& b)
    {
        if (in::bg::equals(a.first, b.first))
        {
            return a.first;
        }
        if (in::bg::equals(a.first, b.second))
        {
            return a.first;
        }
        if (in::bg::equals(a.second, b.first))
        {
            return a.second;
        }
        if (in::bg::equals(a.second, b.second))
        {
            return a.second;
        }
        return {};
    }

    std::vector<ResultEdge>::const_iterator findEdge(std::vector<ResultEdge>& mst, const ResultEdge& edge)
    {
        auto it = std::find_if(mst.begin(), mst.end(), [&edge](const auto& e)
            {
                if (e.u == edge.u && e.v == edge.v)
                {
                    return true;
                }
                if (e.v == edge.u && e.u == edge.v)
                {
                    return true;
                }
                return false;
            });

        return it;
    }

    void removeEdge(std::vector<ResultEdge>& mst, const ResultEdge& edge)
    {
        auto removeIt = std::find_if(mst.begin(), mst.end(), [&edge](const auto& e)
            {
                if (e.u == edge.u && e.v == edge.v)
                {
                    return true;
                }
                if (e.v == edge.u && e.u == edge.v)
                {
                    return true;
                }
                return false;
            });

        if (removeIt == mst.end())
        {
            return;
        }

        mst.erase(removeIt);
    }

    std::vector<ResultEdge>::const_iterator findNeighbor(const std::vector<ResultEdge>& mst, const ResultEdge& edge)
    {
        auto neighborIt = std::find_if(mst.begin(), mst.end(), [&edge](const auto& e)
            {
                if (e.u == edge.u && e.v == edge.v)
                {
                    return false;
                }
                if (e.u == edge.v && e.v == edge.u)
                {
                    return false;
                }
                if (e.u == edge.u || e.v == edge.u || e.u == edge.v || e.v == edge.v)
                {
                    return true;
                }
                return false;                
            });
        
        return neighborIt;
    }


    std::optional<ResultEdge> findFirstBoundaryEdge(
        const std::vector<ResultEdge>& edges)
    {
        if (edges.empty()) return std::nullopt;

        size_t maxVertex = 0;
        for (const auto& e : edges)
        {
            maxVertex = std::max({ maxVertex, e.u, e.v });
        }

        std::vector<int> degree(maxVertex + 1, 0);

        for (const auto& e : edges)
        {
            degree[e.u]++;
            degree[e.v]++;
        }

        for (const auto& e : edges)
        {
            if (degree[e.u] == 1 || degree[e.v] == 1)
            {
                return e;
            }
        }

        return std::nullopt;
    }


    size_t findEndpointVertex(
        const std::vector<ResultEdge>& edges,
        const ResultEdge& edge)
    {

        size_t maxVertex = 0;
        for (const auto& e : edges)
        {
            maxVertex = std::max({ maxVertex, e.u, e.v });
        }

        std::vector<int> degree(maxVertex + 1, 0);

        for (const auto& e : edges)
        {
            degree[e.u]++;
            degree[e.v]++;
        }

        if (degree[edge.u] == 1)
        {
            return edge.u;
        }
        else
        {
            return edge.v;
        }
    }




    std::pair<std::vector<ResultEdge>, std::vector<Point>> rectinilearSteinerMST(const std::vector<Point>& points)
    {
        if (points.size() < 2)
        {
            return { {},points };
        }

        if (points.size() == 2)
        {
            return { rectilinearMST(points),points };
        }

        auto extendedPoints = points;

        std::unordered_set<size_t> active;
        for (size_t i = 0; i < points.size(); ++i)
        {
            active.insert(i);
        }

        auto tsp = getTSP(points, std::vector<size_t>(active.begin(), active.end()));

        auto firstEdge = *findFirstBoundaryEdge(tsp);

        auto neighborIt = findNeighbor(tsp, firstEdge);
        
        while (neighborIt != tsp.end())
        {
            auto neighborEdge = *neighborIt;

            in::Segment firstSegment { extendedPoints[firstEdge.u],extendedPoints[firstEdge.v] };
            in::Segment neighSegment = { extendedPoints[neighborEdge.u],extendedPoints[neighborEdge.v] };

            const auto horizontalOverlap = getBestHorizontalOverlap(firstSegment, neighSegment);
            const auto verticalOverlap = getBestVerticalOverlap(firstSegment, neighSegment);
            const auto commonCoord = getCommonCoord(firstSegment, neighSegment);

            Point neighPoint;
            Point steiner = commonCoord;

            if (neighSegment.first.x() != steiner.x() && neighSegment.first.y() != steiner.y())
            {
                neighPoint = neighSegment.first;
            }
            else
            {
                neighPoint = neighSegment.second;
            }

            if (verticalOverlap > horizontalOverlap)
            {
                if (commonCoord.y() > firstSegment.first.y() || commonCoord.y() > firstSegment.second.y())
                {
                    steiner.set<1>(steiner.y() - verticalOverlap);
                }
                else if (commonCoord.y() < firstSegment.first.y() || commonCoord.y() < firstSegment.second.y())
                {
                    steiner.set<1>(steiner.y() + verticalOverlap);
                }
            }
            else if (horizontalOverlap > verticalOverlap)
            {
                if (commonCoord.x() > firstSegment.first.x() || commonCoord.x() > firstSegment.second.x())
                {
                    steiner.set<0>(steiner.x() - horizontalOverlap);
                }
                else if (commonCoord.x() < firstSegment.first.x() || commonCoord.x() < firstSegment.second.x())
                {
                    steiner.set<0>(steiner.x() + horizontalOverlap);
                }
            }

            if (steiner.x() != commonCoord.x() || steiner.y() != commonCoord.y())
            {
                extendedPoints.push_back(steiner);

                size_t newIndex = extendedPoints.size() - 1;

                active.insert(newIndex);
                active.erase(firstEdge.u);
                active.erase(firstEdge.v);

                tsp = getTSP(extendedPoints, std::vector<size_t>(active.begin(), active.end()));

                firstEdge = *findFirstBoundaryEdge(tsp);
                
                neighborIt = findNeighbor(tsp, firstEdge);
                
            }
            else
            {
                const auto endpointVertex = findEndpointVertex(tsp, firstEdge);

                active.erase(endpointVertex);

                tsp = getTSP(extendedPoints, std::vector<size_t>(active.begin(), active.end()));

                firstEdge = *findFirstBoundaryEdge(tsp);

                neighborIt = findNeighbor(tsp, firstEdge);
            }
        }

        return { rectilinearMST(extendedPoints),extendedPoints };
    }

 
    std::vector<ResultSegment> getSegmentsFromMST(const std::vector<ResultEdge>& mst, const std::vector<Point>& points)
    {
        std::vector<ResultSegment> segments;

        for (const auto& m : mst)
        {
            ResultSegment rs;
            rs.a = points[m.u];
            rs.b = points[m.v];
            rs.weight = m.weight;
            segments.push_back(rs);
        }

        return segments;
    }
}