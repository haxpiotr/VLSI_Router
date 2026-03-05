#include "MinimumSpanningTree.hpp"
#include "NetSolution.hpp"
#include <boost/graph/kruskal_min_spanning_tree.hpp>

namespace tree
{
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
                const int dist = std::abs(points[i].x() - points[j].x()) + std::abs(points[i].y() - points[j].y());
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

    std::pair<std::vector<ResultEdge>, std::vector<Point>> rectinilearSteinerMST(const std::vector<Point>& points)
    {
        auto extendedPoints = points;

        auto mst = rectilinearMST(points);

        auto firstEdgeIt = mst.begin();
        auto firstEdge = *firstEdgeIt;

        mst.erase(firstEdgeIt);

        auto neighborIt = std::find_if(mst.begin(), mst.end(), [&firstEdge](const auto& e)
            {
                if (e.u == firstEdge.u && e.v == firstEdge.v)
                {
                    return false;
                }
                if (e.u == firstEdge.u && e.v != firstEdge.v)
                {
                    return true;
                }
                if (e.u != firstEdge.u && e.v == firstEdge.v)
                {
                    return true;
                }
                if (e.u == firstEdge.v && e.v != firstEdge.u)
                {
                    return true;
                }
                if (e.u != firstEdge.v && e.v == firstEdge.u)
                {
                    return true;
                }
                return false;
            });

        std::cout << "firstEdge: " << firstEdge.u << " " << firstEdge. v << '\n';

        while (neighborIt != mst.end())
        {
            std::cout << "Neighbor: " << neighborIt->u << " " << neighborIt->v << '\n';
 
            in::Segment firstSegment { extendedPoints[firstEdge.u],extendedPoints[firstEdge.v] };
            in::Segment neighSegment = { extendedPoints[neighborIt->u],extendedPoints[neighborIt->v] };

            const auto horizontalOverlap = getBestHorizontalOverlap(firstSegment, neighSegment);
            const auto verticalOverlap = getBestVerticalOverlap(firstSegment, neighSegment);
            const auto commonCoord = getCommonCoord(firstSegment, neighSegment);

            std::cout << "horizontalOverlap: " << horizontalOverlap << '\n';
            std::cout << "verticalOverlap: " << verticalOverlap << '\n';
            std::cout << "commonCoord: " << commonCoord.x() << ", " << commonCoord.y() << '\n';

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

            std::cout << "neighPoint: " << neighPoint.x() << ", " << neighPoint.y() << '\n';

            auto neighPointIt = std::find_if(extendedPoints.begin(), extendedPoints.end(), [&neighPoint](const auto& p)
                {
                    return p.x() == neighPoint.x() && p.y() == neighPoint.y();
                });

            const auto neighIndex = std::distance(extendedPoints.begin(), neighPointIt);


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
                std::cout << "steiner: " << steiner.x() << ", " << steiner.y() << '\n';
                extendedPoints.push_back(steiner);

                mst = rectilinearMST(extendedPoints);

                const size_t steinerIndex = extendedPoints.size() - 1;

                mst.erase(std::remove_if(mst.begin(), mst.end(), [steinerIndex, neighIndex](const auto& e)
                    {
                        if (e.v == steinerIndex && e.u != neighIndex)
                        {
                            return true;
                        }
                        if (e.u == steinerIndex && e.v != neighIndex)
                        {
                            return true;
                        }
                        return false;
                    }),mst.end());

                firstEdgeIt = std::find_if(mst.begin(), mst.end(), [steinerIndex, neighIndex](const auto& e)
                    {
                        if (e.u == steinerIndex && e.v == neighIndex)
                        {
                            return true;
                        }
                        if (e.u == neighIndex && e.v == steinerIndex)
                        {
                            return true;
                        }
                        return false;
                    });
                firstEdge = *firstEdgeIt;
                neighborIt = std::find_if(mst.begin(), mst.end(), [&firstEdge](const auto& e)
                    {
                        if (e.u == firstEdge.u && e.v == firstEdge.v)
                        {
                            return false;
                        }
                        if (e.u == firstEdge.u && e.v != firstEdge.v)
                        {
                            return true;
                        }
                        if (e.u != firstEdge.u && e.v == firstEdge.v)
                        {
                            return true;
                        }
                        if (e.u == firstEdge.v && e.v != firstEdge.u)
                        {
                            return true;
                        }
                        if (e.u != firstEdge.v && e.v == firstEdge.u)
                        {
                            return true;
                        }
                        return false;
                    });
                std::cout << "firstEdge: " << firstEdge.u << " " << firstEdge.v << '\n';
                std::cout << "Neighbor: " << neighborIt->u << " " << neighborIt->v << '\n';
            }
            else
            {
                mst.erase(firstEdgeIt);
                firstEdgeIt = neighborIt;
                firstEdge = *firstEdgeIt;
                    neighborIt = std::find_if(mst.begin(), mst.end(), [&firstEdge](const auto& e)
                    {
                        if (e.u == firstEdge.u && e.v == firstEdge.v)
                        {
                            return false;
                        }
                        if (e.u == firstEdge.u && e.v != firstEdge.v)
                        {
                            return true;
                        }
                        if (e.u != firstEdge.u && e.v == firstEdge.v)
                        {
                            return true;
                        }
                        if (e.u == firstEdge.v && e.v != firstEdge.u)
                        {
                            return true;
                        }
                        if (e.u != firstEdge.v && e.v == firstEdge.u)
                        {
                            return true;
                        }
                        return false;
                    });

                    std::cout << "firstEdge: " << firstEdge.u << " " << firstEdge.v << '\n';
                    std::cout << "Neighbor: " << neighborIt->u << " " << neighborIt->v << '\n';
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