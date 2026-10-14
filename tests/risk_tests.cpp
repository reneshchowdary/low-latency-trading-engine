#include "trading/risk.hpp"
#include <cstdlib>
#include <iostream>
void require(bool ok) { if (!ok) std::abort(); }
int main() {
    using namespace trading;
    RiskValidator risk(RiskLimits{100, 1000, 10000});
    require(risk.validate({1, Side::Buy, Type::Limit, 100, 100}));
    require(!risk.validate({2, Side::Buy, Type::Limit, 101, 100}));
    require(!risk.validate({3, Side::Buy, Type::Limit, 1001, 1}));
    require(!risk.validate({4, Side::Buy, Type::Limit, 100, 101}));
    require(!risk.validate({5, Side::Buy, Type::Limit, 0, 10}));
    require(!risk.validate({0, Side::Buy, Type::Limit, 100, 1}));
    require(risk.validate({6, Side::Sell, Type::Market, 0, 50}));
    require(!risk.validate({7, Side::Sell, Type::Market, 0, 101}));
    std::cout << "risk validation tests passed\n";
}
