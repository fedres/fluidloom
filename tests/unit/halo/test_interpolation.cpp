#include <gtest/gtest.h>
#include "fluidloom/halo/InterpolationParams.h"
#include "fluidloom/halo/interpolation/TrilinearInterpolator.h"
#include "fluidloom/core/backend/MockBackend.h"

using namespace fluidloom::halo;

TEST(InterpolationTest, TrilinearParamsValidation) {
    TrilinearParams params;
    params.initializeDefault();
    
    EXPECT_TRUE(params.validate());
    
    float sum = 0.0f;
    for (float w : params.weights) {
        sum += w;
    }
    EXPECT_NEAR(sum, 1.0f, 1e-6f);
}

TEST(InterpolationTest, LUTInitialization) {
    InterpolationLUT lut;
    lut.initialize();
    
    // Check same level (should be identity/direct copy logic, but params initialized to default)
    const auto& params = lut.get(0, 0);
    EXPECT_TRUE(params.validate());
    
    // Check coarse->fine
    const auto& cf_params = lut.get(1, 0); // Local 1 (fine), Remote 0 (coarse)
    EXPECT_TRUE(cf_params.validate());
}
