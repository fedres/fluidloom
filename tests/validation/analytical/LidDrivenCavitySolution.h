#pragma once

#include &lt;vector&gt;
#include &lt;utility&gt;
#include &lt;string&gt;

namespace fluidloom {
namespace validation {

/**
 * @brief Ghia et al. (1982) benchmark data for lid-driven cavity
 * 
 * Reference: Ghia, U., Ghia, K. N., & Shin, C. T. (1982).
 * High-Re solutions for incompressible flow using the Navier-Stokes equations
 * and a multigrid method. Journal of computational physics, 48(3), 387-411.
 */
class LidDrivenCavitySolution {
public:
    /**
     * @brief Get U-velocity along vertical centerline (x = 0.5)
     * @return Vector of (y-coordinate, u-velocity) pairs for Re=1000
     */
    static std::vector&lt;std::pair&lt;double, double&gt;&gt; getUCenterline() {
        // Ghia et al. (1982) Table II, Re=1000
        static const std::vector&lt;std::pair&lt;double, double&gt;&gt; data = {
            {0.0000, 0.00000},
            {0.0547, -0.03717},
            {0.0625, -0.04192},
            {0.0703, -0.04775},
            {0.1016, -0.06434},
            {0.1719, -0.10150},
            {0.2813, -0.15662},
            {0.4531, -0.21090},
            {0.5000, -0.20581},
            {0.6172, -0.13641},
            {0.7344, 0.00332},
            {0.8516, 0.23151},
            {0.9531, 0.68717},
            {0.9609, 0.73722},
            {0.9688, 0.78871},
            {0.9766, 0.84123},
            {1.0000, 1.00000}
        };
        return data;
    }
    
    /**
     * @brief Get V-velocity along horizontal centerline (y = 0.5)
     * @return Vector of (x-coordinate, v-velocity) pairs for Re=1000
     */
    static std::vector&lt;std::pair&lt;double, double&gt;&gt; getVCenterline() {
        // Ghia et al. (1982) Table II, Re=1000
        static const std::vector&lt;std::pair&lt;double, double&gt;&gt; data = {
            {0.0000, 0.00000},
            {0.0625, 0.09233},
            {0.0703, 0.10091},
            {0.0781, 0.10890},
            {0.0938, 0.12317},
            {0.1563, 0.16077},
            {0.2266, 0.17507},
            {0.2344, 0.17527},
            {0.5000, 0.05454},
            {0.8047, -0.24533},
            {0.8594, -0.22445},
            {0.9063, -0.16914},
            {0.9453, -0.10313},
            {0.9531, -0.08864},
            {0.9609, -0.07391},
            {0.9688, -0.05906},
            {1.0000, 0.00000}
        };
        return data;
    }
    
    /**
     * @brief Interpolate benchmark value at given coordinate
     * @param data Benchmark data points
     * @param coord Query coordinate
     * @return Interpolated value
     */
    static double interpolate(
        const std::vector&lt;std::pair&lt;double, double&gt;&gt;& data,
        double coord) {
        
        if (data.empty()) return 0.0;
        
        // Find bracketing points
        for (size_t i = 0; i &lt; data.size() - 1; ++i) {
            if (coord &gt;= data[i].first && coord &lt;= data[i+1].first) {
                // Linear interpolation
                double frac = (coord - data[i].first) / 
                             (data[i+1].first - data[i].first);
                return data[i].second + frac * (data[i+1].second - data[i].second);
            }
        }
        
        // Extrapolation (return nearest)
        if (coord &lt; data.front().first) return data.front().second;
        return data.back().second;
    }
    
    /**
     * @brief Compute L2 error against Ghia benchmark
     * @param field_name "velocity_x" or "velocity_y"
     * @param computed Computed values at centerline
     * @return L2 relative error
     */
    static double computeL2Error(
        const std::string& field_name,
        const std::vector&lt;std::pair&lt;double, double&gt;&gt;& computed) {
        
        auto reference = (field_name == "velocity_x") ? 
            getUCenterline() : getVCenterline();
        
        double sum_sq_error = 0.0;
        double sum_sq_ref = 0.0;
        
        for (const auto& [coord, value] : computed) {
            double ref_value = interpolate(reference, coord);
            double error = value - ref_value;
            sum_sq_error += error * error;
            sum_sq_ref += ref_value * ref_value;
        }
        
        if (sum_sq_ref &lt; 1e-10) return 0.0;
        return std::sqrt(sum_sq_error / sum_sq_ref);
    }
};

} // namespace validation
} // namespace fluidloom
