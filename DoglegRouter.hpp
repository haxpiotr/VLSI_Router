# pragma once

#include "ISpecializedRouter.hxx"

namespace in
{
class DoglegRouter : public ISpecializedRouter
{
public:
    ~DoglegRouter() override = default;
    RoutedPath route(const Net& net) const override;
protected:
    [[nodiscard]] bool verifyForDogleg(const Net& net) const;
    virtual std::vector<Coord> perform(const Coord& a, const Coord& b) const = 0;
};

class UpperDoglegRouter : public DoglegRouter
{
public:
    ~UpperDoglegRouter() override = default;
private:
    std::vector<Coord> perform(const Coord& a, const Coord& b) const override;
};

class LowerDoglegRouter : public DoglegRouter
{
public:
    ~LowerDoglegRouter() override = default;
private:
    std::vector<Coord> perform(const Coord& a, const Coord& b) const override;
};

std::unique_ptr<DoglegRouter> createDoglegRouter(DoglegType type);

}
